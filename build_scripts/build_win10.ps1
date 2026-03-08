# Build script for Windows 10/11 (MSVC)
# Requires Visual Studio 2022

$ErrorActionPreference = "Stop"

# Setup MSVC Environment if not already in path
if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Host "Searching for Visual Studio..."
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path $vswhere)) {
        Write-Error "vswhere.exe not found. Is Visual Studio installed?"
    }

    $vsPath = & $vswhere -latest -property installationPath
    $vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
    
    if (Test-Path $vcvars) {
        Write-Host "Initializing MSVC environment using $vcvars..."
        # Execute vcvars and grab its environment variables
        $vars = cmd /c "`"$vcvars`" && set"
        foreach ($var in $vars) {
            if ($var -match '^([^=]+)=(.*)$') {
                $name = $Matches[1]
                $val = $Matches[2]
                [Environment]::SetEnvironmentVariable($name, $val, "Process")
            }
        }
    } else {
        Write-Error "Could not find vcvars script at $vcvars"
    }
}

$source = "src\main.cpp"
$outputDir = "dist_win10"
$output = Join-Path $outputDir "AppMain_win10.exe"

if (!(Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir
}

Write-Host "Building App for Windows 10 (MSVC)..."

# MSVC Compiler Flags:
# /O2: Optimization for speed
# /EHsc: Exception handling
# /utf-8: Source and execution character sets are UTF-8
# /DUNICODE /D_UNICODE: Unicode support
# /MT: Static linking to CRT
# /Fe"$output": Output file path
# /link /SUBSYSTEM:WINDOWS: GUI application (no console)
# user32.lib etc: Link necessary libraries
cl.exe /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE /MT /Fe"$output" "$source" `
    /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib winmm.lib shell32.lib shlwapi.lib

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful! Output: $output"

    # Clean up intermediate files (.obj)
    if (Test-Path "main.obj") { Remove-Item "main.obj" }

    # Generate BMP
    Write-Host "Generating BMP assets..."
    python scripts/make_bmp.py assets/images/icon.png assets/images/

    # Generate Sounds
    Write-Host "Generating Sound assets..."
    python scripts/make_sounds.py assets/sounds/

    Write-Host "Preparing assets..."
    $assetsToRemove = Get-ChildItem -Path "$outputDir" -Include *.ttf, *.otf, *.wav, *.bmp, *.png -Recurse
    if ($assetsToRemove) { $assetsToRemove | Remove-Item }

    Copy-Item "assets\sounds\*.wav" "$outputDir\" -ErrorAction SilentlyContinue
    Copy-Item "assets\fonts\teno*.ttf" "$outputDir\" -ErrorAction SilentlyContinue
    Copy-Item "assets\fonts\cp_period.ttf" "$outputDir\" -ErrorAction SilentlyContinue
    Copy-Item "assets\images\icon.bmp" "$outputDir\" -ErrorAction SilentlyContinue
    Copy-Item "assets\images\icon.png" "$outputDir\" -ErrorAction SilentlyContinue

    # Sync to Example folder
    Write-Host "Syncing assets to Example folder..."
    if (!(Test-Path "Example")) { New-Item -ItemType Directory -Path "Example" }
    $exampleToRemove = Get-ChildItem -Path "Example" -Include *.ttf, *.otf, *.wav, *.bmp, *.png -Recurse
    if ($exampleToRemove) { $exampleToRemove | Remove-Item }

    Copy-Item "assets\sounds\*.wav" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\fonts\teno*.ttf" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\fonts\cp_period.ttf" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\images\icon.bmp" "Example\" -ErrorAction SilentlyContinue
    Copy-Item "assets\images\icon.png" "Example\" -ErrorAction SilentlyContinue

    Write-Host "------------------------------------------------"
    Write-Host "Ready! Executable and assets are in $outputDir"
    Write-Host "------------------------------------------------"
} else {
    Write-Error "Build failed."
}
