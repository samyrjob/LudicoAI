#!/bin/bash
set -e

echo "=== Building VisualIA ==="

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

# Build backend
echo -e "${BLUE}[1/3] Building C backend...${NC}"
mkdir -p build
cd build
cmake ..
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cd ..

echo -e "${GREEN}✓ Backend built successfully${NC}"

# Install frontend dependencies
echo -e "${BLUE}[2/3] Installing frontend dependencies...${NC}"
cd frontend
npm install
cd ..

echo -e "${GREEN}✓ Frontend dependencies installed${NC}"

# Download model (if not exists)
echo -e "${BLUE}[3/3] Checking for Whisper model...${NC}"
MODEL_DIR="models"
MODEL_FILE="whisper-base.en.gguf"

if [ ! -f "$MODEL_DIR/$MODEL_FILE" ]; then
    echo "Whisper model not found."
    echo "Please download from: https://huggingface.co/ggerganov/whisper.cpp/tree/main"
    echo "Place the model in: $MODEL_DIR/$MODEL_FILE"
    echo ""
    echo "Suggested command:"
    echo "  wget -P models https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"
    echo "  mv models/ggml-base.en.bin models/whisper-base.en.gguf"
else
    echo -e "${GREEN}✓ Model found: $MODEL_DIR/$MODEL_FILE${NC}"
fi

echo ""
echo -e "${GREEN}=== Build complete! ===${NC}"
echo ""
echo "To run:"
echo "  npm start --prefix frontend"
echo ""
