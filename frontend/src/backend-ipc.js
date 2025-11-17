const { EventEmitter } = require('events');

/**
 * Handles IPC communication with the C backend via JSON-RPC over stdio
 */
class BackendIPC extends EventEmitter {
    constructor(process) {
        super();
        this.process = process;
        this.buffer = '';

        // Read JSON messages from stdout
        this.process.stdout.on('data', (data) => {
            this.handleData(data.toString());
        });
    }

    handleData(data) {
        this.buffer += data;

        // Process complete lines (JSON messages)
        let newlineIndex;
        while ((newlineIndex = this.buffer.indexOf('\n')) !== -1) {
            const line = this.buffer.substring(0, newlineIndex).trim();
            this.buffer = this.buffer.substring(newlineIndex + 1);

            if (line.length > 0) {
                try {
                    const message = JSON.parse(line);
                    this.handleMessage(message);
                } catch (err) {
                    console.error('[BackendIPC] Failed to parse JSON:', err, line);
                }
            }
        }
    }

    handleMessage(message) {
        if (!message.type) {
            console.error('[BackendIPC] Message missing type:', message);
            return;
        }

        // Emit event based on message type
        this.emit(message.type, message.data || {});
    }

    send(message) {
        if (this.process && this.process.stdin) {
            this.process.stdin.write(JSON.stringify(message) + '\n');
        }
    }

    cleanup() {
        if (this.process) {
            this.process.stdout.removeAllListeners();
            this.process.stderr.removeAllListeners();
        }
    }
}

module.exports = BackendIPC;
