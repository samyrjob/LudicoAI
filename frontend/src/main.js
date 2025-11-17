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
    const modelPath = path.join(__dirname, '../../models/whisper-base.en.gguf');

    console.log('[Electron] Starting backend:', backendPath);

    backendProcess = spawn(backendPath, ['-m', modelPath], {
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
