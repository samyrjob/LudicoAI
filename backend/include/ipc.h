#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

/* IPC message types */
typedef enum {
    IPC_MSG_TRANSCRIPTION,
    IPC_MSG_ERROR,
    IPC_MSG_STATUS,
    IPC_MSG_CONTROL
} ipc_message_type_t;

/**
 * Initialize IPC system (JSON-RPC over stdio)
 * @return true on success, false on failure
 */
bool ipc_init(void);

/**
 * Send transcription result to frontend
 * @param text Transcribed text
 * @param timestamp Unix timestamp
 * @return true on success, false on failure
 */
bool ipc_send_transcription(const char *text, long timestamp);

/**
 * Send error message to frontend
 * @param error_msg Error message
 * @return true on success, false on failure
 */
bool ipc_send_error(const char *error_msg);

/**
 * Send status update to frontend
 * @param status Status message
 * @return true on success, false on failure
 */
bool ipc_send_status(const char *status);

/**
 * Check for incoming messages from frontend (non-blocking)
 * @return true if message received and handled, false otherwise
 */
bool ipc_poll(void);

/**
 * Cleanup IPC resources
 */
void ipc_cleanup(void);

#endif /* IPC_H */
