#!/bin/bash
set -e

# VisualIA Comprehensive Setup Script
# Supports: macOS, Ubuntu/Debian, Fedora/RHEL

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

echo -e "${BLUE}=== VisualIA Setup ===${NC}"
echo ""

# ============================================================================
# [1/7] Platform Detection
# ============================================================================
echo -e "${BLUE}[1/7] Detecting platform...${NC}"

if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
    echo -e "${GREEN}✓ Platform: macOS${NC}"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        if [[ "$ID" == "ubuntu" ]] || [[ "$ID" == "debian" ]] || [[ "$ID_LIKE" == *"debian"* ]]; then
            PLATFORM="debian"
            echo -e "${GREEN}✓ Platform: Linux (Debian/Ubuntu)${NC}"
        elif [[ "$ID" == "fedora" ]] || [[ "$ID" == "rhel" ]] || [[ "$ID_LIKE" == *"fedora"* ]]; then
            PLATFORM="fedora"
            echo -e "${GREEN}✓ Platform: Linux (Fedora/RHEL)${NC}"
        else
            PLATFORM="linux"
            echo -e "${YELLOW}⚠ Platform: Linux (Generic - ${ID})${NC}"
        fi
    else
        PLATFORM="linux"
        echo -e "${YELLOW}⚠ Platform: Linux (Generic)${NC}"
    fi
else
    echo -e "${RED}✗ Unsupported platform: $OSTYPE${NC}"
    exit 1
fi

# ============================================================================
# [2/7] Check and Install System Dependencies
# ============================================================================
echo ""
echo -e "${BLUE}[2/7] Checking system dependencies...${NC}"

MISSING_DEPS=()

# Check CMake
if ! command -v cmake &> /dev/null; then
    MISSING_DEPS+=("cmake")
    echo -e "${YELLOW}⚠ CMake not found${NC}"
else
    CMAKE_VERSION=$(cmake --version | head -1 | awk '{print $3}')
    echo -e "${GREEN}✓ CMake $CMAKE_VERSION${NC}"
fi

# Check C/C++ Compiler
if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
    if [[ "$PLATFORM" == "macos" ]]; then
        MISSING_DEPS+=("xcode-tools")
    else
        MISSING_DEPS+=("build-essential")
    fi
    echo -e "${YELLOW}⚠ C/C++ compiler not found${NC}"
else
    if command -v clang &> /dev/null; then
        COMPILER_VERSION=$(clang --version | head -1)
    else
        COMPILER_VERSION=$(gcc --version | head -1)
    fi
    echo -e "${GREEN}✓ Compiler: ${COMPILER_VERSION}${NC}"
fi

# Check Git
if ! command -v git &> /dev/null; then
    MISSING_DEPS+=("git")
    echo -e "${YELLOW}⚠ Git not found${NC}"
else
    GIT_VERSION=$(git --version | awk '{print $3}')
    echo -e "${GREEN}✓ Git ${GIT_VERSION}${NC}"
fi

# Check Node.js
if ! command -v node &> /dev/null; then
    MISSING_DEPS+=("nodejs")
    echo -e "${YELLOW}⚠ Node.js not found${NC}"
else
    NODE_VERSION=$(node --version)
    echo -e "${GREEN}✓ Node.js ${NODE_VERSION}${NC}"
fi

# Check wget or curl (for model downloads)
if ! command -v wget &> /dev/null && ! command -v curl &> /dev/null; then
    MISSING_DEPS+=("wget")
    echo -e "${YELLOW}⚠ wget/curl not found${NC}"
else
    if command -v wget &> /dev/null; then
        echo -e "${GREEN}✓ wget available${NC}"
    else
        echo -e "${GREEN}✓ curl available${NC}"
    fi
fi

# Platform-specific checks
if [[ "$PLATFORM" == "debian" ]]; then
    # Check for PulseAudio development libraries
    if ! pkg-config --exists libpulse-simple 2>/dev/null; then
        MISSING_DEPS+=("libpulse-dev" "pulseaudio")
        echo -e "${YELLOW}⚠ PulseAudio development libraries not found${NC}"
    else
        echo -e "${GREEN}✓ PulseAudio development libraries${NC}"
    fi
elif [[ "$PLATFORM" == "fedora" ]]; then
    # Check for PulseAudio development libraries
    if ! pkg-config --exists libpulse-simple 2>/dev/null; then
        MISSING_DEPS+=("pulseaudio-libs-devel")
        echo -e "${YELLOW}⚠ PulseAudio development libraries not found${NC}"
    else
        echo -e "${GREEN}✓ PulseAudio development libraries${NC}"
    fi
fi

# Install missing dependencies
if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}Missing dependencies detected. Installing...${NC}"

    if [[ "$PLATFORM" == "macos" ]]; then
        # Check for Homebrew
        if ! command -v brew &> /dev/null; then
            echo -e "${BLUE}Installing Homebrew...${NC}"
            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

            # Add Homebrew to PATH for this session
            if [[ -f /opt/homebrew/bin/brew ]]; then
                eval "$(/opt/homebrew/bin/brew shellenv)"
            elif [[ -f /usr/local/bin/brew ]]; then
                eval "$(/usr/local/bin/brew shellenv)"
            fi
        fi

        # Install Xcode Command Line Tools if needed
        if [[ " ${MISSING_DEPS[@]} " =~ " xcode-tools " ]]; then
            echo -e "${BLUE}Installing Xcode Command Line Tools...${NC}"
            xcode-select --install 2>/dev/null || true
            echo -e "${YELLOW}Please complete the Xcode tools installation and re-run this script.${NC}"
            exit 1
        fi

        # Install other dependencies via Homebrew
        for dep in "${MISSING_DEPS[@]}"; do
            if [[ "$dep" != "xcode-tools" ]]; then
                echo -e "${BLUE}Installing ${dep}...${NC}"
                brew install "$dep"
            fi
        done

    elif [[ "$PLATFORM" == "debian" ]]; then
        echo -e "${BLUE}Installing dependencies with apt-get...${NC}"
        sudo apt-get update

        # Map package names to Debian equivalents
        DEBIAN_PACKAGES=()
        for dep in "${MISSING_DEPS[@]}"; do
            case "$dep" in
                "build-essential") DEBIAN_PACKAGES+=("build-essential") ;;
                "cmake") DEBIAN_PACKAGES+=("cmake") ;;
                "git") DEBIAN_PACKAGES+=("git") ;;
                "wget") DEBIAN_PACKAGES+=("wget") ;;
                "nodejs") DEBIAN_PACKAGES+=("curl");;
                "libpulse-dev") DEBIAN_PACKAGES+=("libpulse-dev") ;;
                "pulseaudio") DEBIAN_PACKAGES+=("pulseaudio") ;;
            esac
        done

        if [ ${#DEBIAN_PACKAGES[@]} -gt 0 ]; then
            sudo apt-get install -y "${DEBIAN_PACKAGES[@]}"
        fi

        # Install Node.js via NodeSource if needed
        if [[ " ${MISSING_DEPS[@]} " =~ " nodejs " ]]; then
            echo -e "${BLUE}Installing Node.js via NodeSource...${NC}"
            curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
            sudo apt-get install -y nodejs
        fi

    elif [[ "$PLATFORM" == "fedora" ]]; then
        echo -e "${BLUE}Installing dependencies with dnf...${NC}"

        # Map package names to Fedora equivalents
        FEDORA_PACKAGES=()
        for dep in "${MISSING_DEPS[@]}"; do
            case "$dep" in
                "build-essential") FEDORA_PACKAGES+=("gcc" "gcc-c++" "make") ;;
                "cmake") FEDORA_PACKAGES+=("cmake") ;;
                "git") FEDORA_PACKAGES+=("git") ;;
                "wget") FEDORA_PACKAGES+=("wget") ;;
                "nodejs") FEDORA_PACKAGES+=("nodejs" "npm") ;;
                "pulseaudio-libs-devel") FEDORA_PACKAGES+=("pulseaudio-libs-devel") ;;
            esac
        done

        if [ ${#FEDORA_PACKAGES[@]} -gt 0 ]; then
            sudo dnf install -y "${FEDORA_PACKAGES[@]}"
        fi
    fi

    echo -e "${GREEN}✓ Dependencies installed${NC}"
else
    echo -e "${GREEN}✓ All dependencies satisfied${NC}"
fi

# ============================================================================
# [3/7] Initialize Git Submodules
# ============================================================================
echo ""
echo -e "${BLUE}[3/7] Initializing git submodules...${NC}"
echo "This will clone whisper.cpp and llama.cpp libraries"

git submodule update --init --recursive

echo -e "${GREEN}✓ Submodules initialized${NC}"

# ============================================================================
# [4/7] Build Backend and Frontend
# ============================================================================
echo ""
echo -e "${BLUE}[4/7] Building VisualIA...${NC}"

# Run build script
bash "$SCRIPT_DIR/build.sh"

# ============================================================================
# [5/7] Whisper Model Setup
# ============================================================================
echo ""
echo -e "${BLUE}[5/7] Whisper model setup${NC}"
echo -e "${YELLOW}Which Whisper model(s) would you like to download?${NC}"
echo "Whisper is used for speech recognition (99 languages)"
echo ""
echo "Options:"
echo "  1) whisper-base      (~141MB, recommended - good quality, fast)"
echo "  2) whisper-small     (~466MB, better quality)"
echo "  3) whisper-medium    (~769MB, great quality)"
echo "  4) whisper-large-v3  (~1.5GB, best quality)"
echo "  5) Multiple models   (select multiple)"
echo "  6) Skip (download manually later)"
echo ""
read -p "Enter choice [1-6] (default: 1): " whisper_choice
whisper_choice=${whisper_choice:-1}

MODELS_DIR="$PROJECT_ROOT/models"
mkdir -p "$MODELS_DIR"

# Function to download a model
download_whisper_model() {
    local model_name=$1
    local model_file=$2
    local model_url=$3

    if [ -f "$MODELS_DIR/$model_file" ]; then
        echo -e "${YELLOW}⚠ $model_file already exists, skipping${NC}"
        return
    fi

    echo -e "${BLUE}Downloading $model_name...${NC}"

    if command -v wget &> /dev/null; then
        wget --progress=bar:force -O "$MODELS_DIR/$model_file" "$model_url" 2>&1
    elif command -v curl &> /dev/null; then
        curl -L --progress-bar -o "$MODELS_DIR/$model_file" "$model_url"
    else
        echo -e "${RED}✗ Neither wget nor curl available for download${NC}"
        exit 1
    fi

    echo -e "${GREEN}✓ $model_file installed${NC}"
}

case $whisper_choice in
    1)
        download_whisper_model "whisper-base" "whisper-base.gguf" \
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin"
        ;;
    2)
        download_whisper_model "whisper-small" "whisper-small.gguf" \
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin"
        ;;
    3)
        download_whisper_model "whisper-medium" "whisper-medium.gguf" \
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin"
        ;;
    4)
        download_whisper_model "whisper-large-v3" "whisper-large-v3.gguf" \
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin"
        ;;
    5)
        echo ""
        echo "Select models to download (space-separated, e.g., '1 2 4'):"
        echo "  1) base  2) small  3) medium  4) large-v3"
        read -p "Enter selections: " multi_choice

        for choice in $multi_choice; do
            case $choice in
                1) download_whisper_model "whisper-base" "whisper-base.gguf" \
                       "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin" ;;
                2) download_whisper_model "whisper-small" "whisper-small.gguf" \
                       "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin" ;;
                3) download_whisper_model "whisper-medium" "whisper-medium.gguf" \
                       "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin" ;;
                4) download_whisper_model "whisper-large-v3" "whisper-large-v3.gguf" \
                       "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin" ;;
            esac
        done
        ;;
    *)
        echo -e "${YELLOW}⏭ Skipping Whisper model download${NC}"
        echo "You can download manually:"
        echo "  wget -O models/whisper-base.gguf https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin"
        ;;
esac

# ============================================================================
# [6/7] Translation Setup (Optional)
# ============================================================================
echo ""
echo -e "${BLUE}[6/7] Translation setup (optional)${NC}"
echo -e "${YELLOW}Do you want to set up live translation?${NC}"
echo "This enables real-time translation between 400+ languages using MADLAD-400"
echo ""
echo "Options:"
echo "  1) Skip (transcription only)"
echo "  2) Install MADLAD-400-3b  (~1.7GB, recommended)"
echo "  3) Install MADLAD-400-7b  (~4.0GB, better quality)"
echo "  4) Install MADLAD-400-10b (~5.5GB, best quality)"
echo ""
echo -e "${YELLOW}Note: Requires Python 3 and will install torch, transformers (~2GB)${NC}"
read -p "Enter choice [1-4] (default: 1): " translation_choice
translation_choice=${translation_choice:-1}

case $translation_choice in
    2)
        echo -e "${BLUE}Installing MADLAD-400-3b...${NC}"
        if [ -f "$SCRIPT_DIR/setup_madlad_translation.sh" ]; then
            bash "$SCRIPT_DIR/setup_madlad_translation.sh"
        else
            echo -e "${RED}✗ setup_madlad_translation.sh not found${NC}"
        fi
        ;;
    3|4)
        echo -e "${BLUE}Installing MADLAD-400 (custom size)...${NC}"
        if [ -f "$SCRIPT_DIR/setup_madlad_translation.sh" ]; then
            bash "$SCRIPT_DIR/setup_madlad_translation.sh"
        else
            echo -e "${RED}✗ setup_madlad_translation.sh not found${NC}"
        fi
        ;;
    *)
        echo -e "${YELLOW}⏭ Skipping translation setup${NC}"
        echo "You can run it later with:"
        echo "  bash scripts/setup_madlad_translation.sh"
        ;;
esac

# ============================================================================
# [7/7] Verification
# ============================================================================
echo ""
echo -e "${BLUE}[7/7] Verifying installation...${NC}"

ERRORS=0

# Check backend executable
if [ -f "$PROJECT_ROOT/build/visualia" ]; then
    echo -e "${GREEN}✓ Backend executable: build/visualia${NC}"
else
    echo -e "${RED}✗ Backend executable not found${NC}"
    ERRORS=$((ERRORS + 1))
fi

# Check frontend dependencies
if [ -d "$PROJECT_ROOT/frontend/node_modules" ]; then
    echo -e "${GREEN}✓ Frontend dependencies installed${NC}"
else
    echo -e "${RED}✗ Frontend dependencies missing${NC}"
    ERRORS=$((ERRORS + 1))
fi

# Check for at least one Whisper model
WHISPER_COUNT=$(ls -1 "$MODELS_DIR"/whisper-*.gguf 2>/dev/null | wc -l)
if [ "$WHISPER_COUNT" -gt 0 ]; then
    echo -e "${GREEN}✓ Whisper models found ($WHISPER_COUNT)${NC}"
    ls -1 "$MODELS_DIR"/whisper-*.gguf 2>/dev/null | sed 's|.*/|  - |'
else
    echo -e "${YELLOW}⚠ No Whisper models found (download manually or re-run setup)${NC}"
fi

# Check for translation models
TRANSLATION_COUNT=$(ls -1 "$MODELS_DIR"/madlad*.gguf 2>/dev/null | wc -l)
if [ "$TRANSLATION_COUNT" -gt 0 ]; then
    echo -e "${GREEN}✓ Translation models found ($TRANSLATION_COUNT)${NC}"
    ls -1 "$MODELS_DIR"/madlad*.gguf 2>/dev/null | sed 's|.*/|  - |'
fi

# Final status
echo ""
if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}=== Setup complete! ===${NC}"
    echo ""
    echo "To run VisualIA:"
    echo -e "  ${BLUE}cd frontend && npm start${NC}"
    echo ""
    if [[ "$PLATFORM" == "macos" ]]; then
        echo "Don't forget to grant microphone permissions:"
        echo "  System Preferences → Security & Privacy → Privacy → Microphone"
    fi
else
    echo -e "${YELLOW}=== Setup completed with $ERRORS error(s) ===${NC}"
    echo "Please review the errors above and fix manually"
fi
echo ""
