#!/usr/bin/env bash

set -euo pipefail

REPO_URL="https://github.com/MkP369/idu.git"
INSTALL_DIR="$HOME/.local/bin"
TMP_DIR=$(mktemp -d)

cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

echo "Checking dependencies..."
for cmd in git cmake c++ mold; do
    if ! command -v $cmd &> /dev/null; then
        echo "Error: '$cmd' is not installed."
        exit 1
    fi
done

echo "Cloning repo..."
git clone --depth=1 "$REPO_URL" "$TMP_DIR"
cd "$TMP_DIR"

echo "Compiling"
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release -j$(nproc)

echo "Installing to $INSTALL_DIR..."
mkdir -p "$INSTALL_DIR"
mv "build_release/idu" "$INSTALL_DIR/"

echo "✅ idu Successfully installed! run 'idu -h' to get started"

if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
    echo "⚠️ WARNING: $INSTALL_DIR is not in your PATH."
fi

