#include "audio.h"
#include "whisper_engine.h"
#include "translation_engine.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

/* Global state */
static audio_context_t *g_audio = NULL;
static whisper_engine_t *g_whisper = NULL;
static translation_engine_t *g_translator = NULL;
static volatile sig_atomic_t g_running = 1;

/* Translation settings */
static const char *g_target_lang = NULL;
static const char *g_source_lang = NULL;

/* Language detection state */
static char g_last_detected_lang[8] = {0};
static time_t g_last_lang_check = 0;

/* Configuration */
#define DEFAULT_MODEL_PATH "models/whisper-base.gguf"
#define DEFAULT_TRANSLATION_MODEL "models/madlad400-10b-mt.gguf"
#define DEFAULT_LANGUAGE NULL  /* Auto-detect language */
#define AUDIO_CHUNK_SIZE (AUDIO_SAMPLE_RATE * 3)  /* 3 seconds */

/* Audio buffer for accumulation */
static float audio_buffer[AUDIO_CHUNK_SIZE];
static size_t audio_buffer_pos = 0;

/* Signal handler for graceful shutdown */
static void signal_handler(int sig) {
    (void)sig;
    fprintf(stderr, "\n[Main] Received shutdown signal\n");
    g_running = 0;
}

/* Translation callback - called when translation is ready */
static void on_translation(const char *translated_text, void *user_data) {
    const char *original_text = (const char *)user_data;

    if (translated_text && original_text) {
        fprintf(stderr, "[Translation] %s → %s\n", original_text, translated_text);

        /* Send to frontend via IPC */
        time_t now = time(NULL);
        ipc_send_translation(translated_text, original_text, (long)now);
    }

    /* Free the original text copy */
    if (original_text) {
        free((void *)original_text);
    }
}

/* Transcription callback - called when Whisper has results */
static void on_transcription(const char *text, void *user_data) {
    (void)user_data;

    if (text && strlen(text) > 0) {
        fprintf(stderr, "[Transcription] %s\n", text);

        /* Send to frontend via IPC */
        time_t now = time(NULL);
        ipc_send_transcription(text, (long)now);

        /* If translation is enabled, translate the text */
        if (g_translator && g_target_lang) {
            /* Make a copy of the text for the translation callback */
            char *text_copy = strdup(text);
            if (text_copy) {
                const char *source_lang = g_source_lang ? g_source_lang : "auto";
                translation_translate(g_translator, text_copy, source_lang, g_target_lang, text_copy);
            }
        }
    }
}

/* Audio callback - called when audio data is available */
static void on_audio_data(const float *samples, size_t num_samples, void *user_data) {
    (void)user_data;

    /* Accumulate audio in buffer */
    for (size_t i = 0; i < num_samples; i++) {
        audio_buffer[audio_buffer_pos++] = samples[i];

        /* Process when buffer is full (3 seconds) */
        if (audio_buffer_pos >= AUDIO_CHUNK_SIZE) {
            /* Process with Whisper */
            if (g_whisper) {
                whisper_engine_process(g_whisper, audio_buffer, audio_buffer_pos);
            }

            /* Overlap: keep last 1 second for context */
            const size_t overlap = AUDIO_SAMPLE_RATE;
            memmove(audio_buffer, audio_buffer + audio_buffer_pos - overlap,
                   overlap * sizeof(float));
            audio_buffer_pos = overlap;
        }
    }
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -m MODEL    Path to Whisper model (default: %s)\n", DEFAULT_MODEL_PATH);
    fprintf(stderr, "  -l LANG     Language code (en, fr, es, etc.) or 'auto' for auto-detect (default: auto)\n");
    fprintf(stderr, "  -t LANG     Target language for translation (optional, e.g., en, fr, es)\n");
    fprintf(stderr, "  -T MODEL    Path to translation model (default: %s)\n", DEFAULT_TRANSLATION_MODEL);
    fprintf(stderr, "  -h          Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *model_path = DEFAULT_MODEL_PATH;
    const char *language = DEFAULT_LANGUAGE;
    const char *translation_model_path = DEFAULT_TRANSLATION_MODEL;
    const char *target_lang = NULL;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            model_path = argv[++i];
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            language = argv[++i];
            if (strcmp(language, "auto") == 0) {
                language = NULL;  /* Auto-detect */
            }
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            target_lang = argv[++i];
        } else if (strcmp(argv[i], "-T") == 0 && i + 1 < argc) {
            translation_model_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Store translation settings in global variables */
    g_target_lang = target_lang;
    g_source_lang = language;

    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    fprintf(stderr, "=== VisualIA Backend ===\n");
    fprintf(stderr, "[Main] Starting up...\n");

    /* Initialize IPC */
    if (!ipc_init()) {
        fprintf(stderr, "[Main] Failed to initialize IPC\n");
        return 1;
    }

    ipc_send_status("Initializing Whisper...");

    /* Initialize Whisper */
    g_whisper = whisper_engine_init(model_path, language, on_transcription, NULL);
    if (!g_whisper) {
        fprintf(stderr, "[Main] Failed to initialize Whisper: %s\n", whisper_engine_get_error());
        ipc_send_error("Failed to initialize Whisper");
        ipc_cleanup();
        return 1;
    }

    /* Initialize translation engine if target language is specified */
    if (target_lang) {
        ipc_send_status("Initializing translation engine...");
        fprintf(stderr, "[Main] Initializing translation: %s → %s\n",
                language ? language : "auto", target_lang);

        g_translator = translation_init(translation_model_path, on_translation, NULL);
        if (!g_translator) {
            fprintf(stderr, "[Main] Warning: Failed to initialize translation engine\n");
            fprintf(stderr, "[Main] Translation will be disabled. Continuing without translation...\n");
            ipc_send_status("Translation unavailable - continuing with transcription only");
            g_target_lang = NULL;  /* Disable translation */
        } else {
            fprintf(stderr, "[Main] Translation engine ready\n");
            ipc_send_status("Translation engine ready");
        }
    }

    ipc_send_status("Initializing audio capture...");

    /* Initialize audio capture */
    g_audio = audio_init(on_audio_data, NULL);
    if (!g_audio) {
        fprintf(stderr, "[Main] Failed to initialize audio: %s\n", audio_get_error());
        ipc_send_error("Failed to initialize audio capture");
        whisper_engine_cleanup(g_whisper);
        ipc_cleanup();
        return 1;
    }

    /* Start audio capture */
    if (!audio_start(g_audio)) {
        fprintf(stderr, "[Main] Failed to start audio: %s\n", audio_get_error());
        ipc_send_error("Failed to start audio capture");
        audio_cleanup(g_audio);
        whisper_engine_cleanup(g_whisper);
        ipc_cleanup();
        return 1;
    }

    ipc_send_status("Running - listening for audio...");
    fprintf(stderr, "[Main] Running (press Ctrl+C to stop)\n");

    /* Main loop */
    while (g_running) {
        /* Poll for IPC messages */
        ipc_poll();

        /* Check for language detection changes (every second) */
        time_t now = time(NULL);
        if (now - g_last_lang_check >= 1) {
            g_last_lang_check = now;

            const char *detected_lang = whisper_engine_get_detected_language(g_whisper);
            if (detected_lang && strlen(detected_lang) > 0) {
                /* Check if language has changed */
                if (strcmp(g_last_detected_lang, detected_lang) != 0) {
                    fprintf(stderr, "[Main] Language changed: %s → %s\n",
                            g_last_detected_lang[0] ? g_last_detected_lang : "none",
                            detected_lang);

                    strncpy(g_last_detected_lang, detected_lang, sizeof(g_last_detected_lang) - 1);
                    g_last_detected_lang[sizeof(g_last_detected_lang) - 1] = '\0';

                    /* Send language detection to frontend */
                    ipc_send_language_detected(detected_lang);

                    /* TODO: Reload Whisper with language-specific model */
                    char status_msg[128];
                    snprintf(status_msg, sizeof(status_msg),
                            "Detected language: %s - Consider using language-specific model",
                            detected_lang);
                    ipc_send_status(status_msg);
                }
            }
        }

        /* Sleep briefly to avoid busy waiting */
        usleep(100000);  /* 100ms */
    }

    /* Cleanup */
    fprintf(stderr, "[Main] Shutting down...\n");
    ipc_send_status("Shutting down...");

    audio_stop(g_audio);
    audio_cleanup(g_audio);
    whisper_engine_cleanup(g_whisper);

    if (g_translator) {
        translation_cleanup(g_translator);
    }

    ipc_cleanup();

    fprintf(stderr, "[Main] Goodbye!\n");
    return 0;
}
