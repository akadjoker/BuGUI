#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if ! command -v doxygen &>/dev/null; then
    echo "doxygen not found. Install with: sudo apt install doxygen"
    exit 1
fi

echo "Generating Doxygen documentation..."
doxygen Doxyfile
echo "Done → docs/html/index.html"
