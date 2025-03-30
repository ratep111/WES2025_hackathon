#!/bin/bash

set -e

# Define color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for clang-format
echo -e "${BLUE}Checking for clang-format...${NC}"
if ! command -v clang-format &> /dev/null; then
    echo -e "${YELLOW}clang-format not found. Attempting to install...${NC}"
    if command -v apt &>/dev/null; then
        sudo apt update && sudo apt install -y clang-format
    else
        echo -e "${RED}Error: apt not found. Please install clang-format manually.${NC}" >&2
        exit 1
    fi
else
    echo -e "${GREEN}clang-format is already installed.${NC}"
fi

# Initialize Github hooks
echo -e "${BLUE}Initializing pre-commit hook to run formatter...${NC}"
cp scripts/pre-commit-formatter.sh .git/hooks/pre-commit

# Initialize and download all submodules
echo -e "${BLUE}Initializing and updating submodules...${NC}"
if git submodule update --init --recursive; then
    echo -e "${GREEN}Submodules updated successfully.${NC}"
else
    echo -e "${RED}Error: Failed to update submodules.${NC}" >&2
    exit 1
fi

# Apply patch for ESP LVGL drivers to work with BL devkit
echo -e "${BLUE}Applying patch for ESP LVGL drivers...${NC}"
if cd components/lvgl_esp32_drivers && git apply ../lvgl_esp32_drivers_8-3.patch; then
    echo -e "${GREEN}Patch applied successfully.${NC}"
else
    echo -e "${RED}Error: Failed to apply patch.${NC}" >&2
    exit 1
fi
cd ../..

# Apply patch for SHT drivers to work with BL devkit
echo -e "${BLUE}Applying patch for SHT drivers...${NC}"
if cd components/esp32-sht3x && git apply ../esp32-sht3x.patch; then
    echo -e "${GREEN}Patch applied successfully.${NC}"
else
    echo -e "${RED}Error: Failed to apply patch.${NC}" >&2
    exit 1
fi
cd ../..

# Prepare sdkconfig for BL devkit
echo -e "${BLUE}Copying sdkconfig for BL devkit...${NC}"
if cp sdkconfig.defaults sdkconfig; then
    echo -e "${GREEN}sdkconfig prepared successfully.${NC}"
else
    echo -e "${RED}Error: Failed to copy sdkconfig.${NC}" >&2
    exit 1
fi

echo -e "${GREEN}✅ Project ready for build!${NC}"