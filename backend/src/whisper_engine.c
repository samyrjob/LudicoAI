#include "whisper_engine.h"
#include "whisper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct whisper_engine {
    struct whisper_context *ctx;
    struct whisper_context_params cparams;
    struct whisper_full_params wparams;
    transcription_callback_t callback;
    void *user_data;
    pthread_mutex_t lock;
};

static char last_error[256] = {0};

whisper_engine_t* whisper_init(const char *model_path, transcription_callback_t callback, void *user_data) {
    if (!model_path || !callback) {
        snprintf(last_error, sizeof(last_error), "Invalid parameters");
        return NULL;
    }

    whisper_engine_t *engine = calloc(1, sizeof(whisper_engine_t));
    if (!engine) {
        snprintf(last_error, sizeof(last_error), "Memory allocation failed");
        return NULL;
    }

    /* Initialize context parameters */
    engine->cparams = whisper_context_default_params();
    engine->cparams.use_gpu = true;  /* Try to use GPU if available */

    /* Load model */
    fprintf(stderr, "[Whisper] Loading model: %s\n", model_path);
    engine->ctx = whisper_init_from_file_with_params(model_path, engine->cparams);
    if (!engine->ctx) {
        snprintf(last_error, sizeof(last_error), "Failed to load model: %s", model_path);
        free(engine);
        return NULL;
    }

    /* Initialize whisper parameters */
    engine->wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    engine->wparams.print_progress = false;
    engine->wparams.print_realtime = false;
    engine->wparams.print_timestamps = false;
    engine->wparams.translate = false;
    engine->wparams.language = "en";  /* English by default */
    engine->wparams.n_threads = 4;
    engine->wparams.no_context = true;
    engine->wparams.single_segment = false;

    engine->callback = callback;
    engine->user_data = user_data;
    pthread_mutex_init(&engine->lock, NULL);

    fprintf(stderr, "[Whisper] Initialized successfully\n");
    return engine;
}

bool whisper_process(whisper_engine_t *engine, const float *samples, size_t num_samples) {
    if (!engine || !samples || num_samples == 0) {
        snprintf(last_error, sizeof(last_error), "Invalid parameters");
        return false;
    }

    pthread_mutex_lock(&engine->lock);

    /* Run inference */
    if (whisper_full(engine->ctx, engine->wparams, samples, (int)num_samples) != 0) {
        pthread_mutex_unlock(&engine->lock);
        snprintf(last_error, sizeof(last_error), "Whisper inference failed");
        return false;
    }

    /* Get transcription results */
    const int n_segments = whisper_full_n_segments(engine->ctx);
    if (n_segments > 0) {
        /* Build complete transcription */
        char transcription[4096] = {0};
        size_t offset = 0;

        for (int i = 0; i < n_segments; i++) {
            const char *text = whisper_full_get_segment_text(engine->ctx, i);
            if (text) {
                size_t len = strlen(text);
                if (offset + len + 1 < sizeof(transcription)) {
                    memcpy(transcription + offset, text, len);
                    offset += len;
                }
            }
        }

        transcription[offset] = '\0';

        /* Trim leading/trailing whitespace */
        char *start = transcription;
        while (*start == ' ' || *start == '\t' || *start == '\n') start++;

        if (*start != '\0') {
            pthread_mutex_unlock(&engine->lock);
            engine->callback(start, engine->user_data);
            return true;
        }
    }

    pthread_mutex_unlock(&engine->lock);
    return true;  /* No error, just no speech detected */
}

void whisper_cleanup(whisper_engine_t *engine) {
    if (!engine) return;

    pthread_mutex_lock(&engine->lock);

    if (engine->ctx) {
        whisper_free(engine->ctx);
    }

    pthread_mutex_unlock(&engine->lock);
    pthread_mutex_destroy(&engine->lock);

    free(engine);
    fprintf(stderr, "[Whisper] Cleanup complete\n");
}

const char* whisper_get_error(void) {
    return last_error;
}
