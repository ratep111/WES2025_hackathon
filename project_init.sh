#!/usr/bin/env bash

set -e

# Define color codes that work in Fish/Tilix
RED='\e[31m'
GREEN='\e[32m'
BLUE='\e[34m'
YELLOW='\e[33m'
BOLD='\e[1m'
RESET='\e[0m'

# Function to check if patch is already applied
is_patch_applied() {
    local patch_file="$1"
    local target_dir="$2"
    
    # Check if reverse apply would work (meaning patch is already applied)
    if git -C "$target_dir" apply --reverse --check "$patch_file" &>/dev/null; then
        return 0  # Patch is applied
    else
        return 1  # Patch is not applied
    fi
}

# Check for clang-format
echo -e "${BLUE}Checking for clang-format...${RESET}"
if ! command -v clang-format &>/dev/null; then
    echo -e "${YELLOW}clang-format not found. Attempting to install...${RESET}"
    if command -v apt &>/dev/null; then
        sudo apt update && sudo apt install -y clang-format
    else
        echo -e "${RED}Error: apt not found. Please install clang-format manually.${RESET}" >&2
        exit 1
    fi
else
    echo -e "${GREEN}clang-format is already installed.${RESET}"
fi

# Initialize Github hooks
echo -e "${BLUE}Initializing pre-commit hook to run formatter...${RESET}"
cp scripts/pre-commit-formatter.sh .git/hooks/pre-commit

# Initialize and download all submodules
echo -e "${BLUE}Initializing and updating submodules...${RESET}"
if git submodule update --init --recursive; then
    echo -e "${GREEN}Submodules updated successfully.${RESET}"
else
    echo -e "${RED}Error: Failed to update submodules.${RESET}" >&2
    exit 1
fi

# Handle LVGL drivers patch
LVGL_PATCH="../lvgl_esp32_drivers_8-3.patch"
LVGL_DIR="components/lvgl_esp32_drivers"
echo -e "${BLUE}Checking LVGL drivers patch status...${RESET}"
if is_patch_applied "$LVGL_PATCH" "$LVGL_DIR"; then
    echo -e "${GREEN}LVGL drivers patch is already applied.${RESET}"
else
    echo -e "${YELLOW}Applying LVGL drivers patch...${RESET}"
    if (cd "$LVGL_DIR" && git apply "$LVGL_PATCH"); then
        echo -e "${GREEN}Successfully applied LVGL drivers patch.${RESET}"
    else
        echo -e "${RED}Error: Failed to apply LVGL drivers patch.${RESET}" >&2
        exit 1
    fi
fi

# Handle SHT drivers patch
SHT_PATCH="../esp32-sht3x.patch"
SHT_DIR="components/esp32-sht3x"
echo -e "${BLUE}Checking SHT drivers patch status...${RESET}"
if is_patch_applied "$SHT_PATCH" "$SHT_DIR"; then
    echo -e "${GREEN}SHT drivers patch is already applied.${RESET}"
else
    echo -e "${YELLOW}Applying SHT drivers patch...${RESET}"
    if (cd "$SHT_DIR" && git apply "$SHT_PATCH"); then
        echo -e "${GREEN}Successfully applied SHT drivers patch.${RESET}"
    else
        echo -e "${RED}Error: Failed to apply SHT drivers patch.${RESET}" >&2
        exit 1
    fi
fi

# Prepare sdkconfig
echo -e "${BLUE}Preparing sdkconfig...${RESET}"
if [ -f "sdkconfig" ] && cmp -s "sdkconfig.defaults" "sdkconfig"; then
    echo -e "${GREEN}sdkconfig is already up-to-date.${RESET}"
else
    if cp sdkconfig.defaults sdkconfig; then
        echo -e "${GREEN}Successfully prepared sdkconfig.${RESET}"
    else
        echo -e "${RED}Error: Failed to prepare sdkconfig.${RESET}" >&2
        exit 1
    fi
fi

echo -e "${GREEN}${BOLD}✓ Project setup completed successfully!${RESET}"