// #include "whisper_engine.h"
// #include "whisper.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <pthread.h>
// #include <time.h>

// struct whisper_engine {
//     struct whisper_context *ctx;
//     struct whisper_context_params cparams;
//     struct whisper_full_params wparams;
//     transcription_callback_t callback;
//     void *user_data;
//     pthread_mutex_t lock;
//     char detected_language[8];  /* Store detected language code */
//     time_t last_detection_time; /* Time of last language detection */
// };

// static char last_error[256] = {0};

// whisper_engine_t* whisper_engine_init(const char *model_path, const char *language, transcription_callback_t callback, void *user_data) {
//     if (!model_path || !callback) {
//         snprintf(last_error, sizeof(last_error), "Invalid parameters");
//         return NULL;
//     }

//     whisper_engine_t *engine = calloc(1, sizeof(whisper_engine_t));
//     if (!engine) {
//         snprintf(last_error, sizeof(last_error), "Memory allocation failed");
//         return NULL;
//     }

//     /* Initialize context parameters */
//     engine->cparams = whisper_context_default_params();
//     engine->cparams.use_gpu = true;  /* Try to use GPU if available */

//     /* Load model */
//     fprintf(stderr, "[Whisper] Loading model: %s\n", model_path);
//     engine->ctx = whisper_init_from_file_with_params(model_path, engine->cparams);
//     if (!engine->ctx) {
//         snprintf(last_error, sizeof(last_error), "Failed to load model: %s", model_path);
//         free(engine);
//         return NULL;
//     }

//     /* Initialize whisper parameters */
//     engine->wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
//     engine->wparams.print_progress = false;
//     engine->wparams.print_realtime = false;
//     engine->wparams.print_timestamps = false;
//     engine->wparams.translate = false;

//     /* Set language (NULL = auto-detect) */
//     if (language && strlen(language) > 0) {
//         engine->wparams.language = language;
//         fprintf(stderr, "[Whisper] Language set to: %s\n", language);
//     } else {
//         engine->wparams.language = NULL;  /* Auto-detect */
//         fprintf(stderr, "[Whisper] Language: auto-detect\n");
//     }

//     engine->wparams.n_threads = 4;
//     engine->wparams.no_context = true;
//     engine->wparams.single_segment = false;

//     engine->callback = callback;
//     engine->user_data = user_data;
//     pthread_mutex_init(&engine->lock, NULL);

//     /* Initialize language detection */
//     memset(engine->detected_language, 0, sizeof(engine->detected_language));
//     engine->last_detection_time = 0;

//     fprintf(stderr, "[Whisper] Initialized successfully\n");
//     return engine;
// }

// bool whisper_engine_process(whisper_engine_t *engine, const float *samples, size_t num_samples) {
//     if (!engine || !samples || num_samples == 0) {
//         snprintf(last_error, sizeof(last_error), "Invalid parameters");
//         return false;
//     }

//     pthread_mutex_lock(&engine->lock);

//     /* Check if we should detect language (every 20 seconds) */
//     time_t current_time = time(NULL);
//     bool should_detect = (current_time - engine->last_detection_time >= 20);

//     /* Run inference */
//     if (whisper_full(engine->ctx, engine->wparams, samples, (int)num_samples) != 0) {
//         pthread_mutex_unlock(&engine->lock);
//         snprintf(last_error, sizeof(last_error), "Whisper inference failed");
//         return false;
//     }

//     /* Detect language if it's time */
//     if (should_detect && engine->wparams.language == NULL) {
//         int lang_id = whisper_full_lang_id(engine->ctx);
//         const char* lang_str = whisper_lang_str(lang_id);
//         if (lang_str && strlen(lang_str) > 0) {
//             strncpy(engine->detected_language, lang_str, sizeof(engine->detected_language) - 1);
//             engine->detected_language[sizeof(engine->detected_language) - 1] = '\0';
//             engine->last_detection_time = current_time;
//             fprintf(stderr, "[Whisper] Detected language: %s\n", engine->detected_language);
//         }
//     }

//     /* Get transcription results */
//     const int n_segments = whisper_full_n_segments(engine->ctx);
//     if (n_segments > 0) {
//         /* Build complete transcription */
//         char transcription[4096] = {0};
//         size_t offset = 0;

//         for (int i = 0; i < n_segments; i++) {
//             const char *text = whisper_full_get_segment_text(engine->ctx, i);
//             if (text) {
//                 size_t len = strlen(text);
//                 if (offset + len + 1 < sizeof(transcription)) {
//                     memcpy(transcription + offset, text, len);
//                     offset += len;
//                 }
//             }
//         }

//         transcription[offset] = '\0';

//         /* Trim leading/trailing whitespace */
//         char *start = transcription;
//         while (*start == ' ' || *start == '\t' || *start == '\n') start++;

//         if (*start != '\0') {
//             pthread_mutex_unlock(&engine->lock);
//             engine->callback(start, engine->user_data);
//             return true;
//         }
//     }

//     pthread_mutex_unlock(&engine->lock);
//     return true;  /* No error, just no speech detected */
// }

// void whisper_engine_cleanup(whisper_engine_t *engine) {
//     if (!engine) return;

//     pthread_mutex_lock(&engine->lock);

//     if (engine->ctx) {
//         whisper_free(engine->ctx);
//     }

//     pthread_mutex_unlock(&engine->lock);
//     pthread_mutex_destroy(&engine->lock);

//     free(engine);
//     fprintf(stderr, "[Whisper] Cleanup complete\n");
// }

// const char* whisper_engine_get_error(void) {
//     return last_error;
// }

// const char* whisper_engine_get_detected_language(whisper_engine_t *engine) {
//     if (!engine || engine->detected_language[0] == '\0') {
//         return NULL;
//     }
//     return engine->detected_language;
// }
#include "whisper_engine.h"
#include "whisper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Platform-specific threading includes */
#ifdef _WIN32
    #include <windows.h>
    typedef CRITICAL_SECTION pthread_mutex_t;
    #define pthread_mutex_init(m, attr) InitializeCriticalSection(m)
    #define pthread_mutex_lock(m) EnterCriticalSection(m)
    #define pthread_mutex_unlock(m) LeaveCriticalSection(m)
    #define pthread_mutex_destroy(m) DeleteCriticalSection(m)
#else
    #include <pthread.h>
#endif

struct whisper_engine {
    struct whisper_context *ctx;
    struct whisper_context_params cparams;
    struct whisper_full_params wparams;
    transcription_callback_t callback;
    void *user_data;
    pthread_mutex_t lock;
    char detected_language[8];  /* Store detected language code */
    time_t last_detection_time; /* Time of last language detection */
};

static char last_error[256] = {0};

whisper_engine_t* whisper_engine_init(const char *model_path, const char *language, transcription_callback_t callback, void *user_data) {
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

    /* Set language (NULL = auto-detect) */
    if (language && strlen(language) > 0) {
        engine->wparams.language = language;
        fprintf(stderr, "[Whisper] Language set to: %s\n", language);
    } else {
        engine->wparams.language = NULL;  /* Auto-detect */
        fprintf(stderr, "[Whisper] Language: auto-detect\n");
    }

    engine->wparams.n_threads = 4;
    engine->wparams.no_context = true;
    engine->wparams.single_segment = false;

    engine->callback = callback;
    engine->user_data = user_data;
    pthread_mutex_init(&engine->lock, NULL);

    /* Initialize language detection */
    memset(engine->detected_language, 0, sizeof(engine->detected_language));
    engine->last_detection_time = 0;

    fprintf(stderr, "[Whisper] Initialized successfully\n");
    return engine;
}

bool whisper_engine_process(whisper_engine_t *engine, const float *samples, size_t num_samples) {
    if (!engine || !samples || num_samples == 0) {
        snprintf(last_error, sizeof(last_error), "Invalid parameters");
        return false;
    }

    pthread_mutex_lock(&engine->lock);

    /* Check if we should detect language (every 20 seconds) */
    time_t current_time = time(NULL);
    bool should_detect = (current_time - engine->last_detection_time >= 20);

    /* Run inference */
    if (whisper_full(engine->ctx, engine->wparams, samples, (int)num_samples) != 0) {
        pthread_mutex_unlock(&engine->lock);
        snprintf(last_error, sizeof(last_error), "Whisper inference failed");
        return false;
    }

    /* Detect language if it's time */
    if (should_detect && engine->wparams.language == NULL) {
        int lang_id = whisper_full_lang_id(engine->ctx);
        const char* lang_str = whisper_lang_str(lang_id);
        if (lang_str && strlen(lang_str) > 0) {
            strncpy(engine->detected_language, lang_str, sizeof(engine->detected_language) - 1);
            engine->detected_language[sizeof(engine->detected_language) - 1] = '\0';
            engine->last_detection_time = current_time;
            fprintf(stderr, "[Whisper] Detected language: %s\n", engine->detected_language);
        }
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

void whisper_engine_cleanup(whisper_engine_t *engine) {
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

const char* whisper_engine_get_error(void) {
    return last_error;
}

const char* whisper_engine_get_detected_language(whisper_engine_t *engine) {
    if (!engine || engine->detected_language[0] == '\0') {
        return NULL;
    }
    return engine->detected_language;
}