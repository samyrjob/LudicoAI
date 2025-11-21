#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Audio configuration */
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_CHANNELS 1
#define AUDIO_BUFFER_MS 3000  /* 3 second buffer for Whisper */
#define AUDIO_BUFFER_SIZE (AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * AUDIO_BUFFER_MS / 1000)

/* Audio callback function type */
typedef void (*audio_callback_t)(const float *samples, size_t num_samples, void *user_data);

/* Audio context (opaque) */
typedef struct audio_context audio_context_t;

/**
 * Initialize audio capture
 * @param callback Function to call when audio data is available
 * @param user_data User data to pass to callback
 * @return Audio context or NULL on failure
 */
audio_context_t* audio_init(audio_callback_t callback, void *user_data);

/**
 * Start audio capture
 * @param ctx Audio context
 * @return true on success, false on failure
 */
bool audio_start(audio_context_t *ctx);

/**
 * Stop audio capture
 * @param ctx Audio context
 */
void audio_stop(audio_context_t *ctx);

/**
 * Cleanup audio resources
 * @param ctx Audio context
 */
void audio_cleanup(audio_context_t *ctx);

/**
 * Get last error message
 * @return Error message string
 */
const char* audio_get_error(void);

#endif /* AUDIO_H */
