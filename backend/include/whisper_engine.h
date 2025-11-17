#ifndef WHISPER_ENGINE_H
#define WHISPER_ENGINE_H

#include <stddef.h>
#include <stdbool.h>

/* Whisper engine context (opaque) */
typedef struct whisper_engine whisper_engine_t;

/* Transcription result callback */
typedef void (*transcription_callback_t)(const char *text, void *user_data);

/**
 * Initialize Whisper engine
 * @param model_path Path to Whisper model file (.gguf)
 * @param callback Function to call when transcription is ready
 * @param user_data User data to pass to callback
 * @return Whisper engine context or NULL on failure
 */
whisper_engine_t* whisper_init(const char *model_path, transcription_callback_t callback, void *user_data);

/**
 * Process audio samples for transcription
 * @param engine Whisper engine context
 * @param samples Audio samples (float32, mono, 16kHz)
 * @param num_samples Number of samples
 * @return true on success, false on failure
 */
bool whisper_process(whisper_engine_t *engine, const float *samples, size_t num_samples);

/**
 * Cleanup Whisper engine
 * @param engine Whisper engine context
 */
void whisper_cleanup(whisper_engine_t *engine);

/**
 * Get last error message
 * @return Error message string
 */
const char* whisper_get_error(void);

#endif /* WHISPER_ENGINE_H */
