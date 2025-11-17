#!/bin/bash
set -e

echo "=== Setting up VisualIA ==="

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

# Initialize git submodules
echo -e "${BLUE}[1/2] Initializing git submodules...${NC}"
git submodule update --init --recursive

echo -e "${GREEN}✓ Submodules initialized${NC}"

# Run build
echo -e "${BLUE}[2/2] Running build...${NC}"
bash scripts/build.sh

echo ""
echo -e "${GREEN}=== Setup complete! ===${NC}"
