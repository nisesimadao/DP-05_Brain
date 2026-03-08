# Build script for Windows CE (ARM / Sharp Brain)
# Using WSL (Windows Subsystem for Linux) to run the ELF CeGCC compiler

$ErrorActionPreference = "Stop"

# --- Configuration ---
$cegccPathWSL = "/mnt/c/cegcc"
$binPathWSL = "$cegccPathWSL/bin"
$gppWSL = "$binPathWSL/arm-mingw32ce-g++"

# Check if WSL is available
if (!(Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
    Write-Error "WSL (Windows Subsystem for Linux) not found. Please install WSL to use the ELF compiler."
}

# Source and Output paths (Directly convert to WSL paths to avoid wslpath issues)
function WinToWSLPath($winPath) {
    $p = $winPath.ToString().Replace('\', '/')
    if ($p -match '^([A-Za-z]):/(.*)') {
        return "/mnt/" + $Matches[1].ToLower() + "/" + $Matches[2]
    }
    return $p
}

$sourceWin = (Resolve-Path "src/main.cpp").Path
$outputDirWin = "Example"
$outputWin = (Join-Path (Get-Location).Path "$outputDirWin/AppMain.exe")

$sourceWSL = WinToWSLPath $sourceWin
$outputWSL = WinToWSLPath $outputWin

if (!(Test-Path $outputDirWin)) {
    New-Item -ItemType Directory -Path $outputDirWin
}

Write-Host "Building App for Windows CE (ARM / Sharp Brain) via WSL..."

# Compiler Flags (Matches build.sh)
$compileCmd = "$gppWSL -Wall -Wextra -O3 -mcpu=arm926ej-s -static -s -o $outputWSL $sourceWSL -D_WIN32_IE=0x0400"

Write-Host "Executing in WSL: $compileCmd"
# Isolate PATH to avoid host Windows PATH interference (parentheses, etc.)
wsl bash -c "export PATH=${binPathWSL}:/usr/bin:/bin; $compileCmd"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful! Output: $outputWin"
    
    # Generate BMP
    Write-Host "Generating BMP assets..."
    python scripts/make_bmp.py assets/images/icon.png assets/images/

    # Generate Sounds
    Write-Host "Generating Sound assets..."
    python scripts/make_sounds.py assets/sounds/

    # Sync assets to Example folder
    Write-Host "Syncing assets to Example folder..."
    $exampleToRemove = Get-ChildItem -Path "Example" -Include *.ttf, *.otf, *.wav, *.bmp, *.png -Recurse
    if ($exampleToRemove) { $exampleToRemove | Remove-Item }

    Copy-Item "assets\sounds\*.wav" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\fonts\teno*.ttf" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\fonts\cp_period.ttf" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\images\icon.bmp" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\images\icon8.bmp" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\images\icon.png" "Example\" -ErrorAction SilentlyContinue

    Write-Host "------------------------------------------------"
    Write-Host "Ready! You can find the executable and assets in $outputDirWin"
    Write-Host "------------------------------------------------"
} else {
    Write-Error "Build failed."
}

