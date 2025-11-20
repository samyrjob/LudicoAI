#include "translation_engine.h"
#include "llama.h"
#include <string>
#include <vector>
#include <cstring>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>
#include <chrono>

/**
 * Translation Engine using T5/mT5 models via llama.cpp
 *
 * T5 is a text-to-text model that frames all NLP tasks as text generation.
 * For translation: Input format is "translate English to French: <text>"
 */

struct translation_request {
    std::string text;
    std::string source_lang;
    std::string target_lang;
    void *user_data;  // Per-request user data for callback
};

struct translation_engine_t {
    // llama.cpp context
    llama_model *model;
    llama_context *ctx;

    // Callback
    translation_callback_t callback;
    void *user_data;

    // Thread-safe queue
    std::queue<translation_request> request_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::thread worker_thread;
    bool shutdown;

    translation_engine_t()
        : model(nullptr), ctx(nullptr),
          callback(nullptr), user_data(nullptr),
          shutdown(false) {}
};

// Language code to language name mapping for T5 prompts
static std::string get_language_name(const char *lang_code) {
    if (strcmp(lang_code, "en") == 0) return "English";
    if (strcmp(lang_code, "fr") == 0) return "French";
    if (strcmp(lang_code, "es") == 0) return "Spanish";
    if (strcmp(lang_code, "de") == 0) return "German";
    if (strcmp(lang_code, "it") == 0) return "Italian";
    if (strcmp(lang_code, "pt") == 0) return "Portuguese";
    if (strcmp(lang_code, "nl") == 0) return "Dutch";
    if (strcmp(lang_code, "pl") == 0) return "Polish";
    if (strcmp(lang_code, "ru") == 0) return "Russian";
    if (strcmp(lang_code, "zh") == 0) return "Chinese";
    if (strcmp(lang_code, "ja") == 0) return "Japanese";
    if (strcmp(lang_code, "ko") == 0) return "Korean";
    if (strcmp(lang_code, "ar") == 0) return "Arabic";
    if (strcmp(lang_code, "hi") == 0) return "Hindi";
    if (strcmp(lang_code, "tr") == 0) return "Turkish";
    return "English";  // Default
}

// Build T5 translation prompt
static std::string build_t5_prompt(const char *text, const char *source_lang, const char *target_lang) {
    std::string source_name = get_language_name(source_lang);
    std::string target_name = get_language_name(target_lang);

    // T5 format: "translate English to French: <text>"
    return "translate " + source_name + " to " + target_name + ": " + std::string(text);
}

// Worker thread that processes translation requests
static void translation_worker(translation_engine_t *engine) {
    while (true) {
        translation_request req;

        {
            std::unique_lock<std::mutex> lock(engine->queue_mutex);
            engine->queue_cv.wait(lock, [engine] {
                return !engine->request_queue.empty() || engine->shutdown;
            });

            if (engine->shutdown && engine->request_queue.empty()) {
                break;
            }

            req = engine->request_queue.front();
            engine->request_queue.pop();
        }

        // Build T5 prompt
        auto start_time = std::chrono::steady_clock::now();
        std::string prompt = build_t5_prompt(req.text.c_str(),
                                             req.source_lang.c_str(),
                                             req.target_lang.c_str());

        std::cerr << "[Translation] [START] Prompt: " << prompt << std::endl;

        // Tokenize
        std::cerr << "[Translation] [TOKENIZE] Starting tokenization..." << std::endl;
        std::vector<llama_token> tokens;
        tokens.resize(prompt.size() + 128);  // Extra space

        // Get vocab from model
        const struct llama_vocab * vocab = llama_model_get_vocab(engine->model);

        int n_tokens = llama_tokenize(
            vocab,
            prompt.c_str(),
            prompt.size(),
            tokens.data(),
            tokens.size(),
            true,  // add_special (BOS token)
            false  // parse_special
        );

        if (n_tokens < 0) {
            std::cerr << "[Translation] [ERROR] Tokenization failed" << std::endl;
            if (engine->callback) {
                engine->callback("[Translation Error]", engine->user_data);
            }
            continue;
        }

        tokens.resize(n_tokens);
        std::cerr << "[Translation] [TOKENIZE] Success - " << n_tokens << " tokens" << std::endl;

        // Prepare batch for encoder
        std::cerr << "[Translation] [ENCODE] Preparing batch with " << n_tokens << " tokens..." << std::endl;
        llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);

        // Encode the input prompt (MT5 is encoder-decoder model)
        std::cerr << "[Translation] [ENCODE] Starting encoder..." << std::endl;
        if (llama_encode(engine->ctx, batch) != 0) {
            std::cerr << "[Translation] [ERROR] Encoding failed" << std::endl;
            if (engine->callback) {
                engine->callback("[Translation Error]", engine->user_data);
            }
            continue;
        }
        std::cerr << "[Translation] [ENCODE] Encoding success" << std::endl;

        // Start decoder with decoder start token (for T5/MT5 encoder-decoder models)
        std::cerr << "[Translation] [DECODE] Initializing decoder..." << std::endl;
        llama_token decoder_start_token = llama_model_decoder_start_token(engine->model);

        if (decoder_start_token < 0) {
            std::cerr << "[Translation] [ERROR] No decoder start token found for this model" << std::endl;
            if (engine->callback) {
                engine->callback("[Translation Error: Invalid model]", req.user_data);
            }
            continue;
        }

        std::cerr << "[Translation] [DECODE] Using decoder start token: " << decoder_start_token << std::endl;
        batch = llama_batch_get_one(&decoder_start_token, 1);
        if (llama_decode(engine->ctx, batch) != 0) {
            std::cerr << "[Translation] [ERROR] Initial decoder step failed" << std::endl;
            if (engine->callback) {
                engine->callback("[Translation Error]", req.user_data);
            }
            continue;
        }
        std::cerr << "[Translation] [DECODE] Decoder initialized" << std::endl;

        // Generate translation (max 256 tokens)
        std::cerr << "[Translation] [GENERATE] Starting generation (max 256 tokens)..." << std::endl;
        std::string result;
        int n_generated = 0;
        const int max_tokens = 256;

        while (n_generated < max_tokens) {
            if (n_generated % 10 == 0 && n_generated > 0) {
                std::cerr << "[Translation] [GENERATE] Progress: " << n_generated << " tokens generated" << std::endl;
            }
            // Get logits and sample next token (greedy sampling)
            float * logits = llama_get_logits_ith(engine->ctx, -1);
            int n_vocab = llama_vocab_n_tokens(vocab);

            // Find token with highest probability (greedy)
            llama_token new_token = 0;
            float max_logit = logits[0];
            for (int i = 1; i < n_vocab; i++) {
                if (logits[i] > max_logit) {
                    max_logit = logits[i];
                    new_token = i;
                }
            }

            // Check for EOS
            if (llama_vocab_is_eog(vocab, new_token)) {
                break;
            }

            // Decode token to text
            char buf[128];
            int n = llama_token_to_piece(vocab, new_token, buf, sizeof(buf), 0, false);
            if (n > 0) {
                result.append(buf, n);
            }

            // Prepare next batch with new token
            batch = llama_batch_get_one(&new_token, 1);
            if (llama_decode(engine->ctx, batch) != 0) {
                std::cerr << "[Translation] [ERROR] Decode step failed at token " << n_generated << std::endl;
                break;
            }

            n_generated++;
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cerr << "[Translation] [COMPLETE] Generated " << n_generated << " tokens in "
                  << duration.count() << "ms" << std::endl;
        std::cerr << "[Translation] [RESULT] " << result << std::endl;

        // Invoke callback with result and request-specific user_data
        if (engine->callback) {
            std::cerr << "[Translation] [CALLBACK] Invoking callback..." << std::endl;
            engine->callback(result.c_str(), req.user_data);
            std::cerr << "[Translation] [CALLBACK] Callback completed" << std::endl;
        }
    }

    std::cerr << "[Translation] [WORKER] Thread exiting" << std::endl;
}

extern "C" {

translation_engine_t* translation_init(
    const char *model_path,
    translation_callback_t callback,
    void *user_data
) {
    if (!model_path || !callback) {
        std::cerr << "[Translation] Invalid parameters" << std::endl;
        return nullptr;
    }

    translation_engine_t *engine = new translation_engine_t();
    engine->callback = callback;
    engine->user_data = user_data;

    // Initialize llama backend
    llama_backend_init();

    // Load model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 16;  // Offload 16 layers to GPU (reduce if still OOM)

    engine->model = llama_model_load_from_file(model_path, model_params);
    if (!engine->model) {
        std::cerr << "[Translation] Failed to load model: " << model_path << std::endl;
        delete engine;
        return nullptr;
    }

    // Create context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 512;  // Context size for translation
    ctx_params.n_batch = 512;
    ctx_params.n_ubatch = 512;
    ctx_params.n_threads = 4;  // CPU threads
    ctx_params.n_threads_batch = 4;

    engine->ctx = llama_init_from_model(engine->model, ctx_params);
    if (!engine->ctx) {
        std::cerr << "[Translation] Failed to create context" << std::endl;
        llama_model_free(engine->model);
        delete engine;
        return nullptr;
    }

    // Start worker thread
    engine->worker_thread = std::thread(translation_worker, engine);

    std::cerr << "[Translation] Engine initialized with model: " << model_path << std::endl;

    return engine;
}

bool translation_translate(
    translation_engine_t *engine,
    const char *text,
    const char *source_lang,
    const char *target_lang,
    void *user_data
) {
    if (!engine || !text || !source_lang || !target_lang) {
        return false;
    }

    translation_request req;
    req.text = text;
    req.source_lang = source_lang;
    req.target_lang = target_lang;
    req.user_data = user_data;

    {
        std::lock_guard<std::mutex> lock(engine->queue_mutex);
        engine->request_queue.push(req);
    }

    engine->queue_cv.notify_one();

    return true;
}

bool translation_is_ready(translation_engine_t *engine) {
    return engine && engine->model && engine->ctx;
}

void translation_cleanup(translation_engine_t *engine) {
    if (!engine) return;

    // Signal shutdown
    {
        std::lock_guard<std::mutex> lock(engine->queue_mutex);
        engine->shutdown = true;
    }
    engine->queue_cv.notify_one();

    // Wait for worker thread
    if (engine->worker_thread.joinable()) {
        engine->worker_thread.join();
    }

    // Free llama.cpp resources
    if (engine->ctx) {
        llama_free(engine->ctx);
    }
    if (engine->model) {
        llama_model_free(engine->model);
    }

    llama_backend_free();

    delete engine;

    std::cerr << "[Translation] Engine cleaned up" << std::endl;
}

}  // extern "C"
