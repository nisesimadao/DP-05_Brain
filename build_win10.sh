#!/bin/bash

# Build script for Windows 10 (Desktop)
# Requires mingw-w64 environment (x86_64-w64-mingw32-g++)

SOURCE="main.cpp"
OUTPUT_DIR="./dist_win10"
OUTPUT="$OUTPUT_DIR/AppMain_win10.exe"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

echo "Building App for Windows 10..."

# Compile with x86_64 mingw compiler
# -DUNICODE -D_UNICODE for wide string support
# -mwindows for GUI app (no console)
# -municode for wWinMain support
# -static for standalone executable
x86_64-w64-mingw32-g++ -Wall -Wextra -O3 -mwindows -municode -static -s -o "$OUTPUT" "$SOURCE" -DUNICODE -D_UNICODE -lwinmm

if [ $? -eq 0 ]; then
    echo "Build successful! Output: $OUTPUT"
    
    # Generate BMP
    python3 make_bmp.py
    
    echo "Preparing assets..."
    rm -f "$OUTPUT_DIR"/*.ttf "$OUTPUT_DIR"/*.otf "$OUTPUT_DIR"/*.wav "$OUTPUT_DIR"/*.bmp "$OUTPUT_DIR"/*.png
    cp *.wav "$OUTPUT_DIR/"
    cp teno*.ttf "$OUTPUT_DIR/" 2>/dev/null || true
    cp cp_period.ttf "$OUTPUT_DIR/" 2>/dev/null || true
    cp icon.bmp "$OUTPUT_DIR/" 2>/dev/null || true
    cp icon.png "$OUTPUT_DIR/" 2>/dev/null || true
    
    # Also sync to Example folder
    echo "Syncing assets to Example folder..."
    rm -f ./Example/*.ttf ./Example/*.otf ./Example/*.wav ./Example/*.bmp ./Example/*.png
    cp *.wav ./Example/ 2>/dev/null || true
    cp teno*.ttf ./Example/ 2>/dev/null || true
    cp cp_period.ttf ./Example/ 2>/dev/null || true
    cp icon.bmp ./Example/ 2>/dev/null || true
    cp icon.png ./Example/ 2>/dev/null || true
    
    echo "------------------------------------------------"
    echo "Ready! You can find the executable and wav files in $OUTPUT_DIR"
    echo "------------------------------------------------"
else
    echo "Build failed."
    exit 1
fi
