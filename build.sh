#!/bin/bash

# Build script for Brain PW-SH2 Apps
# Requires CeGCC environment

SOURCE="main.cpp"
OUTPUT="./Example/AppMain.exe"

# Create output directory if it doesn't exist
dirname "$OUTPUT" | xargs mkdir -p

echo "Building App for Windows CE..."

arm-mingw32ce-g++ -Wall -Wextra -O3 -mcpu=arm926ej-s -static -s -o "$OUTPUT" "$SOURCE" -D_WIN32_IE=0x0400

if [ $? -eq 0 ]; then
    echo "Build successful! Output: $OUTPUT"
    
    # Generate BMP
    python3 make_bmp.py

    # Sync assets to Example folder for consistency
    echo "Syncing assets to Example folder..."
    rm -f ./Example/*.ttf ./Example/*.otf ./Example/*.wav ./Example/*.bmp ./Example/*.png
    cp *.wav ./Example/ 2>/dev/null || true
    cp teno*.ttf ./Example/ 2>/dev/null || true
    cp cp_period.ttf ./Example/ 2>/dev/null || true
    cp icon.bmp ./Example/ 2>/dev/null || true
    cp icon8.bmp ./Example/ 2>/dev/null || true
    cp icon.png ./Example/ 2>/dev/null || true

    # Deploy to SD Card
    PROJECT_NAME=$(basename "$PWD")
    SD_ROOT="/Volumes/SDCARD/アプリ"
    DEPLOY_DIR="$SD_ROOT/$PROJECT_NAME"
    
    if [ ! -d "$SD_ROOT" ]; then
        echo "Warning: SD Card not found at $SD_ROOT. Skipping deployment."
        echo "Build and sync to ./Example complete."
        exit 0
    fi

    echo "Deploying to $DEPLOY_DIR..."
    
    # Ensure deployment directory exists and is clean
    mkdir -p "$DEPLOY_DIR"
    rm -rf "$DEPLOY_DIR"/*
    
    # Copy core binary and config files
    cp -r ./Example/* "$DEPLOY_DIR/"
    
    # Copy assets (wav, ttf, otf) individually or via glob but safely
    echo "Copying assets..."
    
    # Function to copy files if they exist
    copy_if_exists() {
        local pattern="$1"
        local target="$2"
        # Check if any files match the pattern
        if ls $pattern >/dev/null 2>&1; then
            cp $pattern "$target/"
            return 0
        fi
        return 0
    }

    rm -f "$DEPLOY_DIR"/*.ttf "$DEPLOY_DIR"/*.otf "$DEPLOY_DIR"/*.wav "$DEPLOY_DIR"/*.bmp "$DEPLOY_DIR"/*.png
    copy_if_exists "*.wav" "$DEPLOY_DIR"
    copy_if_exists "teno*.ttf" "$DEPLOY_DIR"
    copy_if_exists "cp_period.ttf" "$DEPLOY_DIR"
    copy_if_exists "*.bmp" "$DEPLOY_DIR"
    
    if [ $? -eq 0 ]; then
        echo "Deployment successful!"
        echo "Contents of "$DEPLOY_DIR":"
        ls -F "$DEPLOY_DIR"
    else
        echo "Deployment failed."
        exit 1
    fi
else
    echo "Build failed."
fi
