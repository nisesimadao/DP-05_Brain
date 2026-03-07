#!/bin/bash

# Build script for Brain PW-SH2 Apps
# Requires CeGCC environment
cd "$(dirname "$0")"

SOURCE="../src/main.cpp"
OUTPUT="../Example/AppMain.exe"

# Create output directory if it doesn't exist
mkdir -p "../Example"

echo "Building App for Windows CE..."

arm-mingw32ce-g++ -Wall -Wextra -O3 -mcpu=arm926ej-s -static -s -o "$OUTPUT" "$SOURCE" -D_WIN32_IE=0x0400

if [ $? -eq 0 ]; then
    echo "Build successful! Output: $OUTPUT"
    
    # Generate BMP
    python3 ../scripts/make_bmp.py ../assets/images/icon.png ../assets/images/

    # Generate Sounds
    python3 ../scripts/make_sounds.py ../assets/sounds/

    # Sync assets to Example folder for consistency
    echo "Syncing assets to Example folder..."
    rm -f ../Example/*.ttf ../Example/*.otf ../Example/*.wav ../Example/*.bmp ../Example/*.png
    cp ../assets/sounds/*.wav ../Example/ 2>/dev/null || true
    cp ../assets/fonts/teno*.ttf ../Example/ 2>/dev/null || true
    cp ../assets/fonts/cp_period.ttf ../Example/ 2>/dev/null || true
    cp ../assets/images/icon.bmp ../Example/ 2>/dev/null || true
    cp ../assets/images/icon8.bmp ../Example/ 2>/dev/null || true
    cp ../assets/images/icon.png ../Example/ 2>/dev/null || true

    # Deploy to SD Card
    # Project name is the name of the parent directory of build_scripts
    PROJECT_NAME=$(basename "$(dirname "$PWD")")
    SD_ROOT="/Volumes/SDCARD/アプリ"
    DEPLOY_DIR="$SD_ROOT/$PROJECT_NAME"
    
    if [ ! -d "$SD_ROOT" ]; then
        echo "Warning: SD Card not found at $SD_ROOT. Skipping deployment."
        echo "Build and sync to ../Example complete."
        exit 0
    fi

    echo "Deploying to $DEPLOY_DIR..."
    
    # Ensure deployment directory exists and is clean
    mkdir -p "$DEPLOY_DIR"
    rm -rf "$DEPLOY_DIR"/*
    
    # Copy core binary and config files from Example
    cp -r ../Example/* "$DEPLOY_DIR/"
    
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
