#!/bin/bash

# Initialize Github hooks
echo "Initializing pre-commit hook to run formatter..."
cp scripts/pre-commit-formatter.sh .git/hooks/pre-commit


# Initialize and download all submodules
echo "Initializing and updating submodules..."
if git submodule update --init --recursive; then
    echo "Submodules updated successfully."
else
    echo "Error: Failed to update submodules." >&2
    exit 1
fi

# Apply patch for ESP LVGL drivers to work with BL devkit
echo "Applying patch for ESP LVGL drivers..."
if cd components/lvgl_esp32_drivers && git apply ../lvgl_esp32_drivers_8-3.patch; then
    echo "Patch applied successfully."
else
    echo "Error: Failed to apply patch." >&2
    exit 1
fi
cd ../..

# Prepare sdkconfig for BL devkit
echo "Copying sdkconfig for BL devkit..."
if cp sdkconfig.defaults sdkconfig; then
    echo "sdkconfig prepared successfully."
else
    echo "Error: Failed to copy sdkconfig." >&2
    exit 1
fi

echo "Project ready for build!"
