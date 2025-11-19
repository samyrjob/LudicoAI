#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MODELS_DIR="$PROJECT_ROOT/models"

echo "============================================"
echo "VisualIA T5 Translation Model Setup"
echo "============================================"
echo ""
echo "⚠️  WARNING: DEPRECATED"
echo ""
echo "MT5 base models are NOT fine-tuned for translation and"
echo "generate sentinel tokens (<extra_id_0>) instead of translations."
echo ""
echo "RECOMMENDED: Use MADLAD-400 models instead, which are"
echo "MT5 models fine-tuned specifically for translation:"
echo ""
echo "  bash scripts/setup_madlad_translation.sh"
echo ""
echo "This script is kept for reference only."
echo "============================================"
echo ""
read -p "Continue anyway with MT5 setup? [y/N] " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborting. Please use setup_madlad_translation.sh instead."
    exit 0
fi
echo ""

# Create models directory
mkdir -p "$MODELS_DIR"

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "Error: python3 is required for model conversion"
    echo "Please install Python 3"
    exit 1
fi

# Check if Hugging Face CLI is installed
if ! command -v huggingface-cli &> /dev/null; then
    echo "Installing Hugging Face CLI..."
    pip3 install -U "huggingface_hub[cli]"
fi

# Model selection
echo "Available T5 models for translation:"
echo ""
echo "1. google-t5/t5-small      (~242MB)  - Fast, basic quality"
echo "2. google/mt5-small        (~300MB)  - Multilingual, recommended"
echo "3. google/mt5-base         (~580MB)  - Better quality"
echo "4. google/flan-t5-small    (~308MB)  - Instruction-tuned"
echo ""
echo "Recommended: mt5-small (multilingual, good balance)"
echo ""

# Default to mt5-small
MODEL_NAME=${1:-"google/mt5-small"}
MODEL_SHORT=$(basename "$MODEL_NAME")
GGUF_PATH="$MODELS_DIR/${MODEL_SHORT}.gguf"

# Check if model already exists
if [ -f "$GGUF_PATH" ]; then
    echo "Model already exists: $GGUF_PATH"
    echo ""
    read -p "Re-download and convert? [y/N] " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Using existing model."
        exit 0
    fi
fi

echo "Downloading $MODEL_NAME..."
echo ""

# Download model from Hugging Face
HF_CACHE="$MODELS_DIR/.hf_cache"
mkdir -p "$HF_CACHE"

huggingface-cli download "$MODEL_NAME" \
    --local-dir "$HF_CACHE/$MODEL_SHORT" \
    --local-dir-use-symlinks False

echo ""
echo "Model downloaded to: $HF_CACHE/$MODEL_SHORT"

# Check if conversion script exists
CONVERT_SCRIPT="$PROJECT_ROOT/backend/libs/llama.cpp/convert_hf_to_gguf.py"

if [ ! -f "$CONVERT_SCRIPT" ]; then
    echo "Error: Conversion script not found at $CONVERT_SCRIPT"
    echo "Make sure llama.cpp submodule is initialized:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

# Install Python dependencies for conversion
echo ""
echo "Installing conversion dependencies..."
pip3 install -q -U torch numpy sentencepiece transformers protobuf

# Convert to GGUF format
echo ""
echo "Converting to GGUF format..."
echo "This may take a few minutes..."
echo ""

python3 "$CONVERT_SCRIPT" \
    "$HF_CACHE/$MODEL_SHORT" \
    --outfile "$GGUF_PATH" \
    --outtype q8_0

if [ ! -f "$GGUF_PATH" ]; then
    echo ""
    echo "Error: Conversion failed. GGUF file not created."
    exit 1
fi

# Get file size
FILE_SIZE=$(du -h "$GGUF_PATH" | cut -f1)

echo ""
echo "============================================"
echo "✅ T5 Translation Model Ready!"
echo "============================================"
echo ""
echo "Model: $MODEL_NAME"
echo "Location: $GGUF_PATH"
echo "Size: $FILE_SIZE"
echo ""
echo "To use this model, set the environment variable:"
echo "  export VISUALIA_TRANSLATION_MODEL=\"$GGUF_PATH\""
echo ""
echo "Or the backend will auto-detect models in the models/ directory"
echo ""

# Cleanup HuggingFace cache (optional)
read -p "Delete HuggingFace cache to save space? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -rf "$HF_CACHE/$MODEL_SHORT"
    echo "Cache deleted."
fi

echo ""
echo "Setup complete!"
