const { ipcRenderer } = require('electron');

// DOM elements
const subtitleBox = document.getElementById('subtitle-box');
const subtitleText = document.getElementById('subtitle-text');
const statusElement = document.getElementById('status');
const statusText = document.getElementById('status-text');

// State
let hideTimeout = null;

// Update subtitle display
function showSubtitle(text) {
    subtitleText.textContent = text;
    subtitleBox.classList.remove('hidden');
    subtitleBox.classList.add('fade-in');

    // Clear existing timeout
    if (hideTimeout) {
        clearTimeout(hideTimeout);
    }

    // Hide after 5 seconds of no new text
    hideTimeout = setTimeout(() => {
        subtitleBox.classList.add('hidden');
    }, 5000);
}

// Update status
function updateStatus(message, isError = false) {
    statusText.textContent = message;
    if (isError) {
        statusElement.classList.add('error');
    } else {
        statusElement.classList.remove('error');
    }
}

// Handle transcription from backend
ipcRenderer.on('transcription', (event, data) => {
    console.log('[Renderer] Transcription:', data.text);
    showSubtitle(data.text);
});

// Handle status updates
ipcRenderer.on('status', (event, data) => {
    console.log('[Renderer] Status:', data.message);
    updateStatus(data.message);
});

// Handle errors
ipcRenderer.on('error', (event, data) => {
    console.error('[Renderer] Error:', data.message);
    updateStatus(data.message, true);
});

// Initialize
console.log('[Renderer] VisualIA overlay ready');
updateStatus('Connecting to backend...');

// Keyboard shortcuts (for testing)
document.addEventListener('keydown', (e) => {
    // Toggle click-through with Ctrl+Shift+C
    if (e.ctrlKey && e.shiftKey && e.key === 'C') {
        const clickThrough = !subtitleBox.classList.contains('click-through');
        ipcRenderer.send('toggle-click-through', clickThrough);
        console.log('[Renderer] Click-through:', clickThrough);
    }
});
