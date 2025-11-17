#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static char last_error[256] = {0};

/* =================================================================
 * Platform-specific implementations
 * ================================================================= */

#ifdef PLATFORM_MACOS
/* ===== macOS Core Audio Implementation ===== */
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

#define NUM_BUFFERS 3

struct audio_context {
    AudioQueueRef queue;
    AudioQueueBufferRef buffers[NUM_BUFFERS];
    audio_callback_t callback;
    void *user_data;
    bool running;
    pthread_mutex_t lock;
};

static void audio_input_callback(void *user_data, AudioQueueRef queue,
                                 AudioQueueBufferRef buffer,
                                 const AudioTimeStamp *start_time,
                                 UInt32 num_packets,
                                 const AudioStreamPacketDescription *packet_desc) {
    (void)start_time;
    (void)packet_desc;

    audio_context_t *ctx = (audio_context_t*)user_data;
    if (!ctx || !ctx->running) return;

    /* Convert int16 PCM to float32 */
    int16_t *samples_i16 = (int16_t*)buffer->mAudioData;
    size_t num_samples = buffer->mAudioDataByteSize / sizeof(int16_t);

    float *samples_f32 = malloc(num_samples * sizeof(float));
    if (samples_f32) {
        for (size_t i = 0; i < num_samples; i++) {
            samples_f32[i] = (float)samples_i16[i] / 32768.0f;
        }

        pthread_mutex_lock(&ctx->lock);
        ctx->callback(samples_f32, num_samples, ctx->user_data);
        pthread_mutex_unlock(&ctx->lock);

        free(samples_f32);
    }

    /* Re-enqueue buffer */
    if (ctx->running) {
        AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    }
}

audio_context_t* audio_init(audio_callback_t callback, void *user_data) {
    if (!callback) {
        snprintf(last_error, sizeof(last_error), "Invalid callback");
        return NULL;
    }

    audio_context_t *ctx = calloc(1, sizeof(audio_context_t));
    if (!ctx) {
        snprintf(last_error, sizeof(last_error), "Memory allocation failed");
        return NULL;
    }

    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->running = false;
    pthread_mutex_init(&ctx->lock, NULL);

    /* Set up audio format (16kHz, mono, 16-bit) */
    AudioStreamBasicDescription format = {0};
    format.mSampleRate = AUDIO_SAMPLE_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBitsPerChannel = 16;
    format.mChannelsPerFrame = AUDIO_CHANNELS;
    format.mBytesPerFrame = format.mChannelsPerFrame * (format.mBitsPerChannel / 8);
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;

    /* Create audio queue */
    OSStatus status = AudioQueueNewInput(&format, audio_input_callback, ctx,
                                        NULL, kCFRunLoopCommonModes, 0, &ctx->queue);
    if (status != noErr) {
        snprintf(last_error, sizeof(last_error), "Failed to create audio queue: %d", (int)status);
        free(ctx);
        return NULL;
    }

    /* Allocate and enqueue buffers */
    UInt32 buffer_size = (AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * sizeof(int16_t)) / 10;  /* 100ms buffers */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        status = AudioQueueAllocateBuffer(ctx->queue, buffer_size, &ctx->buffers[i]);
        if (status != noErr) {
            snprintf(last_error, sizeof(last_error), "Failed to allocate buffer: %d", (int)status);
            AudioQueueDispose(ctx->queue, true);
            free(ctx);
            return NULL;
        }
    }

    fprintf(stderr, "[Audio] Initialized (macOS Core Audio)\n");
    return ctx;
}

bool audio_start(audio_context_t *ctx) {
    if (!ctx) return false;

    /* Enqueue all buffers */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        AudioQueueEnqueueBuffer(ctx->queue, ctx->buffers[i], 0, NULL);
    }

    ctx->running = true;
    OSStatus status = AudioQueueStart(ctx->queue, NULL);
    if (status != noErr) {
        snprintf(last_error, sizeof(last_error), "Failed to start audio queue: %d", (int)status);
        ctx->running = false;
        return false;
    }

    fprintf(stderr, "[Audio] Started recording\n");
    return true;
}

void audio_stop(audio_context_t *ctx) {
    if (!ctx) return;

    ctx->running = false;
    AudioQueueStop(ctx->queue, true);
    fprintf(stderr, "[Audio] Stopped recording\n");
}

void audio_cleanup(audio_context_t *ctx) {
    if (!ctx) return;

    audio_stop(ctx);
    AudioQueueDispose(ctx->queue, true);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
    fprintf(stderr, "[Audio] Cleanup complete\n");
}

#elif defined(PLATFORM_LINUX)
/* ===== Linux PulseAudio Implementation ===== */
#include <pulse/simple.h>
#include <pulse/error.h>

struct audio_context {
    pa_simple *pa;
    audio_callback_t callback;
    void *user_data;
    pthread_t thread;
    bool running;
    pthread_mutex_t lock;
};

static void* audio_thread(void *arg) {
    audio_context_t *ctx = (audio_context_t*)arg;
    const size_t buffer_samples = AUDIO_SAMPLE_RATE / 10;  /* 100ms */
    int16_t *buffer_i16 = malloc(buffer_samples * sizeof(int16_t));
    float *buffer_f32 = malloc(buffer_samples * sizeof(float));

    if (!buffer_i16 || !buffer_f32) {
        free(buffer_i16);
        free(buffer_f32);
        return NULL;
    }

    while (ctx->running) {
        int error;
        if (pa_simple_read(ctx->pa, buffer_i16, buffer_samples * sizeof(int16_t), &error) < 0) {
            fprintf(stderr, "[Audio] Read error: %s\n", pa_strerror(error));
            break;
        }

        /* Convert to float32 */
        for (size_t i = 0; i < buffer_samples; i++) {
            buffer_f32[i] = (float)buffer_i16[i] / 32768.0f;
        }

        pthread_mutex_lock(&ctx->lock);
        ctx->callback(buffer_f32, buffer_samples, ctx->user_data);
        pthread_mutex_unlock(&ctx->lock);
    }

    free(buffer_i16);
    free(buffer_f32);
    return NULL;
}

audio_context_t* audio_init(audio_callback_t callback, void *user_data) {
    if (!callback) {
        snprintf(last_error, sizeof(last_error), "Invalid callback");
        return NULL;
    }

    audio_context_t *ctx = calloc(1, sizeof(audio_context_t));
    if (!ctx) {
        snprintf(last_error, sizeof(last_error), "Memory allocation failed");
        return NULL;
    }

    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->running = false;
    pthread_mutex_init(&ctx->lock, NULL);

    /* PulseAudio sample spec */
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = AUDIO_SAMPLE_RATE,
        .channels = AUDIO_CHANNELS
    };

    int error;
    ctx->pa = pa_simple_new(NULL, "VisualIA", PA_STREAM_RECORD, NULL,
                           "Audio Capture", &ss, NULL, NULL, &error);
    if (!ctx->pa) {
        snprintf(last_error, sizeof(last_error), "PulseAudio init failed: %s", pa_strerror(error));
        free(ctx);
        return NULL;
    }

    fprintf(stderr, "[Audio] Initialized (Linux PulseAudio)\n");
    return ctx;
}

bool audio_start(audio_context_t *ctx) {
    if (!ctx) return false;

    ctx->running = true;
    if (pthread_create(&ctx->thread, NULL, audio_thread, ctx) != 0) {
        snprintf(last_error, sizeof(last_error), "Failed to create thread");
        ctx->running = false;
        return false;
    }

    fprintf(stderr, "[Audio] Started recording\n");
    return true;
}

void audio_stop(audio_context_t *ctx) {
    if (!ctx) return;

    ctx->running = false;
    pthread_join(ctx->thread, NULL);
    fprintf(stderr, "[Audio] Stopped recording\n");
}

void audio_cleanup(audio_context_t *ctx) {
    if (!ctx) return;

    audio_stop(ctx);
    if (ctx->pa) {
        pa_simple_free(ctx->pa);
    }
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
    fprintf(stderr, "[Audio] Cleanup complete\n");
}

#elif defined(PLATFORM_WINDOWS)
/* ===== Windows WASAPI Implementation ===== */
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

struct audio_context {
    IMMDeviceEnumerator *enumerator;
    IMMDevice *device;
    IAudioClient *audio_client;
    IAudioCaptureClient *capture_client;
    audio_callback_t callback;
    void *user_data;
    HANDLE thread;
    HANDLE stop_event;
    bool running;
    pthread_mutex_t lock;
};

static DWORD WINAPI audio_thread(LPVOID arg) {
    audio_context_t *ctx = (audio_context_t*)arg;
    CoInitialize(NULL);

    const DWORD buffer_samples = AUDIO_SAMPLE_RATE / 10;  /* 100ms */
    float *buffer_f32 = malloc(buffer_samples * sizeof(float));

    while (WaitForSingleObject(ctx->stop_event, 0) == WAIT_TIMEOUT) {
        UINT32 packet_length = 0;
        ctx->capture_client->lpVtbl->GetNextPacketSize(ctx->capture_client, &packet_length);

        while (packet_length != 0) {
            BYTE *data;
            UINT32 num_frames;
            DWORD flags;

            HRESULT hr = ctx->capture_client->lpVtbl->GetBuffer(ctx->capture_client, &data,
                                                                &num_frames, &flags, NULL, NULL);
            if (SUCCEEDED(hr)) {
                int16_t *samples_i16 = (int16_t*)data;
                for (UINT32 i = 0; i < num_frames; i++) {
                    buffer_f32[i] = (float)samples_i16[i] / 32768.0f;
                }

                pthread_mutex_lock(&ctx->lock);
                ctx->callback(buffer_f32, num_frames, ctx->user_data);
                pthread_mutex_unlock(&ctx->lock);

                ctx->capture_client->lpVtbl->ReleaseBuffer(ctx->capture_client, num_frames);
            }

            ctx->capture_client->lpVtbl->GetNextPacketSize(ctx->capture_client, &packet_length);
        }

        Sleep(10);
    }

    free(buffer_f32);
    CoUninitialize();
    return 0;
}

audio_context_t* audio_init(audio_callback_t callback, void *user_data) {
    /* WASAPI implementation - simplified for brevity */
    snprintf(last_error, sizeof(last_error), "Windows WASAPI not fully implemented yet");
    return NULL;
}

bool audio_start(audio_context_t *ctx) {
    return false;
}

void audio_stop(audio_context_t *ctx) {
}

void audio_cleanup(audio_context_t *ctx) {
}

#else
#error "Unsupported platform"
#endif

const char* audio_get_error(void) {
    return last_error;
}
