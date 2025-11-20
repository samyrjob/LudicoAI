// #include "ipc.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <time.h>

// /* Simple JSON-RPC implementation using stdio */

// bool ipc_init(void) {
//     /* Set stdout to line buffering for immediate output */
//     setvbuf(stdout, NULL, _IOLBF, 0);
//     setvbuf(stderr, NULL, _IOLBF, 0);

//     fprintf(stderr, "[IPC] Initialized (stdio mode)\n");
//     return true;
// }

// static void escape_json_string(const char *src, char *dest, size_t dest_size) {
//     size_t j = 0;
//     for (size_t i = 0; src[i] != '\0' && j < dest_size - 2; i++) {
//         if (src[i] == '"' || src[i] == '\\') {
//             dest[j++] = '\\';
//         }
//         dest[j++] = src[i];
//     }
//     dest[j] = '\0';
// }

// bool ipc_send_transcription(const char *text, long timestamp) {
//     if (!text) return false;

//     /* Escape special characters in JSON */
//     char escaped[4096];
//     escape_json_string(text, escaped, sizeof(escaped));

//     /* Send JSON message */
//     printf("{\"type\":\"transcription\",\"data\":{\"text\":\"%s\",\"timestamp\":%ld}}\n",
//            escaped, timestamp);
//     fflush(stdout);

//     return true;
// }

// bool ipc_send_error(const char *error_msg) {
//     if (!error_msg) return false;

//     char escaped[1024];
//     escape_json_string(error_msg, escaped, sizeof(escaped));

//     printf("{\"type\":\"error\",\"data\":{\"message\":\"%s\"}}\n", escaped);
//     fflush(stdout);

//     return true;
// }

// bool ipc_send_status(const char *status) {
//     if (!status) return false;

//     char escaped[1024];
//     escape_json_string(status, escaped, sizeof(escaped));

//     printf("{\"type\":\"status\",\"data\":{\"message\":\"%s\"}}\n", escaped);
//     fflush(stdout);

//     return true;
// }

// bool ipc_send_translation(const char *translated_text, const char *original_text, long timestamp) {
//     if (!translated_text || !original_text) return false;

//     char escaped_translation[4096];
//     char escaped_original[4096];
//     escape_json_string(translated_text, escaped_translation, sizeof(escaped_translation));
//     escape_json_string(original_text, escaped_original, sizeof(escaped_original));

//     printf("{\"type\":\"translation\",\"data\":{\"text\":\"%s\",\"original\":\"%s\",\"timestamp\":%ld}}\n",
//            escaped_translation, escaped_original, timestamp);
//     fflush(stdout);

//     return true;
// }

// bool ipc_send_language_detected(const char *language) {
//     if (!language) return false;

//     char escaped[32];
//     escape_json_string(language, escaped, sizeof(escaped));

//     printf("{\"type\":\"language_detected\",\"data\":{\"language\":\"%s\"}}\n", escaped);
//     fflush(stdout);

//     return true;
// }

// bool ipc_poll(void) {
//     /* For now, we don't expect messages from frontend */
//     /* This can be extended to handle control commands */
//     return false;
// }

// void ipc_cleanup(void) {
//     fflush(stdout);
//     fflush(stderr);
//     fprintf(stderr, "[IPC] Cleanup complete\n");
// }

#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Simple JSON-RPC implementation using stdio */

bool ipc_init(void) {
    /* Set stdout to line buffering for immediate output */
    #ifdef _WIN32
        /* Windows doesn't support _IOLBF, use unbuffered mode */
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    #else
        /* POSIX systems support line buffering */
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IOLBF, 0);
    #endif

    fprintf(stderr, "[IPC] Initialized (stdio mode)\n");
    return true;
}

static void escape_json_string(const char *src, char *dest, size_t dest_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dest_size - 2; i++) {
        if (src[i] == '"' || src[i] == '\\') {
            dest[j++] = '\\';
        }
        dest[j++] = src[i];
    }
    dest[j] = '\0';
}

bool ipc_send_transcription(const char *text, long timestamp) {
    if (!text) return false;

    /* Escape special characters in JSON */
    char escaped[4096];
    escape_json_string(text, escaped, sizeof(escaped));

    /* Send JSON message */
    printf("{\"type\":\"transcription\",\"data\":{\"text\":\"%s\",\"timestamp\":%ld}}\n",
           escaped, timestamp);
    fflush(stdout);

    return true;
}

bool ipc_send_error(const char *error_msg) {
    if (!error_msg) return false;

    char escaped[1024];
    escape_json_string(error_msg, escaped, sizeof(escaped));

    printf("{\"type\":\"error\",\"data\":{\"message\":\"%s\"}}\n", escaped);
    fflush(stdout);

    return true;
}

bool ipc_send_status(const char *status) {
    if (!status) return false;

    char escaped[1024];
    escape_json_string(status, escaped, sizeof(escaped));

    printf("{\"type\":\"status\",\"data\":{\"message\":\"%s\"}}\n", escaped);
    fflush(stdout);

    return true;
}

bool ipc_send_translation(const char *translated_text, const char *original_text, long timestamp) {
    if (!translated_text || !original_text) return false;

    char escaped_translation[4096];
    char escaped_original[4096];
    escape_json_string(translated_text, escaped_translation, sizeof(escaped_translation));
    escape_json_string(original_text, escaped_original, sizeof(escaped_original));

    printf("{\"type\":\"translation\",\"data\":{\"text\":\"%s\",\"original\":\"%s\",\"timestamp\":%ld}}\n",
           escaped_translation, escaped_original, timestamp);
    fflush(stdout);

    return true;
}

bool ipc_send_language_detected(const char *language) {
    if (!language) return false;

    char escaped[32];
    escape_json_string(language, escaped, sizeof(escaped));

    printf("{\"type\":\"language_detected\",\"data\":{\"language\":\"%s\"}}\n", escaped);
    fflush(stdout);

    return true;
}

bool ipc_poll(void) {
    /* For now, we don't expect messages from frontend */
    /* This can be extended to handle control commands */
    return false;
}

void ipc_cleanup(void) {
    fflush(stdout);
    fflush(stderr);
    fprintf(stderr, "[IPC] Cleanup complete\n");
}