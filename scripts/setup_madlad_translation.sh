#!/bin/bash
set -e

echo "=== Setting up MADLAD-400 Translation Model ==="
echo ""
echo "MADLAD-400 is MT5 fine-tuned specifically for translation."
echo "It supports 400+ languages and produces actual translations"
echo "(unlike MT5-base which only does pre-training tasks)."
echo ""

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MODELS_DIR="$PROJECT_ROOT/models"

mkdir -p "$MODELS_DIR"

# Model selection
echo -e "${BLUE}Select MADLAD-400 model size:${NC}"
echo "  1) madlad400-3b-mt  (~1.7GB, good quality)"
echo "  2) madlad400-7b-mt  (~4.0GB, better quality)"
echo "  3) madlad400-10b-mt (~5.5GB, best quality)"
echo ""
read -p "Enter choice [1-3] (default: 1): " choice
choice=${choice:-1}

case $choice in
    1)
        MODEL_NAME="madlad400-3b-mt"
        HF_REPO="google/madlad400-3b-mt"
        ;;
    2)
        MODEL_NAME="madlad400-7b-mt"
        HF_REPO="google/madlad400-7b-mt"
        ;;
    3)
        MODEL_NAME="madlad400-10b-mt"
        HF_REPO="google/madlad400-10b-mt"
        ;;
    *)
        echo -e "${RED}Invalid choice${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${YELLOW}Note: MADLAD-400 models need to be converted to GGUF format.${NC}"
echo -e "${YELLOW}This requires:"
echo "  1. Downloading the model from HuggingFace"
echo "  2. Converting to GGUF format using llama.cpp tools"
echo ""
echo -e "${BLUE}Step 1: Install HuggingFace Hub${NC}"

# Check if huggingface_hub is installed
if ! python3 -c "import huggingface_hub" &> /dev/null; then
    echo "Installing huggingface_hub..."
    pip3 install -U huggingface_hub
fi

echo -e "${GREEN}✓ HuggingFace Hub installed${NC}"
echo ""

# Download model
echo -e "${BLUE}Step 2: Downloading $MODEL_NAME from HuggingFace...${NC}"
echo "This may take a while depending on your internet connection."
echo ""

CACHE_DIR="$MODELS_DIR/huggingface_cache"
mkdir -p "$CACHE_DIR"

python3 -c "
from huggingface_hub import snapshot_download
snapshot_download(
    repo_id='$HF_REPO',
    local_dir='$CACHE_DIR/$MODEL_NAME',
    local_dir_use_symlinks=False
)
"

echo -e "${GREEN}✓ Model downloaded${NC}"
echo ""

# Convert to GGUF
echo -e "${BLUE}Step 3: Converting to GGUF format...${NC}"

LLAMA_CPP_DIR="$PROJECT_ROOT/backend/libs/llama.cpp"
CONVERT_SCRIPT="$LLAMA_CPP_DIR/convert_hf_to_gguf.py"

if [ ! -f "$CONVERT_SCRIPT" ]; then
    echo -e "${RED}Error: llama.cpp convert script not found${NC}"
    echo "Expected: $CONVERT_SCRIPT"
    exit 1
fi

# Install Python dependencies for conversion
echo "Installing conversion dependencies..."
pip3 install -U torch transformers sentencepiece protobuf

# Convert
python3 "$CONVERT_SCRIPT" \
    "$CACHE_DIR/$MODEL_NAME" \
    --outfile "$MODELS_DIR/$MODEL_NAME.gguf" \
    --outtype f16

echo -e "${GREEN}✓ Model converted to GGUF${NC}"
echo ""

# Cleanup (optional)
echo -e "${BLUE}Cleanup${NC}"
read -p "Remove HuggingFace cache (~save disk space)? [y/N]: " cleanup
if [[ $cleanup =~ ^[Yy]$ ]]; then
    rm -rf "$CACHE_DIR/$MODEL_NAME"
    echo -e "${GREEN}✓ Cache cleaned${NC}"
fi

echo ""
echo -e "${GREEN}=== Setup complete! ===${NC}"
echo ""
echo "MADLAD-400 model installed:"
echo "  $MODELS_DIR/$MODEL_NAME.gguf"
echo ""
echo "To use this model, update your translation settings to use:"
echo "  $MODEL_NAME"
echo ""
