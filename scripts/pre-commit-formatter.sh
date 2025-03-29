#!/bin/bash

INCLUDED_DIRS=(
  "components"
  "main"
)

# Subdirectories to exclude
EXCLUDED_DIRS=(
  "components/lvgl"
  "components/lvgl_esp32_drivers"
)

# Find all staged C/H files in included directories
FILES=$(git diff --cached --name-only --diff-filter=d -- "${INCLUDED_DIRS[@]}" | grep -E '\.(c|h)$')

# Filter out excluded directories
for exclude in "${EXCLUDED_DIRS[@]}"; do
  FILES=$(echo "$FILES" | grep -v "^$exclude/")
done

if [ -z "$FILES" ]; then
  echo "No matching C/H files to format (or all excluded)"
  exit 0
fi

echo "Running clang-format on:"
echo "$FILES"

# Check formatting (dry-run first)
echo "$FILES" | xargs clang-format --dry-run --Werror --style=file
if [ $? -ne 0 ]; then
  echo "Formatting errors detected!"
  echo "$FILES"
fi

echo "Running formatter..."
echo "$FILES" | xargs clang-format -i --style=file
echo "$FILES" | xargs git add