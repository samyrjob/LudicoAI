const { ipcRenderer } = require('electron');

// DOM elements
const subtitleBox = document.getElementById('subtitle-box');
const originalText = document.getElementById('original-text');
const translationText = document.getElementById('translation-text');
const translationRow = document.getElementById('translation-row');
const statusElement = document.getElementById('status');
const statusText = document.getElementById('status-text');
const statusDot = document.getElementById('status-dot');
const subtitleContainer = document.getElementById('subtitle-container');

// Control elements
const modelSelect = document.getElementById('model-select');
const sourceLangSelect = document.getElementById('source-lang-select');
const targetLangSelect = document.getElementById('target-lang-select');
const translationToggle = document.getElementById('translation-toggle');
const targetLangGroup = document.getElementById('target-lang-group');
const translationModelSelect = document.getElementById('translation-model-select');
const translationModelGroup = document.getElementById('translation-model-group');
const captionHistoryToggle = document.getElementById('caption-history-toggle');
const soundCaptionsToggle = document.getElementById('sound-captions-toggle');
const controls = document.getElementById('controls');
const closeSettingsBtn = document.getElementById('close-settings');

// Helper function to get smart default target language
function getSmartTargetLang(sourceLang = 'auto') {
    // Get system locale (e.g., 'en-US', 'fr-FR')
    const systemLang = navigator.language.split('-')[0]; // Get 'en' from 'en-US'

    // If source is English or system is English, choose a common alternative
    if (sourceLang === 'en' || systemLang === 'en') {
        return systemLang !== 'en' ? systemLang : 'fr'; // Use system lang if not English, else French
    }

    // If source is known and not English, translate to English
    if (sourceLang !== 'auto') {
        return 'en';
    }

    // Default: if auto-detect, prefer English unless system is already English
    return systemLang !== 'en' ? 'en' : 'fr';
}

// State
let hideTimeout = null;
let currentSettings = {
    model: 'base',
    sourceLang: 'auto',
    targetLang: getSmartTargetLang('auto'),
    translationEnabled: false,
    captionHistory: false,
    translationModel: 'madlad400-10b-mt',
    showSoundCaptions: true

};

// Caption history state
const MAX_CAPTION_HISTORY = 4; // Maximum number of caption bubbles to show
let captionHistory = []; // Array of caption objects {original, translation}

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

            translationModelSelect.value = currentSettings.translationModel || 'madlad400-10b-mt';
            soundCaptionsToggle.checked = currentSettings.showSoundCaptions !== false;
            captionHistoryToggle.checked = currentSettings.captionHistory || false;

            // Show/hide target language dropdown based on toggle
            targetLangGroup.style.display = translationToggle.checked ? 'flex' : 'none';
            translationModelGroup.style.display = translationToggle.checked ? 'flex' : 'none';
        } catch (e) {
            console.error('[Renderer] Failed to load settings:', e);
        }
    }
}

// Save settings
function saveSettings() {
    localStorage.setItem('visualia-settings', JSON.stringify(currentSettings));
}

// Translation is now handled by the backend via T5 model
// Translations will arrive via IPC 'translation' events
// This function is kept for cache management only
function getCachedTranslation(text, targetLang) {
    const cacheKey = `${text}:${targetLang}`;
    return translationCache.get(cacheKey);
}

function cacheTranslation(text, targetLang, translation) {
    const cacheKey = `${text}:${targetLang}`;
    translationCache.set(cacheKey, translation);

    // Limit cache size
    if (translationCache.size > 100) {
        const firstKey = translationCache.keys().next().value;
        translationCache.delete(firstKey);
    }
}

// Caption history functions
function addCaptionToBubbleHistory(text, translation = null) {
    // Add new caption to the beginning of the array
    captionHistory.unshift({ original: text, translation: translation });

    // Remove oldest if exceeding limit
    if (captionHistory.length > MAX_CAPTION_HISTORY) {
        captionHistory.pop();
    }

    // Render all bubbles
    renderCaptionBubbles();
}

function renderCaptionBubbles() {
    // Get existing bubbles
    const existingBubbles = Array.from(subtitleContainer.querySelectorAll('.subtitle-bubble'));

    // Reverse iteration: oldest to newest (for horizontal left-to-right display)
    const reversedHistory = [...captionHistory].reverse();

    // Update or create bubbles for each caption
    reversedHistory.forEach((caption, displayIndex) => {
        // Original index in captionHistory (for age calculation)
        const historyIndex = captionHistory.length - 1 - displayIndex;

        let bubble = existingBubbles[displayIndex];

        if (!bubble) {
            // Create new bubble
            bubble = createCaptionBubble(caption, historyIndex);

            // Add to DOM - no animations, just appears
            subtitleContainer.appendChild(bubble);
        } else {
            // Update existing bubble
            updateCaptionBubble(bubble, caption, historyIndex);
        }

        // Apply aging classes to all bubbles
        bubble.classList.remove('newest', 'older-1', 'older-2', 'older-3');
        if (historyIndex === 0) {
            bubble.classList.add('newest');
        } else {
            bubble.classList.add(`older-${Math.min(historyIndex, 3)}`);
        }
    });

    // Center the newest bubble by shifting container by half its width
    const newestBubble = subtitleContainer.querySelector('.subtitle-bubble.newest');
    if (newestBubble) {
        const bubbleWidth = newestBubble.offsetWidth;
        subtitleContainer.style.transform = `translateX(${bubbleWidth / 2}px)`;
    }

    // Remove excess bubbles immediately
    for (let i = reversedHistory.length; i < existingBubbles.length; i++) {
        const bubbleToRemove = existingBubbles[i];
        if (bubbleToRemove.parentNode) {
            bubbleToRemove.parentNode.removeChild(bubbleToRemove);
        }
    }

    // Auto-hide oldest bubbles after timeout
    if (hideTimeout) {
        clearTimeout(hideTimeout);
    }

    hideTimeout = setTimeout(() => {
        // Clear all bubbles immediately
        captionHistory = [];
        subtitleContainer.innerHTML = '';
        subtitleContainer.style.transform = '';
    }, 5000);
}

function createCaptionBubble(caption, index) {
    const bubble = document.createElement('div');
    bubble.className = 'subtitle-bubble';
    bubble.dataset.original = caption.original;

    // Create content structure
    const contentDiv = document.createElement('div');
    contentDiv.style.display = 'flex';
    contentDiv.style.flexDirection = 'column';
    contentDiv.style.gap = '12px';

    // Original text row (no label)
    const originalRow = document.createElement('div');
    const originalTextDiv = document.createElement('div');
    originalTextDiv.textContent = caption.original;
    originalTextDiv.className = 'subtitle-text original-caption-text';
    originalRow.appendChild(originalTextDiv);
    contentDiv.appendChild(originalRow);

    // Translation row (if available and enabled)
    if (currentSettings.translationEnabled) {
        const translationRow = document.createElement('div');
        translationRow.className = 'translation-caption-row';
        translationRow.style.borderTop = '1px solid rgba(255, 255, 255, 0.1)';
        translationRow.style.paddingTop = '12px';
        translationRow.style.display = caption.translation ? 'block' : 'none';

        const translationLabel = document.createElement('span');
        translationLabel.textContent = 'Translation';
        translationLabel.className = 'subtitle-label';

        const translationTextDiv = document.createElement('div');
        translationTextDiv.textContent = caption.translation || 'Translating...';
        translationTextDiv.className = 'subtitle-text translation-caption-text';
        translationTextDiv.style.color = '#4ade80';

        translationRow.appendChild(translationLabel);
        translationRow.appendChild(translationTextDiv);
        contentDiv.appendChild(translationRow);
    }

    bubble.appendChild(contentDiv);
    return bubble;
}

function updateCaptionBubble(bubble, caption, index) {
    // Update text content
    const originalTextDiv = bubble.querySelector('.original-caption-text');
    if (originalTextDiv) {
        originalTextDiv.textContent = caption.original;
    }

    // Update translation if exists
    if (currentSettings.translationEnabled) {
        const translationRow = bubble.querySelector('.translation-caption-row');
        const translationTextDiv = bubble.querySelector('.translation-caption-text');

        if (translationRow && translationTextDiv) {
            if (caption.translation) {
                translationRow.style.display = 'block';
                translationTextDiv.textContent = caption.translation;
            } else {
                translationRow.style.display = 'none';
            }
        }
    }
}

function updateCaptionTranslation(originalText, translation) {
    // Find the caption in history and update its translation
    const caption = captionHistory.find(c => c.original === originalText);
    if (caption) {
        caption.translation = translation;
        renderCaptionBubbles();
    }
}

// Update subtitle display
function showSubtitle(text) {
    if (currentSettings.captionHistory) {
        // Multi-bubble mode
        addCaptionToBubbleHistory(text);
        subtitleBox.classList.add('hidden');
    } else {
        // Single bubble mode (legacy)
        subtitleContainer.innerHTML = '';
        originalText.textContent = text;
        subtitleBox.classList.remove('hidden');
        subtitleBox.classList.add('fade-in');

        // Show translation row if enabled (translation will come via IPC)
        if (currentSettings.translationEnabled && currentSettings.targetLang !== 'none') {
            translationRow.style.display = 'flex';
            translationText.textContent = 'Translating...';
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
}

function updateStatus(message, isError = false) {
    // Skip if currently showing hint
    if (isShowingHint) {
        currentStatusMessage = message;
        return;
    }

    // Fade out
    statusText.classList.add('fade-out');

    setTimeout(() => {
        statusText.textContent = message;
        if (isError) {
            statusElement.classList.add('error');
        } else {
            statusElement.classList.remove('error');
        }

        // Fade in
        statusText.classList.remove('fade-out');
    }, 200);
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
        translationModelGroup.style.display = 'flex';
        updateStatus(`Translation enabled - ${targetLangSelect.options[targetLangSelect.selectedIndex].text}`);

        // Enable translation in backend
        ipcRenderer.send('change-translation', {
            enabled: true,
            targetLang: currentSettings.targetLang,
            translationModel: currentSettings.translationModel
        });
    } else {
        targetLangGroup.style.display = 'none';
        translationModelGroup.style.display = 'none';
        translationRow.style.display = 'none';
        updateStatus('Translation disabled');

        // Disable translation in backend
        ipcRenderer.send('change-translation', {
            enabled: false,
            targetLang: null,
            translationModel: null
        });
    }
});

// Handle caption history toggle
captionHistoryToggle.addEventListener('change', (e) => {
    currentSettings.captionHistory = e.target.checked;
    saveSettings();

    if (e.target.checked) {
        updateStatus('Caption history enabled');
        // Clear old single bubble
        subtitleBox.classList.add('hidden');
    } else {
        updateStatus('Caption history disabled');
        // Clear caption history bubbles
        captionHistory = [];
        subtitleContainer.innerHTML = '';
    }
});

// Handle target language change
targetLangSelect.addEventListener('change', (e) => {
    const newTargetLang = e.target.value;

    // Prevent same-language translation
    if (newTargetLang === currentSettings.sourceLang && currentSettings.sourceLang !== 'auto') {
        updateStatus(`Warning: Source and target are both ${currentSettings.sourceLang.toUpperCase()}`, true);
        console.warn('[Renderer] Same-language translation selected:', currentSettings.sourceLang);
    }

    currentSettings.targetLang = newTargetLang;
    saveSettings();
    updateStatus(`Translating to ${e.target.options[e.target.selectedIndex].text}`);

    // Update translation target in backend if translation is enabled
    if (currentSettings.translationEnabled) {
        ipcRenderer.send('change-translation', {
            enabled: true,
            targetLang: currentSettings.targetLang,
            translationModel: currentSettings.translationModel
        });
    }
});

// Handle translation model change
translationModelSelect.addEventListener('change', (e) => {
    currentSettings.translationModel = e.target.value;
    saveSettings();
    updateStatus(`Switching to ${e.target.options[e.target.selectedIndex].text}...`);

    // Update translation model in backend if translation is enabled
    if (currentSettings.translationEnabled) {
        ipcRenderer.send('change-translation', {
            enabled: true,
            targetLang: currentSettings.targetLang,
            translationModel: currentSettings.translationModel
        });
    }
});

// Handle sound captions toggle
soundCaptionsToggle.addEventListener('change', (e) => {
    currentSettings.showSoundCaptions = e.target.checked;
    saveSettings();
    updateStatus(e.target.checked ? 'Sound captions enabled' : 'Sound captions disabled');
});

// Close settings button
closeSettingsBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    controls.classList.add('hidden');
});

// Status element reveals settings on hover
statusElement.addEventListener('mouseenter', () => {
    controls.classList.remove('hidden');
    controls.classList.add('show');
    // Disable click-through so controls are interactive on Windows
    ipcRenderer.send('toggle-click-through', false);
});

statusElement.addEventListener('mouseleave', (e) => {
    // Small delay to allow moving to controls
    setTimeout(() => {
        // If not hovering over controls, hide them and re-enable click-through
        if (!controls.matches(':hover')) {
            controls.classList.remove('show');
            ipcRenderer.send('toggle-click-through', true);
        }
    }, 100);
});

// Controls stay visible when hovering over them
controls.addEventListener('mouseenter', () => {
    controls.classList.remove('hidden');
    controls.classList.add('show');
    ipcRenderer.send('toggle-click-through', false);
});

controls.addEventListener('mouseleave', () => {
    controls.classList.remove('show');
    ipcRenderer.send('toggle-click-through', true);
});

// Handle transcription from backend
ipcRenderer.on('transcription', (event, data) => {
    console.log('[Renderer] Transcription:', data.text);

    // Check if text is a sound caption (text in brackets, asterisks, or parentheses like [silence], *footsteps*, (door closing), etc.)
    const isSoundCaption = /^(\[.*\]|\*.*\*|\(.*\))$/.test(data.text.trim());

    // Don't show UI if it's a sound caption and the setting is disabled
    if (isSoundCaption && !currentSettings.showSoundCaptions) {
        return;
    }

    showSubtitle(data.text);
});

// Handle translation from backend (T5 model)
ipcRenderer.on('translation', (event, data) => {
    console.log('[Renderer] Translation:', data.original, '→', data.text);

    if (currentSettings.captionHistory) {
        // Update translation in caption history
        updateCaptionTranslation(data.original, data.text);
    } else {
        // Update translation display in single bubble mode
        if (translationRow.style.display === 'flex') {
            translationText.textContent = data.text;

            // Cache the translation
            if (currentSettings.targetLang) {
                cacheTranslation(data.original, currentSettings.targetLang, data.text);
            }
        }
    }
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

// Handle language detection
ipcRenderer.on('language_detected', (event, data) => {
    console.log('[Renderer] Language detected:', data.language);

    const detectedLang = data.language;

    // Update source language in UI (switch from auto to detected)
    currentSettings.sourceLang = detectedLang;
    sourceLangSelect.value = detectedLang;

    // Intelligently set target language based on detected source
    const newTargetLang = getSmartTargetLang(detectedLang);

    // Only update target if it would result in same-language translation
    if (currentSettings.targetLang === detectedLang) {
        currentSettings.targetLang = newTargetLang;
        targetLangSelect.value = newTargetLang;
        console.log('[Renderer] Adjusted target language to avoid same-language translation:', newTargetLang);
    } else if (currentSettings.targetLang === 'none' || !currentSettings.targetLang) {
        // If target was "none", set it to smart default
        currentSettings.targetLang = newTargetLang;
        targetLangSelect.value = newTargetLang;
    }

    saveSettings();

    updateStatus(`Detected: ${detectedLang.toUpperCase()} → Target: ${currentSettings.targetLang.toUpperCase()}`);
});

// Track the original status message
let currentStatusMessage = '';
let isShowingHint = false;

// Periodic hint to show users they can hover for settings
function showStatusHint() {
    // Only show if controls are not already visible
    if (!controls.classList.contains('show') && !isShowingHint) {
        isShowingHint = true;
        currentStatusMessage = statusText.textContent;

        // Fade out current status
        statusText.classList.add('fade-out');

        setTimeout(() => {
            // Hide status dot and change to hint mode
            statusDot.classList.add('hidden');
            statusElement.classList.add('hint-mode');
            statusText.textContent = 'Hover here for settings';

            // Fade in hint
            statusText.classList.remove('fade-out');

            // After 3 seconds, restore original status
            setTimeout(() => {
                // Fade out hint
                statusText.classList.add('fade-out');

                setTimeout(() => {
                    // Show status dot and remove hint mode
                    statusDot.classList.remove('hidden');
                    statusElement.classList.remove('hint-mode');
                    statusText.textContent = currentStatusMessage;

                    // Fade in original status
                    statusText.classList.remove('fade-out');
                    isShowingHint = false;
                }, 200);
            }, 3000); // Show hint for 3 seconds
        }, 200);
    }
}

// Show hint every 40 seconds
setInterval(showStatusHint, 40000);

// Initialize
console.log('[Renderer] VisualIA overlay ready');
loadSettings();
updateStatus('Connecting to backend...');
