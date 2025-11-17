# Development Guide

## Project Structure

```
VisualIA/
├── backend/               # C backend
│   ├── src/              # Source files
│   │   ├── main.c        # Entry point & main loop
│   │   ├── audio.c       # Cross-platform audio capture
│   │   ├── whisper_engine.c  # Whisper integration
│   │   └── ipc.c         # JSON-RPC over stdio
│   ├── include/          # Header files
│   └── libs/             # Git submodules (whisper.cpp, llama.cpp)
├── frontend/             # Electron app
│   └── src/              # Frontend source
│       ├── main.js       # Electron main process
│       ├── backend-ipc.js # Backend communication
│       ├── overlay.html  # UI
│       └── renderer.js   # Renderer process
├── models/               # AI models (.gguf)
└── scripts/              # Build scripts
```

## Architecture

### Backend (C)
- **Audio Capture**: Platform-specific implementations
  - macOS: Core Audio (AudioQueue)
  - Linux: PulseAudio
  - Windows: WASAPI (stub)
- **Whisper Integration**: Uses whisper.cpp for STT
- **IPC**: JSON-RPC over stdio for frontend communication

### Frontend (Electron)
- **Overlay Window**: Transparent, always-on-top
- **IPC Handler**: Receives transcriptions from backend
- **UI**: Minimalist subtitle display

### Communication Flow
```
Audio Device → Audio Capture → Buffer (3s chunks)
                                    ↓
                              Whisper STT
                                    ↓
                              Transcription
                                    ↓
                          IPC (JSON over stdio)
                                    ↓
                            Electron Frontend
                                    ↓
                             Overlay Display
```

## Building

### Prerequisites
- CMake 3.15+
- C compiler (Clang/GCC)
- Node.js 18+
- Git

### Setup
```bash
# Clone and initialize submodules
git submodule update --init --recursive

# Run setup script
bash scripts/setup.sh
```

### Manual Build
```bash
# Build backend
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)
cd ..

# Install frontend dependencies
cd frontend && npm install && cd ..
```

## Running

### Development Mode
```bash
# Terminal 1: Backend with logging
./build/visualia -m models/whisper-base.en.gguf

# Terminal 2: Frontend with DevTools
cd frontend
NODE_ENV=development npm start
```

### Production Mode
```bash
cd frontend
npm start
```

## Models

### Whisper Models
Download from: https://huggingface.co/ggerganov/whisper.cpp/tree/main

Recommended models:
- `ggml-base.en.bin` (142 MB) - Fast, English only
- `ggml-small.en.bin` (466 MB) - Better accuracy, English only
- `ggml-base.bin` (142 MB) - Multilingual

Place in `models/` directory and rename to `.gguf` extension.

## Platform-Specific Notes

### macOS
- Requires microphone permission
- Request access in System Preferences → Security & Privacy

### Linux
- Requires PulseAudio
- Install: `sudo apt-get install libpulse-dev`

### Windows
- WASAPI implementation incomplete
- Contribution welcome!

## Adding Features

### Future Extensions
1. **Screen Capture**: Add in `backend/src/screen.c`
2. **LLaVA Integration**: Use llama.cpp for vision
3. **Translation**: Add translation service
4. **Multi-language**: Remove English-only restriction

### Code Style
- C: Follow K&R style
- JavaScript: StandardJS
- 4-space indentation
- Clear comments for complex logic

## Debugging

### Backend Debugging
```bash
# Run with logging
./build/visualia -m models/whisper-base.en.gguf 2>&1 | tee debug.log

# GDB debugging
gdb ./build/visualia
```

### Frontend Debugging
```bash
# Enable DevTools
NODE_ENV=development npm start --prefix frontend

# Check IPC messages
# See renderer.js console.log output in DevTools
```

### Common Issues

**No audio input:**
- Check microphone permissions
- Verify default input device

**Model load fails:**
- Check model path is correct
- Ensure model format is compatible (.gguf)

**IPC not working:**
- Check backend stdout/stderr
- Verify JSON format in IPC messages

## Contributing

1. Fork the repository
2. Create feature branch
3. Make changes with tests
4. Submit pull request

## License

MIT
