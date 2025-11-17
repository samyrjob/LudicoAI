#include "audio.h"
#include "whisper_engine.h"
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
static volatile sig_atomic_t g_running = 1;

/* Configuration */
#define DEFAULT_MODEL_PATH "models/whisper-base.en.gguf"
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

/* Transcription callback - called when Whisper has results */
static void on_transcription(const char *text, void *user_data) {
    (void)user_data;

    if (text && strlen(text) > 0) {
        fprintf(stderr, "[Transcription] %s\n", text);

        /* Send to frontend via IPC */
        time_t now = time(NULL);
        ipc_send_transcription(text, (long)now);
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
                whisper_process(g_whisper, audio_buffer, audio_buffer_pos);
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
    fprintf(stderr, "  -h          Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *model_path = DEFAULT_MODEL_PATH;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            model_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

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
    g_whisper = whisper_init(model_path, on_transcription, NULL);
    if (!g_whisper) {
        fprintf(stderr, "[Main] Failed to initialize Whisper: %s\n", whisper_get_error());
        ipc_send_error("Failed to initialize Whisper");
        ipc_cleanup();
        return 1;
    }

    ipc_send_status("Initializing audio capture...");

    /* Initialize audio capture */
    g_audio = audio_init(on_audio_data, NULL);
    if (!g_audio) {
        fprintf(stderr, "[Main] Failed to initialize audio: %s\n", audio_get_error());
        ipc_send_error("Failed to initialize audio capture");
        whisper_cleanup(g_whisper);
        ipc_cleanup();
        return 1;
    }

    /* Start audio capture */
    if (!audio_start(g_audio)) {
        fprintf(stderr, "[Main] Failed to start audio: %s\n", audio_get_error());
        ipc_send_error("Failed to start audio capture");
        audio_cleanup(g_audio);
        whisper_cleanup(g_whisper);
        ipc_cleanup();
        return 1;
    }

    ipc_send_status("Running - listening for audio...");
    fprintf(stderr, "[Main] Running (press Ctrl+C to stop)\n");

    /* Main loop */
    while (g_running) {
        /* Poll for IPC messages */
        ipc_poll();

        /* Sleep briefly to avoid busy waiting */
        usleep(100000);  /* 100ms */
    }

    /* Cleanup */
    fprintf(stderr, "[Main] Shutting down...\n");
    ipc_send_status("Shutting down...");

    audio_stop(g_audio);
    audio_cleanup(g_audio);
    whisper_cleanup(g_whisper);
    ipc_cleanup();

    fprintf(stderr, "[Main] Goodbye!\n");
    return 0;
}
