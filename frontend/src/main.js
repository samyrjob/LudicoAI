const { app, BrowserWindow, ipcMain, screen } = require('electron');
const path = require('path');
const { spawn } = require('child_process');
const BackendIPC = require('./backend-ipc');

let mainWindow = null;
let backendProcess = null;
let backendIPC = null;

function createWindow() {
    const { width, height } = screen.getPrimaryDisplay().workAreaSize;

    mainWindow = new BrowserWindow({
        width: width,
        height: 150,  // Height for subtitle bar
        x: 0,
        y: height - 150,  // Position at bottom of screen
        transparent: true,
        frame: false,
        alwaysOnTop: true,
        skipTaskbar: true,
        resizable: false,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    // Make window click-through (non-interactive)
    mainWindow.setIgnoreMouseEvents(true);

    // Load the overlay UI
    mainWindow.loadFile(path.join(__dirname, 'overlay.html'));

    // Open DevTools in development
    if (process.env.NODE_ENV === 'development') {
        mainWindow.webContents.openDevTools({ mode: 'detach' });
    }

    mainWindow.on('closed', () => {
        mainWindow = null;
    });

    console.log('[Electron] Window created');
}

function startBackend() {
    // Path to the C backend executable
    const backendPath = path.join(__dirname, '../../build/visualia');
    const modelFile = process.env.VISUALIA_MODEL || 'whisper-base.gguf';
    const modelPath = path.join(__dirname, '../../models', modelFile);
    const language = process.env.VISUALIA_LANG || 'auto';  // Default to auto-detect

    console.log('[Electron] Starting backend:', backendPath);
    console.log('[Electron] Model:', modelPath);
    console.log('[Electron] Language:', language);

    backendProcess = spawn(backendPath, ['-m', modelPath, '-l', language], {
        stdio: ['pipe', 'pipe', 'pipe']
    });

    // Initialize IPC handler
    backendIPC = new BackendIPC(backendProcess);

    // Handle messages from backend
    backendIPC.on('transcription', (data) => {
        if (mainWindow) {
            mainWindow.webContents.send('transcription', data);
        }
    });

    backendIPC.on('status', (data) => {
        console.log('[Backend Status]', data.message);
        if (mainWindow) {
            mainWindow.webContents.send('status', data);
        }
    });

    backendIPC.on('error', (data) => {
        console.error('[Backend Error]', data.message);
        if (mainWindow) {
            mainWindow.webContents.send('error', data);
        }
    });

    // Handle backend process exit
    backendProcess.on('exit', (code) => {
        console.log(`[Electron] Backend exited with code ${code}`);
        backendIPC = null;
        backendProcess = null;
    });

    // Log backend stderr
    backendProcess.stderr.on('data', (data) => {
        console.log('[Backend]', data.toString().trim());
    });
}

function stopBackend() {
    if (backendProcess) {
        console.log('[Electron] Stopping backend...');
        backendProcess.kill('SIGTERM');
        backendProcess = null;
    }
    if (backendIPC) {
        backendIPC.cleanup();
        backendIPC = null;
    }
}

// App lifecycle
app.whenReady().then(() => {
    createWindow();
    startBackend();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on('window-all-closed', () => {
    stopBackend();
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('before-quit', () => {
    stopBackend();
});

// IPC handlers
ipcMain.on('toggle-click-through', (event, enabled) => {
    if (mainWindow) {
        mainWindow.setIgnoreMouseEvents(enabled);
    }
});

// Handle model change requests
ipcMain.on('change-model', (event, model) => {
    console.log('[Electron] Changing model to:', model);
    stopBackend();

    // Update model path based on selection
    const modelFiles = {
        'base': 'whisper-base.gguf',
        'small': 'whisper-small.gguf',
        'medium': 'whisper-medium.gguf',
        'large-v3': 'whisper-large-v3.gguf'
    };

    process.env.VISUALIA_MODEL = modelFiles[model] || 'whisper-base.gguf';

    setTimeout(() => {
        startBackend();
    }, 1000);
});

// Handle source language change requests
ipcMain.on('change-source-lang', (event, lang) => {
    console.log('[Electron] Changing source language to:', lang);
    stopBackend();

    process.env.VISUALIA_LANG = lang;

    setTimeout(() => {
        startBackend();
    }, 1000);
});
