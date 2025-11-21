#ifndef TRANSLATION_ENGINE_H
#define TRANSLATION_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Translation Engine using T5/mT5 models via llama.cpp
 *
 * Supports multilingual translation with T5 encoder-decoder architecture.
 * Uses existing llama.cpp infrastructure for zero new dependencies.
 */

typedef struct translation_engine_t translation_engine_t;

/**
 * Callback for translation results
 *
 * @param translated_text The translated text (UTF-8)
 * @param user_data User-provided context pointer
 */
typedef void (*translation_callback_t)(const char *translated_text, void *user_data);

/**
 * Initialize translation engine with T5 model
 *
 * @param model_path Path to GGUF T5/mT5 model file
 * @param callback Callback function for translation results
 * @param user_data User context to pass to callback
 * @return Initialized engine or NULL on failure
 */
translation_engine_t* translation_init(
    const char *model_path,
    translation_callback_t callback,
    void *user_data
);

/**
 * Translate text from source language to target language
 *
 * @param engine The translation engine
 * @param text Text to translate (UTF-8)
 * @param source_lang Source language code (e.g., "en", "fr", "auto")
 * @param target_lang Target language code (e.g., "en", "fr", "es")
 * @param user_data User context to pass to callback for this request
 * @return true if translation request was queued successfully
 *
 * Note: Translation is asynchronous. Result will be delivered via callback.
 */
bool translation_translate(
    translation_engine_t *engine,
    const char *text,
    const char *source_lang,
    const char *target_lang,
    void *user_data
);

/**
 * Check if translation engine is ready
 *
 * @param engine The translation engine
 * @return true if ready to accept translation requests
 */
bool translation_is_ready(translation_engine_t *engine);

/**
 * Clean up and free translation engine resources
 *
 * @param engine The translation engine to cleanup
 */
void translation_cleanup(translation_engine_t *engine);

#ifdef __cplusplus
}
#endif

#endif /* TRANSLATION_ENGINE_H */
