const { ipcRenderer } = require('electron');

// DOM elements
const subtitleBox = document.getElementById('subtitle-box');
const originalText = document.getElementById('original-text');
const translationText = document.getElementById('translation-text');
const translationRow = document.getElementById('translation-row');
const statusElement = document.getElementById('status');
const statusText = document.getElementById('status-text');

// Control elements
const modelSelect = document.getElementById('model-select');
const sourceLangSelect = document.getElementById('source-lang-select');
const targetLangSelect = document.getElementById('target-lang-select');
const translationToggle = document.getElementById('translation-toggle');
const targetLangGroup = document.getElementById('target-lang-group');
const controls = document.getElementById('controls');

// State
let hideTimeout = null;
let currentSettings = {
    model: 'base',
    sourceLang: 'auto',
    targetLang: 'en',
    translationEnabled: false
};

// Translation cache
const translationCache = new Map();

// Load saved settings
function loadSettings() {
    const saved = localStorage.getItem('visualia-settings');
    if (saved) {
        try {
            currentSettings = JSON.parse(saved);
            modelSelect.value = currentSettings.model;
            sourceLangSelect.value = currentSettings.sourceLang;
            targetLangSelect.value = currentSettings.targetLang;
            translationToggle.checked = currentSettings.translationEnabled || false;

            // Show/hide target language dropdown based on toggle
            targetLangGroup.style.display = translationToggle.checked ? 'flex' : 'none';
        } catch (e) {
            console.error('[Renderer] Failed to load settings:', e);
        }
    }
}

// Save settings
function saveSettings() {
    localStorage.setItem('visualia-settings', JSON.stringify(currentSettings));
}

// Translate text using a local/free translation service
async function translateText(text, targetLang) {
    if (!text || targetLang === 'none') return null;

    // Check cache
    const cacheKey = `${text}:${targetLang}`;
    if (translationCache.has(cacheKey)) {
        return translationCache.get(cacheKey);
    }

    try {
        // Using LibreTranslate API (free, self-hostable)
        // You can also use Google Translate, DeepL, or run local models
        const response = await fetch('https://libretranslate.com/translate', {
            method: 'POST',
            body: JSON.stringify({
                q: text,
                source: 'auto',
                target: targetLang,
                format: 'text'
            }),
            headers: { 'Content-Type': 'application/json' }
        });

        if (!response.ok) {
            throw new Error(`Translation API error: ${response.status}`);
        }

        const data = await response.json();
        const translation = data.translatedText;

        // Cache the result
        translationCache.set(cacheKey, translation);

        // Limit cache size
        if (translationCache.size > 100) {
            const firstKey = translationCache.keys().next().value;
            translationCache.delete(firstKey);
        }

        return translation;
    } catch (error) {
        console.error('[Renderer] Translation error:', error);
        return null;
    }
}

// Update subtitle display
async function showSubtitle(text) {
    originalText.textContent = text;
    subtitleBox.classList.remove('hidden');
    subtitleBox.classList.add('fade-in');

    // Handle translation
    if (currentSettings.translationEnabled && currentSettings.targetLang !== 'none') {
        translationRow.style.display = 'flex';
        translationText.textContent = 'Translating...';

        const translated = await translateText(text, currentSettings.targetLang);
        if (translated) {
            translationText.textContent = translated;
        } else {
            translationText.textContent = '[Translation unavailable]';
        }
    } else {
        translationRow.style.display = 'none';
    }

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

// Handle model change
modelSelect.addEventListener('change', (e) => {
    currentSettings.model = e.target.value;
    saveSettings();
    updateStatus(`Switching to ${e.target.options[e.target.selectedIndex].text}...`);

    // Send message to main process to restart backend with new model
    ipcRenderer.send('change-model', currentSettings.model);
});

// Handle source language change
sourceLangSelect.addEventListener('change', (e) => {
    currentSettings.sourceLang = e.target.value;
    saveSettings();
    updateStatus(`Source language: ${e.target.options[e.target.selectedIndex].text}`);

    // Send message to main process to restart backend with new language
    ipcRenderer.send('change-source-lang', currentSettings.sourceLang);
});

// Handle translation toggle
translationToggle.addEventListener('change', (e) => {
    currentSettings.translationEnabled = e.target.checked;
    saveSettings();

    if (e.target.checked) {
        targetLangGroup.style.display = 'flex';
        updateStatus(`Translation enabled - ${targetLangSelect.options[targetLangSelect.selectedIndex].text}`);
    } else {
        targetLangGroup.style.display = 'none';
        translationRow.style.display = 'none';
        updateStatus('Translation disabled');
    }
});

// Handle target language change
targetLangSelect.addEventListener('change', (e) => {
    currentSettings.targetLang = e.target.value;
    saveSettings();
    updateStatus(`Translating to ${e.target.options[e.target.selectedIndex].text}`);
});

// Fix click-through: Enable mouse events when hovering over controls
controls.addEventListener('mouseenter', () => {
    ipcRenderer.send('toggle-click-through', false);
});

controls.addEventListener('mouseleave', () => {
    ipcRenderer.send('toggle-click-through', true);
});

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
loadSettings();
updateStatus('Connecting to backend...');
