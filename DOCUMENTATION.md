# VisualIA - Complete Documentation

Real-time AI assistant with live speech transcription, translation, and overlay display.

---

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [Architecture](#architecture)
4. [Codebase Structure](#codebase-structure)
5. [Component Interactions](#component-interactions)
6. [Installation & Setup](#installation--setup)
7. [Usage Guide](#usage-guide)
8. [Language Support](#language-support)
9. [Translation System](#translation-system)
10. [Development Guide](#development-guide)
11. [API Reference](#api-reference)
12. [Troubleshooting](#troubleshooting)

---

## Overview

VisualIA is a cross-platform desktop application that provides real-time speech-to-text transcription with translation capabilities. It uses a C-based backend for high-performance audio processing and an Electron frontend for the user interface.

### Key Technologies
- **Backend**: C with whisper.cpp (STT) and llama.cpp (Translation)
- **Frontend**: Electron (Node.js)
- **Communication**: JSON-RPC over stdio
- **Models**: GGUF format (Whisper for STT, mT5 for translation)

---

## Features

### ✅ Implemented
- **Live Speech-to-Text**: Real-time transcription using Whisper models
- **99 Languages**: Automatic language detection or manual selection
- **Live Translation**: T5-based translation with 15+ target languages
- **Overlay Display**: Transparent, always-on-top subtitle window
- **Multiple Models**: Switch between base/small/medium/large Whisper models
- **GPU Acceleration**: Metal support for Apple Silicon
- **Cross-platform Audio**: macOS (CoreAudio), Linux (PulseAudio), Windows (WASAPI)

### 🚧 Planned
- **Screen Understanding**: Visual context with LLaVA
- **Context-Aware Info**: Definitions, pronunciation guides
- **Multi-modal Input**: Screen + audio processing

---

## Architecture

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      USER INTERFACE                          │
│                   (Electron Overlay)                         │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Controls: Model | Source Lang | Translation Toggle  │  │
│  │  Subtitle Box: Original + Translation                 │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────┬────────────────────────────────────────┘
                     │ IPC (JSON-RPC over stdio)
                     ↓
┌─────────────────────────────────────────────────────────────┐
│                     C BACKEND                                │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │   Audio     │→ │   Whisper    │→ │   Translation   │   │
│  │   Capture   │  │   Engine     │  │   Engine (T5)   │   │
│  └─────────────┘  └──────────────┘  └─────────────────┘   │
│         ↓                ↓                    ↓             │
│      CoreAudio      whisper.cpp         llama.cpp          │
│     PulseAudio      (GGUF model)      (mT5 GGUF model)     │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow

```
1. Audio Device
       ↓
2. Platform Audio API (CoreAudio/PulseAudio)
       ↓
3. Audio Buffer (3s chunks, 1s overlap)
       ↓
4. Whisper Engine (whisper.cpp)
       ↓
5. Transcription Result
       ↓
6. Translation Engine (T5 + llama.cpp) [if enabled]
       ↓
7. IPC Message (JSON over stdout)
       ↓
8. Electron IPC Handler
       ↓
9. Renderer Process
       ↓
10. UI Update (Subtitle Display)
```

---

## Codebase Structure

```
VisualIA/
├── backend/                      # C backend
│   ├── include/                  # Header files
│   │   ├── audio.h              # Audio capture API
│   │   ├── whisper_engine.h     # Whisper STT wrapper
│   │   ├── translation_engine.h # T5 translation wrapper
│   │   └── ipc.h                # IPC communication
│   ├── src/                      # Implementation files
│   │   ├── main.c               # Entry point, main loop, signal handling
│   │   ├── audio.c              # Platform-specific audio capture
│   │   ├── whisper_engine.c     # Whisper integration
│   │   ├── translation_engine.cpp # T5 translation with llama.cpp
│   │   └── ipc.c                # JSON-RPC over stdio
│   └── libs/                     # Git submodules
│       ├── whisper.cpp/         # Whisper inference engine
│       └── llama.cpp/           # LLM inference engine (for T5)
│
├── frontend/                     # Electron frontend
│   ├── src/
│   │   ├── main.js              # Electron main process
│   │   ├── overlay.html         # UI template with controls
│   │   ├── renderer.js          # Renderer process (UI logic)
│   │   └── backend-ipc.js       # Backend communication handler
│   ├── package.json             # Node dependencies
│   └── node_modules/            # NPM packages
│
├── models/                       # AI models (GGUF format)
│   ├── whisper-base.gguf        # 141MB, 99 languages
│   ├── whisper-small.gguf       # 466MB, better quality
│   ├── whisper-medium.gguf      # 769MB, great quality
│   ├── whisper-large-v3.gguf    # 1.5GB, best quality
│   └── mt5-small.gguf           # 300MB, multilingual translation
│
├── build/                        # CMake build output
│   └── visualia                 # Compiled backend executable
│
├── scripts/                      # Build and setup scripts
│   ├── setup.sh                 # Full setup (submodules + build)
│   ├── build.sh                 # Build backend + frontend deps
│   └── setup_t5_translation.sh  # Download/convert T5 model
│
├── CMakeLists.txt               # CMake configuration
├── DOCUMENTATION.md             # This file
└── README.md                    # Quick start guide
```

### Key Files Explained

#### Backend

**`backend/src/main.c`** (Entry Point)
- Parses command-line arguments (`-m model`, `-l language`, `-t target_lang`)
- Initializes audio, Whisper, and translation engines
- Runs main event loop with IPC polling
- Handles graceful shutdown with signal handlers

**`backend/src/audio.c`** (Audio Capture)
- Platform abstraction for audio input
- macOS: AudioQueue from CoreAudio
- Linux: PulseAudio simple API
- Windows: WASAPI with COM interfaces
- Converts PCM int16 → float32 for Whisper
- Buffers 3-second chunks with 1-second overlap

**`backend/src/whisper_engine.c`** (Speech-to-Text)
- Wraps whisper.cpp C++ API with C interface
- Loads GGUF models
- Processes audio chunks with Whisper
- Supports language specification or auto-detect
- Invokes callback with transcription results

**`backend/src/translation_engine.cpp`** (Translation)
- Wraps llama.cpp for T5 encoder-decoder models
- Uses worker thread for async translation
- Builds T5 prompts: "translate English to French: <text>"
- Greedy token sampling for translation generation
- Caches translations to avoid redundant work

**`backend/src/ipc.c`** (Communication)
- JSON-RPC over stdio (stdout for messages, stderr for logs)
- Message types: `transcription`, `translation`, `status`, `error`
- Escapes JSON strings properly
- Line-buffered output for immediate delivery

#### Frontend

**`frontend/src/main.js`** (Electron Main)
- Creates transparent overlay window
- Spawns backend C process with arguments
- Routes IPC messages: backend ↔ renderer
- Handles settings changes (model, language, translation)
- Manages window click-through behavior

**`frontend/src/renderer.js`** (UI Logic)
- Handles user input (dropdowns, toggle)
- Displays transcriptions and translations
- Manages auto-hide timeout (5 seconds)
- Saves/loads settings to localStorage
- Sends control messages to main process

**`frontend/src/backend-ipc.js`** (Backend Handler)
- Parses line-delimited JSON from backend stdout
- Emits events for each message type
- Buffers partial messages across reads
- Error handling for malformed JSON

**`frontend/src/overlay.html`** (UI Template)
- Transparent overlay with controls
- Model selector (base/small/medium/large-v3)
- Source language dropdown (28 languages with flags)
- Live translation toggle switch
- Target language selector (15 languages)
- Subtitle display (original + translation)

---

## Component Interactions

### 1. Startup Sequence

```
User runs: npm start

1. main.js creates BrowserWindow
2. main.js spawns backend process:
   ./build/visualia -m models/whisper-base.gguf -l auto

3. Backend initializes:
   - IPC system (stdio)
   - Whisper engine (load model)
   - Audio capture (CoreAudio/PulseAudio)
   - Translation engine (if -t specified)

4. Backend sends IPC: {"type":"status","data":{"message":"Running..."}}

5. Frontend loads overlay.html
6. renderer.js loads settings from localStorage
7. UI displays "Connecting to backend..."
8. Backend status received → UI shows "Running - listening for audio..."
```

### 2. Audio Processing Flow

```
Audio Device captures sound
       ↓
audio.c callback: on_audio_data()
       ↓
Accumulate in 3-second buffer
       ↓
Buffer full? → Process with Whisper
       ↓
whisper_engine_process() calls whisper.cpp
       ↓
Transcription complete → on_transcription() callback
       ↓
Send IPC: {"type":"transcription","data":{"text":"Hello","timestamp":1234567890}}
       ↓
Frontend backend-ipc.js parses JSON
       ↓
Emits 'transcription' event
       ↓
main.js forwards to renderer
       ↓
renderer.js: showSubtitle("Hello")
       ↓
UI displays text in subtitle box
```

### 3. Translation Flow

```
Transcription received: "Bonjour"
       ↓
Translation enabled? Check g_translator && g_target_lang
       ↓
Yes → Create copy of text with strdup()
       ↓
translation_translate(engine, "Bonjour", "fr", "en")
       ↓
Worker thread picks up request from queue
       ↓
Build T5 prompt: "translate French to English: Bonjour"
       ↓
Tokenize with llama_tokenize()
       ↓
Run encoder-decoder inference with llama_decode()
       ↓
Sample tokens greedily until EOS
       ↓
Result: "Hello"
       ↓
on_translation() callback
       ↓
Send IPC: {"type":"translation","data":{"text":"Hello","original":"Bonjour"}}
       ↓
Frontend renders translation below original
```

### 4. Settings Change Flow

```
User changes model dropdown: base → small
       ↓
renderer.js: modelSelect.addEventListener('change')
       ↓
Update currentSettings.model = 'small'
       ↓
Save to localStorage
       ↓
Send IPC to main: ipcRenderer.send('change-model', 'small')
       ↓
main.js: ipcMain.on('change-model')
       ↓
stopBackend() - kills C process
       ↓
Update process.env.VISUALIA_MODEL = 'whisper-small.gguf'
       ↓
setTimeout(startBackend, 1000)
       ↓
Spawn new backend with new model
       ↓
Backend initializes with new model
       ↓
Audio processing continues with new model
```

### 5. Translation Toggle Flow

```
User clicks "Live Translation" toggle
       ↓
renderer.js: translationToggle.addEventListener('change')
       ↓
currentSettings.translationEnabled = true
       ↓
Show target language dropdown
       ↓
Send IPC: ipcRenderer.send('change-translation', {enabled: true, targetLang: 'en'})
       ↓
main.js: ipcMain.on('change-translation')
       ↓
stopBackend()
       ↓
Set process.env.VISUALIA_TARGET_LANG = 'en'
       ↓
startBackend() with -t en flag
       ↓
Backend initializes translation_engine
       ↓
All future transcriptions trigger translation
```

---

## Installation & Setup

### Prerequisites

- **Operating System**: macOS 10.15+, Linux, or Windows 10+
- **CMake**: 3.15 or newer
- **Compiler**: Clang, GCC, or MSVC with C11 and C++17 support
- **Node.js**: 18.x or newer
- **Git**: For cloning and submodules
- **Python 3**: For model conversion (translation setup)

### Quick Setup

```bash
# 1. Clone repository
git clone https://github.com/yourusername/VisualIA.git
cd VisualIA

# 2. Initialize submodules
git submodule update --init --recursive

# 3. Run automated setup
bash scripts/setup.sh

# 4. Download Whisper model
# Visit: https://huggingface.co/ggerganov/whisper.cpp/tree/main
# Download whisper-base.gguf (141MB) to models/

# 5. (Optional) Setup translation
bash scripts/setup_t5_translation.sh
```

### Manual Setup

```bash
# Build backend
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)  # Linux
cmake --build . -j$(sysctl -n hw.ncpu)  # macOS
cd ..

# Install frontend dependencies
cd frontend
npm install
cd ..
```

### Platform-Specific Requirements

#### macOS
```bash
# Xcode Command Line Tools required
xcode-select --install

# Grant microphone permission
# System Preferences → Security & Privacy → Privacy → Microphone
```

#### Linux (Ubuntu/Debian)
```bash
# Install PulseAudio development headers
sudo apt-get update
sudo apt-get install libpulse-dev cmake build-essential
```

#### Windows
```bash
# Install Visual Studio 2019+ with C++ tools
# CMake and Node.js from official websites
# WASAPI audio capture is fully implemented
# Grant microphone permission in Windows Settings
```

---

## Usage Guide

### Starting VisualIA

#### Basic Usage (Auto-detect language)
```bash
cd frontend
npm start
```

#### With Specific Language
```bash
# French
VISUALIA_LANG=fr npm start --prefix frontend

# Spanish
VISUALIA_LANG=es npm start --prefix frontend

# Japanese
VISUALIA_LANG=ja npm start --prefix frontend
```

#### Backend Standalone Testing
```bash
# Test transcription without UI
./build/visualia -m models/whisper-base.gguf -l en

# With translation
./build/visualia -m models/whisper-base.gguf -l fr -t en

# Help
./build/visualia -h
```

### UI Controls

**Top-Right Panel:**

1. **Model Selector**
   - Base (141MB, Fast)
   - Small (466MB, Better)
   - Medium (769MB, Great)
   - Large v3 (1.5GB, Best)

2. **Source Language**
   - Auto-Detect (recommended)
   - 28 languages with flag emojis

3. **Live Translation Toggle**
   - Enable/disable translation on the fly
   - Shows target language dropdown when enabled

4. **Translate To**
   - Choose target language
   - 15 supported languages

**Subtitle Box:**
- Shows original transcription
- Shows translation below (if enabled)
- Auto-hides after 5 seconds of silence

### Keyboard Shortcuts

- **Mouse hover over controls**: Enables clicking
- **Mouse leave controls**: Window becomes click-through
- **Ctrl+C in terminal**: Gracefully stops backend

---

## Language Support

### Supported Languages (99 total)

**Western European**
English (en), French (fr), Spanish (es), German (de), Italian (it), Portuguese (pt), Dutch (nl), Swedish (sv), Norwegian (no), Danish (da), Finnish (fi)

**Eastern European**
Polish (pl), Czech (cs), Romanian (ro), Greek (el), Ukrainian (uk), Russian (ru)

**Middle Eastern**
Arabic (ar), Hebrew (he), Turkish (tr)

**Asian**
Chinese (zh), Japanese (ja), Korean (ko), Hindi (hi), Thai (th), Vietnamese (vi), Indonesian (id), Malay (ms)

**And 71+ more languages...**

### Model Comparison

| Model                 | Size   | Languages    | Speed  | Accuracy     | Use Case                |
|-----------------------|--------|--------------|--------|--------------|-------------------------|
| whisper-base.en.gguf  | 141MB  | English only | Faster | Good         | English-only users      |
| whisper-base.gguf     | 141MB  | 99 languages | Fast   | Good         | **Recommended default** |
| whisper-small.gguf    | 466MB  | 99 languages | Medium | Better       | Better accuracy         |
| whisper-medium.gguf   | 769MB  | 99 languages | Slow   | Great        | High accuracy needed    |
| whisper-large-v3.gguf | 1.5GB  | 99 languages | Slower | Best         | Maximum accuracy        |

### Auto-Detect vs Manual Language

**Auto-Detect (Recommended)**
- Whisper analyzes first 30 seconds of audio
- Minimal overhead (~100ms)
- Works seamlessly in multilingual environments
- Automatically switches between languages

**Manual Language Selection**
- Slightly better accuracy for specified language
- Faster startup (no detection phase)
- Use when you know the language in advance

---

## Translation System

### Overview

VisualIA uses **Google's mT5 (Multilingual T5)** model via llama.cpp for local translation. No cloud APIs or internet connection required.

### Architecture

```
Transcription: "Bonjour"
       ↓
T5 Prompt: "translate French to English: Bonjour"
       ↓
T5 Encoder → T5 Decoder (via llama.cpp)
       ↓
Token Generation (greedy sampling)
       ↓
Result: "Hello"
```

### Supported Translation Languages

**Target Languages (15):**
- English (en)
- French (fr)
- Spanish (es)
- German (de)
- Italian (it)
- Portuguese (pt)
- Dutch (nl)
- Polish (pl)
- Russian (ru)
- Chinese (zh)
- Japanese (ja)
- Korean (ko)
- Arabic (ar)
- Hindi (hi)
- Turkish (tr)

### Translation Models

| Model       | Size  | Quality | Speed  | Recommended |
|-------------|-------|---------|--------|-------------|
| mt5-small   | 300MB | Good    | Fast   | ✅ Yes      |
| mt5-base    | 580MB | Better  | Medium | For quality |
| mt5-large   | 1.2GB | Best    | Slow   | High-end    |

### Setup Translation

```bash
# Download and convert mT5-small (recommended)
bash scripts/setup_t5_translation.sh

# This will:
# 1. Download mt5-small from HuggingFace
# 2. Convert to GGUF format using llama.cpp
# 3. Save to models/mt5-small.gguf (~300MB)
```

### Using Translation

**Via UI:**
1. Start VisualIA normally
2. Toggle "Live Translation" ON
3. Select target language
4. Backend restarts with translation enabled
5. All transcriptions are automatically translated

**Via Command Line:**
```bash
# French audio → English translation
./build/visualia -l fr -t en

# Auto-detect source → Spanish translation
./build/visualia -l auto -t es
```

### Translation Performance

- **Latency**: ~500ms per translation (depends on text length)
- **Caching**: Translations are cached in memory (max 100 entries)
- **Threading**: Worker thread ensures non-blocking operation
- **GPU**: Metal acceleration on Apple Silicon

### Translation Quality Tips

1. **Use larger models** for better quality
2. **Specify source language** when known (improves T5 performance)
3. **Short sentences** translate better than long paragraphs
4. **Technical terms** may not translate accurately

---

## Development Guide

### Project Structure Deep Dive

#### Backend Header Files

**`backend/include/audio.h`**
```c
// Audio capture abstraction
typedef void (*audio_callback_t)(const float *samples, size_t num_samples, void *user_data);

audio_context_t* audio_init(audio_callback_t callback, void *user_data);
bool audio_start(audio_context_t *ctx);
bool audio_stop(audio_context_t *ctx);
void audio_cleanup(audio_context_t *ctx);
```

**`backend/include/whisper_engine.h`**
```c
// Whisper STT wrapper
typedef void (*transcription_callback_t)(const char *text, void *user_data);

whisper_engine_t* whisper_engine_init(
    const char *model_path,
    const char *language,
    transcription_callback_t callback,
    void *user_data
);

bool whisper_engine_process(
    whisper_engine_t *engine,
    const float *samples,
    size_t num_samples
);
```

**`backend/include/translation_engine.h`**
```c
// T5 translation wrapper
typedef void (*translation_callback_t)(const char *translated_text, void *user_data);

translation_engine_t* translation_init(
    const char *model_path,
    translation_callback_t callback,
    void *user_data
);

bool translation_translate(
    translation_engine_t *engine,
    const char *text,
    const char *source_lang,
    const char *target_lang
);
```

**`backend/include/ipc.h`**
```c
// JSON-RPC over stdio
bool ipc_send_transcription(const char *text, long timestamp);
bool ipc_send_translation(const char *translated_text, const char *original_text, long timestamp);
bool ipc_send_status(const char *status);
bool ipc_send_error(const char *error_msg);
bool ipc_poll(void);
```

### Building from Source

#### CMake Configuration

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Enable GPU acceleration (Apple Silicon)
cmake -DGGML_METAL=ON ..

# Disable GPU (CPU only)
cmake -DGGML_METAL=OFF ..
```

#### Compilation Flags

The project uses:
- **C Standard**: C11
- **C++ Standard**: C++17
- **Warnings**: `-Wall -Wextra -pedantic`
- **Optimizations**: `-O3` in Release mode

### Running in Development Mode

```bash
# Terminal 1: Backend with debug output
./build/visualia -m models/whisper-base.gguf -l auto 2>&1 | tee debug.log

# Terminal 2: Frontend with DevTools
NODE_ENV=development npm start --prefix frontend
```

### Debugging

#### Backend (C)

**Using GDB:**
```bash
gdb ./build/visualia
(gdb) set args -m models/whisper-base.gguf -l en
(gdb) break main.c:100
(gdb) run
```

**Using LLDB (macOS):**
```bash
lldb ./build/visualia
(lldb) settings set -- target.run-args -m models/whisper-base.gguf -l en
(lldb) breakpoint set --file main.c --line 100
(lldb) run
```

**Print Debugging:**
```c
// All stderr output goes to terminal
fprintf(stderr, "[DEBUG] Variable: %d\n", value);
```

#### Frontend (JavaScript)

**DevTools:**
- Press `Cmd+Option+I` (macOS) or `Ctrl+Shift+I` (Linux/Windows)
- View Console for `console.log()` output
- Network tab for debugging (not used in this app)

**Renderer Process:**
```javascript
// All console.log goes to DevTools console
console.log('[Renderer] Message:', data);
```

**Main Process:**
```javascript
// console.log goes to terminal
console.log('[Electron] Backend started');
```

### Code Style Guidelines

**C Code:**
- K&R style bracing
- 4-space indentation (no tabs)
- Snake_case for functions and variables
- ALL_CAPS for macros
- Comment complex logic

**JavaScript Code:**
- StandardJS style
- 4-space indentation
- camelCase for variables and functions
- Clear function names

**Commit Messages:**
- Format: `type: description`
- Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`
- Example: `feat: add T5 translation support`

### Adding New Features

#### Example: Adding Screen Capture

1. **Create header**: `backend/include/screen.h`
2. **Implement**: `backend/src/screen.c`
3. **Update CMakeLists.txt**: Add source file
4. **Integrate in main.c**: Initialize and use screen capture
5. **Add IPC message**: Define `ipc_send_screen_data()`
6. **Frontend**: Handle screen data in renderer

#### Example: Adding New Translation Language

1. **Update language map** in `translation_engine.cpp`:
   ```cpp
   if (strcmp(lang_code, "fa") == 0) return "Farsi";
   ```

2. **Add to UI** in `overlay.html`:
   ```html
   <option value="fa">🇮🇷 Farsi</option>
   ```

3. **Test**: Verify translation works end-to-end

---

## API Reference

### Backend Command-Line Interface

```bash
./build/visualia [OPTIONS]

OPTIONS:
  -m MODEL    Path to Whisper model file (default: models/whisper-base.gguf)
  -l LANG     Source language code or 'auto' (default: auto)
  -t LANG     Target language for translation (optional)
  -T MODEL    Path to translation model (default: models/mt5-small.gguf)
  -h          Show help message

EXAMPLES:
  ./build/visualia
  ./build/visualia -m models/whisper-small.gguf -l en
  ./build/visualia -l fr -t en
  ./build/visualia -m models/whisper-large-v3.gguf -l auto -t es
```

### IPC Message Format

#### Transcription Message
```json
{
  "type": "transcription",
  "data": {
    "text": "Hello world",
    "timestamp": 1234567890
  }
}
```

#### Translation Message
```json
{
  "type": "translation",
  "data": {
    "text": "Bonjour le monde",
    "original": "Hello world",
    "timestamp": 1234567890
  }
}
```

#### Status Message
```json
{
  "type": "status",
  "data": {
    "message": "Running - listening for audio..."
  }
}
```

#### Error Message
```json
{
  "type": "error",
  "data": {
    "message": "Failed to load model: file not found"
  }
}
```

### Electron IPC Events

#### Renderer → Main

```javascript
// Change Whisper model
ipcRenderer.send('change-model', 'small');

// Change source language
ipcRenderer.send('change-source-lang', 'fr');

// Enable/disable translation
ipcRenderer.send('change-translation', {
  enabled: true,
  targetLang: 'en'
});

// Toggle click-through
ipcRenderer.send('toggle-click-through', true);
```

#### Main → Renderer

```javascript
// Transcription received
ipcRenderer.on('transcription', (event, data) => {
  // data: { text: "...", timestamp: 1234567890 }
});

// Translation received
ipcRenderer.on('translation', (event, data) => {
  // data: { text: "...", original: "...", timestamp: 1234567890 }
});

// Status update
ipcRenderer.on('status', (event, data) => {
  // data: { message: "..." }
});

// Error occurred
ipcRenderer.on('error', (event, data) => {
  // data: { message: "..." }
});
```

---

## Troubleshooting

### Common Issues

#### No Audio Input

**Symptoms:**
- Backend runs but no transcriptions appear
- Audio buffer never fills

**Solutions:**
1. Check microphone permissions (macOS/Linux)
2. Verify default input device: `pactl list sources` (Linux)
3. Test with: `arecord -d 5 test.wav` (Linux) or QuickTime (macOS)
4. Check backend logs for audio errors

#### Model Load Fails

**Symptoms:**
```
[Main] Failed to initialize Whisper: whisper_init_from_file: failed to load model
```

**Solutions:**
1. Verify model file exists: `ls -lh models/`
2. Check model format is GGUF (not GGML or BIN)
3. Download correct model from HuggingFace
4. Ensure model path is correct (absolute or relative)

#### Backend Crashes

**Symptoms:**
- Backend process exits immediately
- Frontend shows "Backend exited with code 139" (segfault)

**Solutions:**
1. Run backend directly to see error: `./build/visualia -m models/whisper-base.gguf`
2. Check model compatibility (some older models don't work)
3. Enable debug build: `cmake -DCMAKE_BUILD_TYPE=Debug ..`
4. Use debugger: `gdb ./build/visualia` and `run -m models/whisper-base.gguf`

#### Translation Not Working

**Symptoms:**
- Transcriptions appear but no translations
- Translation shows "Translating..." forever

**Solutions:**
1. Check translation model exists: `ls -lh models/mt5-small.gguf`
2. Run setup script: `bash scripts/setup_t5_translation.sh`
3. Check backend logs for translation errors
4. Verify translation enabled in UI (toggle is ON)

#### UI Not Responding to Clicks

**Symptoms:**
- Cannot click on dropdowns or buttons
- Mouse passes through window

**Solutions:**
1. Hover over the controls panel (top-right)
2. Click-through is intentional for subtitle area
3. If controls still don't work, check console for errors

#### High CPU Usage

**Symptoms:**
- Backend using 100% CPU constantly
- Computer fan running loud

**Solutions:**
1. Use smaller Whisper model (base instead of large)
2. Increase audio chunk size (reduce processing frequency)
3. Enable GPU acceleration (Metal on macOS)
4. Check for infinite loops in backend logs

#### Translation Quality Poor

**Symptoms:**
- Translations are inaccurate
- Wrong language detected

**Solutions:**
1. Specify source language explicitly (don't use auto)
2. Use larger T5 model: `mt5-base` or `mt5-large`
3. Ensure audio transcription quality is good first
4. Some language pairs may have limited training data

### Platform-Specific Issues

#### macOS

**Microphone Permission Denied:**
```bash
# Grant permission in System Preferences
System Preferences → Security & Privacy → Privacy → Microphone
# Enable for Terminal/iTerm
```

**Rosetta 2 (Intel Macs):**
- Whisper and llama.cpp run slower on Intel
- Consider using smaller models
- Metal acceleration not available on Intel

#### Linux

**PulseAudio Not Found:**
```bash
sudo apt-get install libpulse-dev pulseaudio
systemctl --user start pulseaudio
```

**Audio Device Errors:**
```bash
# List available devices
pactl list sources

# Set default device
pactl set-default-source alsa_input.pci-0000_00_1f.3.analog-stereo
```

#### Windows

**Microphone Permission:**
```bash
# Grant permission in Windows Settings
Settings → Privacy → Microphone
# Enable for the application
```

**Audio Device Errors:**
```powershell
# List available audio devices (PowerShell)
Get-AudioDevice -List

# Check default recording device
Get-AudioDevice -RecordingMute
```

### Error Codes

| Code | Meaning                    | Solution                                  |
|------|----------------------------|-------------------------------------------|
| 1    | Initialization failed      | Check model paths and permissions         |
| 139  | Segmentation fault         | Debug with GDB, check model compatibility |
| 255  | Generic error              | Check backend logs for details            |

### Getting Help

1. **Check logs**: Backend stderr output in terminal
2. **Enable debug mode**: `NODE_ENV=development npm start`
3. **Search issues**: https://github.com/yourusername/VisualIA/issues
4. **Ask questions**: Open new issue with logs and system info

---

## Performance Tips

### Optimizing Latency

1. **Use smaller models**: base < small < medium < large
2. **Reduce audio chunk size**: Modify `AUDIO_CHUNK_SIZE` in main.c
3. **Enable GPU**: Ensure Metal (macOS) or CUDA (Linux) enabled
4. **Disable translation**: Only enable when needed

### Reducing Memory Usage

1. **Close unused apps**: Free up RAM for models
2. **Use base model**: 141MB vs 1.5GB for large
3. **Limit translation cache**: Reduce from 100 to 50 entries

### Improving Accuracy

1. **Use larger models**: large-v3 > medium > small > base
2. **Specify language**: Better than auto-detect for single language
3. **Good audio quality**: Use external microphone if possible
4. **Quiet environment**: Reduce background noise

---

## License

MIT License - See LICENSE file for details

---

## Contributors

- Original Author: [Your Name]
- Claude (Anthropic): Architecture design and implementation assistance

---

## Changelog

### v1.0.0 (Current)
- ✅ Real-time speech-to-text with Whisper
- ✅ 99 language support with auto-detect
- ✅ Live translation with mT5
- ✅ Electron overlay UI
- ✅ Multiple model support
- ✅ GPU acceleration (Metal)
- ✅ Cross-platform audio (macOS, Linux, Windows)
- ✅ Windows WASAPI audio capture implementation

### v0.2.0
- Added T5 translation engine
- Improved UI with translation toggle
- Fixed transparent window lag
- Added backend IPC for translation

### v0.1.0
- Initial release
- Basic speech-to-text
- Electron overlay
- Whisper integration

---

**For the latest updates, visit: https://github.com/yourusername/VisualIA**
