#!/bin/bash
set -e

# VisualIA Build Script
# Builds C backend with whisper.cpp/llama.cpp and installs frontend dependencies

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Parse arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--debug] [--clean]"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}=== Building VisualIA ===${NC}"
echo ""

# ============================================================================
# [1/3] Build C Backend
# ============================================================================
echo -e "${BLUE}[1/3] Building C backend ($BUILD_TYPE mode)...${NC}"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$PROJECT_ROOT/build"
fi

# Create and enter build directory
mkdir -p "$PROJECT_ROOT/build"
cd "$PROJECT_ROOT/build"

# Detect number of CPU cores for parallel builds
if command -v nproc &> /dev/null; then
    CORES=$(nproc)
elif command -v sysctl &> /dev/null; then
    CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
else
    CORES=4
fi

echo "Using $CORES parallel jobs"

# Configure with CMake
echo "Configuring build with CMake..."
if ! cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. ; then
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
fi

# Build
echo "Building backend..."
if ! cmake --build . -j$CORES ; then
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

cd "$PROJECT_ROOT"

# Verify executable exists
if [ -f "$PROJECT_ROOT/build/visualia" ]; then
    EXECUTABLE_SIZE=$(du -h "$PROJECT_ROOT/build/visualia" | cut -f1)
    echo -e "${GREEN}✓ Backend built successfully ($EXECUTABLE_SIZE)${NC}"

    # Show build architecture
    if command -v file &> /dev/null; then
        ARCH_INFO=$(file "$PROJECT_ROOT/build/visualia" | sed 's/.*executable //' | cut -d',' -f1)
        echo "  Architecture: $ARCH_INFO"
    fi
else
    echo -e "${RED}✗ Backend executable not found${NC}"
    exit 1
fi

# ============================================================================
# [2/3] Install Frontend Dependencies
# ============================================================================
echo ""
echo -e "${BLUE}[2/3] Installing frontend dependencies...${NC}"

if [ ! -d "$PROJECT_ROOT/frontend" ]; then
    echo -e "${RED}✗ Frontend directory not found${NC}"
    exit 1
fi

cd "$PROJECT_ROOT/frontend"

# Install npm dependencies
if ! npm install --quiet; then
    echo -e "${RED}✗ npm install failed${NC}"
    exit 1
fi

cd "$PROJECT_ROOT"
echo -e "${GREEN}✓ Frontend dependencies installed${NC}"

# ============================================================================
# [3/3] Model Status Check
# ============================================================================
echo ""
echo -e "${BLUE}[3/3] Checking AI models...${NC}"

MODEL_DIR="$PROJECT_ROOT/models"
mkdir -p "$MODEL_DIR"

# Check for Whisper models
WHISPER_MODELS=$(ls -1 "$MODEL_DIR"/whisper-*.gguf 2>/dev/null || true)
if [ -z "$WHISPER_MODELS" ]; then
    echo -e "${YELLOW}⚠ No Whisper models found${NC}"
    echo ""
    echo "  Download required for speech recognition:"
    echo -e "    ${BLUE}wget -O models/whisper-base.gguf \\${NC}"
    echo -e "      ${BLUE}https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin${NC}"
    echo ""
    echo "  Or run setup to download interactively:"
    echo -e "    ${BLUE}bash scripts/setup.sh${NC}"
else
    echo -e "${GREEN}✓ Whisper models:${NC}"
    for model in $WHISPER_MODELS; do
        MODEL_SIZE=$(du -h "$model" 2>/dev/null | cut -f1)
        MODEL_NAME=$(basename "$model")
        echo "  - $MODEL_NAME ($MODEL_SIZE)"
    done
fi

# Check for translation models (MADLAD or MT5)
MADLAD_MODELS=$(ls -1 "$MODEL_DIR"/madlad*.gguf 2>/dev/null || true)
MT5_MODELS=$(ls -1 "$MODEL_DIR"/mt5-*.gguf 2>/dev/null || true)

if [ -n "$MADLAD_MODELS" ]; then
    echo -e "${GREEN}✓ Translation models (MADLAD-400):${NC}"
    for model in $MADLAD_MODELS; do
        MODEL_SIZE=$(du -h "$model" 2>/dev/null | cut -f1)
        MODEL_NAME=$(basename "$model")
        echo "  - $MODEL_NAME ($MODEL_SIZE)"
    done
elif [ -n "$MT5_MODELS" ]; then
    echo -e "${YELLOW}⚠ Translation models (MT5):${NC}"
    for model in $MT5_MODELS; do
        MODEL_SIZE=$(du -h "$model" 2>/dev/null | cut -f1)
        MODEL_NAME=$(basename "$model")
        echo "  - $MODEL_NAME ($MODEL_SIZE)"
    done
    echo ""
    echo -e "${YELLOW}  Note: MT5 base models may not work for translation.${NC}"
    echo "  Consider using MADLAD-400 instead:"
    echo -e "    ${BLUE}bash scripts/setup_madlad_translation.sh${NC}"
else
    echo -e "${YELLOW}⚠ No translation models found (optional)${NC}"
    echo ""
    echo "  To enable translation, run:"
    echo -e "    ${BLUE}bash scripts/setup_madlad_translation.sh${NC}"
fi

# ============================================================================
# Build Summary
# ============================================================================
echo ""
echo -e "${GREEN}=== Build complete! ===${NC}"
echo ""
echo "Build artifacts:"
echo "  Backend:  build/visualia"
echo "  Frontend: frontend/node_modules/"
echo ""
echo "To run VisualIA:"
echo -e "  ${BLUE}cd frontend && npm start${NC}"
echo ""
echo "Or use:"
echo -e "  ${BLUE}npm start --prefix frontend${NC}"
echo ""
