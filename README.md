# VisualIA

Real-time AI assistant with screen understanding, live subtitles, translation, and contextual information.

## Features

- **Live Subtitles**: Real-time speech transcription using Whisper
- **Screen Understanding**: Visual context with LLaVA (coming soon)
- **Translation**: Real-time language translation (coming soon)
- **Context-Aware**: Definitions, pronunciation, and information on demand (coming soon)

## Architecture

- **Backend**: C-based audio processing with whisper.cpp and llama.cpp
- **Frontend**: Electron overlay for transparent subtitle display
- **IPC**: JSON-RPC communication between backend and frontend

## Prerequisites

- CMake 3.15+
- C compiler (GCC/Clang/MSVC)
- Node.js 18+
- Git

## Building

### 1. Clone with submodules
```bash
git submodule update --init --recursive
```

### 2. Build backend
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### 3. Install frontend dependencies
```bash
cd frontend
npm install
```

### 4. Download models
Place Whisper models in `models/` directory:
- Recommended: `whisper-small.en.gguf` or `whisper-base.en.gguf`
- Download from: https://huggingface.co/ggerganov/whisper.cpp

## Quick Start

### Option 1: Automated Setup
```bash
bash scripts/setup.sh
```

### Option 2: Manual Setup
```bash
# 1. Initialize submodules
git submodule update --init --recursive

# 2. Build
bash scripts/build.sh

# 3. Download Whisper model
# Get from: https://huggingface.co/ggerganov/whisper.cpp/tree/main
# Place in models/whisper-base.en.gguf
```

## Usage

### Start the application
```bash
# Start frontend (automatically launches backend)
cd frontend
npm start

# Backend will look for model at: models/whisper-base.en.gguf
# Subtitles will appear at the bottom of your screen
```

### Standalone Backend Testing
```bash
# Run backend independently
./build/visualia -m models/whisper-base.en.gguf

# Backend outputs JSON to stdout
# stderr shows debug logs
```

## Development Status

- [x] Project structure
- [ ] Audio capture (cross-platform)
- [ ] Whisper integration
- [ ] IPC layer
- [ ] Electron overlay
- [ ] Live subtitle display
- [ ] Screen capture
- [ ] LLaVA integration
- [ ] Translation
- [ ] Context features

## License

MIT
