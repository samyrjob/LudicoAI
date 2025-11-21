# VisualIA

Real-time AI assistant with live speech transcription, translation, and overlay display.

## Features

- **Live Speech-to-Text**: Real-time transcription using Whisper models
- **99 Languages**: Automatic language detection or manual selection
- **Live Translation**: Local T5-based translation (no cloud APIs)
- **Overlay Display**: Transparent, always-on-top subtitle window
- **GPU Accelerated**: Metal support for Apple Silicon
- **Cross-platform**: macOS, Linux, Windows

## Quick Start

### 1. Clone and Setup

```bash
git clone https://github.com/yourusername/VisualIA.git
cd VisualIA
bash scripts/setup.sh
```

The setup script will:
- Initialize git submodules (whisper.cpp, llama.cpp)
- Build the C backend
- Install frontend dependencies
- **Optionally** download and setup translation models (MT5)

**Translation Setup Options:**
1. Skip (transcription only)
2. Install mt5-small (~300MB, recommended)
3. Install mt5-base (~580MB, better quality)
4. Install mt5-large (~1.2GB, best quality)
5. Install all sizes

### 2. Download Whisper Model

Download from [HuggingFace Whisper Models](https://huggingface.co/ggerganov/whisper.cpp/tree/main):

```bash
# Base model (141MB, recommended)
wget -P models https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin
mv models/ggml-base.bin models/whisper-base.gguf
```

Or use the model switcher in the GUI to select from:
- **Base** (141MB) - Fast, good quality
- **Small** (466MB) - Better accuracy
- **Medium** (769MB) - Great accuracy
- **Large v3** (1.5GB) - Best accuracy

### 3. Run

```bash
cd frontend
npm start
```

The overlay will appear with:
- **Model selector** - Choose Whisper model size
- **Source language** - Auto-detect or specific language
- **Live Translation toggle** - Enable real-time translation
- **Target language** - Choose translation output language
- **Translation model** - Select MT5 model size (small/base/large)

## Usage

### Basic Transcription

```bash
# Auto-detect language (default)
npm start --prefix frontend

# Specific language
VISUALIA_LANG=fr npm start --prefix frontend
```

### With Translation

#### Option 1: During Setup (Recommended)
Run `bash scripts/setup.sh` and select a translation model when prompted.

#### Option 2: Manual Installation
```bash
# Install specific model
bash scripts/setup_t5_translation.sh google/mt5-small    # 300MB
bash scripts/setup_t5_translation.sh google/mt5-base     # 580MB
bash scripts/setup_t5_translation.sh google/mt5-large    # 1.2GB

# Or install all sizes for switching in GUI
bash scripts/setup_t5_translation.sh google/mt5-small
bash scripts/setup_t5_translation.sh google/mt5-base
bash scripts/setup_t5_translation.sh google/mt5-large
```

#### Enable in UI
1. Click **"Live Translation"** toggle in overlay
2. Select **target language** (English, French, Spanish, etc.)
3. Choose **translation model** (MT5-Small/Base/Large)
4. Start speaking - see both transcription and translation!

### Backend Standalone

```bash
# Transcription only
./build/visualia -m models/whisper-base.gguf -l en

# With translation
./build/visualia -l fr -t en

# Help
./build/visualia -h
```

## Architecture

```
┌─────────────────────────────────────────┐
│        Electron Overlay UI               │
│  Controls | Subtitles | Translation      │
└───────────────┬─────────────────────────┘
                │ JSON-RPC (stdio)
                ↓
┌─────────────────────────────────────────┐
│         C Backend                        │
│  Audio → Whisper → T5 Translation        │
│  CoreAudio/PulseAudio | whisper.cpp      │
│  llama.cpp (T5)                          │
└─────────────────────────────────────────┘
```

## Documentation

**Full documentation available in [DOCUMENTATION.md](DOCUMENTATION.md)**

Topics covered:
- Complete architecture overview
- Codebase structure and file descriptions
- Component interactions and data flows
- Installation and setup guides
- Language support (99 languages)
- Translation system (T5 with llama.cpp)
- Development guide and API reference
- Troubleshooting and performance tips

## Prerequisites

- **CMake**: 3.15+
- **Compiler**: C11 and C++17 support
- **Node.js**: 18+
- **Platform**: macOS 10.15+ / Linux / Windows 10+
- **Python 3**: For translation setup

## Project Structure

```
VisualIA/
├── backend/          # C backend (audio, Whisper, T5)
│   ├── src/         # Source files
│   ├── include/     # Headers
│   └── libs/        # whisper.cpp, llama.cpp
├── frontend/         # Electron UI
│   └── src/         # main.js, renderer.js, overlay.html
├── models/          # AI models (GGUF format)
├── scripts/         # Build and setup scripts
└── DOCUMENTATION.md # Complete documentation
```

## Models

### Whisper (Speech-to-Text)

| Model                 | Size   | Languages | Quality |
|-----------------------|--------|-----------|---------|
| whisper-base.gguf     | 141MB  | 99        | Good    |
| whisper-small.gguf    | 466MB  | 99        | Better  |
| whisper-medium.gguf   | 769MB  | 99        | Great   |
| whisper-large-v3.gguf | 1.5GB  | 99        | Best    |

### Translation (T5)

| Model      | Size  | Quality |
|------------|-------|---------|
| mt5-small  | 300MB | Good    |
| mt5-base   | 580MB | Better  |
| mt5-large  | 1.2GB | Best    |

## Supported Languages

**Transcription**: 99 languages with auto-detect

**Translation**: 15 languages
- English, French, Spanish, German, Italian, Portuguese
- Dutch, Polish, Russian, Chinese, Japanese, Korean
- Arabic, Hindi, Turkish

## Development

### Build from Source

```bash
# Backend
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)

# Frontend
cd frontend && npm install
```

### Debug Mode

```bash
# Backend with logs
./build/visualia -m models/whisper-base.gguf 2>&1 | tee debug.log

# Frontend with DevTools
NODE_ENV=development npm start --prefix frontend
```

## Contributing

Contributions welcome! Areas needing help:
- Screen capture integration
- LLaVA vision support
- UI improvements
- GPU acceleration for Windows/Linux

See [DOCUMENTATION.md](DOCUMENTATION.md) for development guide.

## Troubleshooting

### No Audio Input
```bash
# macOS: Grant microphone permission
System Preferences → Security & Privacy → Microphone

# Linux: Check PulseAudio
pactl list sources

# Windows: Grant microphone permission
Settings → Privacy → Microphone
```

### Model Not Loading
- Verify file exists: `ls -lh models/`
- Check GGUF format (not GGML or BIN)
- Test backend: `./build/visualia -m models/whisper-base.gguf`

### Translation Not Working
```bash
# Install translation model if not already done
bash scripts/setup_t5_translation.sh google/mt5-small

# Verify model exists
ls -lh models/mt5-*.gguf

# Check if model matches selection in GUI
# Make sure the model size you selected is actually downloaded
```

For more issues, see [DOCUMENTATION.md - Troubleshooting](DOCUMENTATION.md#troubleshooting)

## Performance Tips

- **Faster**: Use `whisper-base.gguf`
- **Better accuracy**: Use `whisper-large-v3.gguf`
- **Lower memory**: Disable translation when not needed
- **GPU**: Enabled by default on Apple Silicon

## License

MIT

## Credits

- **whisper.cpp**: https://github.com/ggerganov/whisper.cpp
- **llama.cpp**: https://github.com/ggerganov/llama.cpp
- **OpenAI Whisper**: Speech recognition models
- **Google T5**: Translation models

---

**For complete documentation, see [DOCUMENTATION.md](DOCUMENTATION.md)**
