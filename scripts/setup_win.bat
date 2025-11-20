@echo off
REM VisualIA Windows Setup Script
REM Supports: Windows 10/11 with Visual Studio 2019+

setlocal enabledelayedexpansion

echo ========================================
echo VisualIA Windows Setup
echo ========================================
echo.

REM Get script directory and project root
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
cd /d "%PROJECT_ROOT%"

REM ========================================================================
REM [1/7] Check Prerequisites
REM ========================================================================
echo [1/7] Checking prerequisites...
echo.

set ERRORS=0
set MISSING_DEPS=

REM Check for Visual Studio
echo Checking for Visual Studio...
set VS_PATH=
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
    set VS_PATH=%%i
)

if "%VS_PATH%"=="" (
    echo [!] Visual Studio 2019+ with C++ tools not found
    set MISSING_DEPS=!MISSING_DEPS! visualstudio
    set /a ERRORS+=1
) else (
    echo [OK] Visual Studio found
)

REM Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] CMake not found
    set MISSING_DEPS=!MISSING_DEPS! cmake
    set /a ERRORS+=1
) else (
    for /f "tokens=3" %%v in ('cmake --version 2^>^&1 ^| findstr /C:"version"') do (
        echo [OK] CMake %%v
    )
)

REM Check for Git
where git >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] Git not found
    set MISSING_DEPS=!MISSING_DEPS! git
    set /a ERRORS+=1
) else (
    for /f "tokens=*" %%v in ('git --version') do (
        echo [OK] %%v
    )
)

REM Check for Node.js
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] Node.js not found
    set MISSING_DEPS=!MISSING_DEPS! nodejs
    set /a ERRORS+=1
) else (
    for /f "tokens=*" %%v in ('node --version') do (
        echo [OK] Node.js %%v
    )
)

REM Check for Python (optional, for translation)
where python >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] Python not found (optional, needed for translation)
) else (
    for /f "tokens=*" %%v in ('python --version 2^>^&1') do (
        echo [OK] %%v
    )
)

REM ========================================================================
REM [2/7] Install Missing Dependencies
REM ========================================================================
echo.
echo [2/7] Checking for missing dependencies...
echo.

if %ERRORS% gtr 0 (
    echo.
    echo Missing dependencies detected: %MISSING_DEPS%
    echo.
    echo Would you like to install them using Chocolatey?
    echo This requires administrator privileges.
    echo.
    set /p INSTALL_CHOICE="Install missing dependencies? (y/N): "

    if /i "!INSTALL_CHOICE!"=="y" (
        echo.
        echo Checking for Chocolatey...
        where choco >nul 2>nul
        if !errorlevel! neq 0 (
            echo Installing Chocolatey...
            echo This requires administrator privileges. Please confirm the UAC prompt.
            powershell -NoProfile -ExecutionPolicy Bypass -Command "Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"

            REM Refresh PATH
            call refreshenv.cmd 2>nul || (
                echo Please close and reopen this command prompt, then run this script again.
                pause
                exit /b 1
            )
        )

        echo.
        echo Installing dependencies with Chocolatey...

        if not "%MISSING_DEPS:visualstudio=%"=="%MISSING_DEPS%" (
            echo Installing Visual Studio Build Tools...
            echo This may take a while...
            choco install visualstudio2022buildtools --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools" -y
        )

        if not "%MISSING_DEPS:cmake=%"=="%MISSING_DEPS%" (
            choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y
        )

        if not "%MISSING_DEPS:git=%"=="%MISSING_DEPS%" (
            choco install git -y
        )

        if not "%MISSING_DEPS:nodejs=%"=="%MISSING_DEPS%" (
            choco install nodejs -y
        )

        echo.
        echo Dependencies installed. Please close and reopen this command prompt,
        echo then run this script again to continue setup.
        pause
        exit /b 0
    ) else (
        echo.
        echo Please install the following manually:
        if not "%MISSING_DEPS:visualstudio=%"=="%MISSING_DEPS%" (
            echo   - Visual Studio 2019+ with "Desktop development with C++" workload
            echo     Download from: https://visualstudio.microsoft.com/downloads/
        )
        if not "%MISSING_DEPS:cmake=%"=="%MISSING_DEPS%" (
            echo   - CMake: https://cmake.org/download/
        )
        if not "%MISSING_DEPS:git=%"=="%MISSING_DEPS%" (
            echo   - Git: https://git-scm.com/download/win
        )
        if not "%MISSING_DEPS:nodejs=%"=="%MISSING_DEPS%" (
            echo   - Node.js: https://nodejs.org/
        )
        echo.
        echo Then run this script again.
        pause
        exit /b 1
    )
) else (
    echo [OK] All dependencies satisfied
)

REM ========================================================================
REM [3/7] Initialize Git Submodules
REM ========================================================================
echo.
echo [3/7] Initializing git submodules...
echo This will clone whisper.cpp and llama.cpp libraries
echo.

git submodule update --init --recursive
if %errorlevel% neq 0 (
    echo [ERROR] Failed to initialize submodules
    pause
    exit /b 1
)

echo [OK] Submodules initialized

REM ========================================================================
REM [4/7] Build Backend and Frontend
REM ========================================================================
echo.
echo [4/7] Building VisualIA...
echo.

call "%SCRIPT_DIR%build_win.bat"
if %errorlevel% neq 0 (
    echo [ERROR] Build failed
    pause
    exit /b 1
)

REM ========================================================================
REM [5/7] Whisper Model Setup
REM ========================================================================
echo.
echo [5/7] Whisper model setup
echo Which Whisper model(s) would you like to download?
echo Whisper is used for speech recognition (99 languages)
echo.
echo Options:
echo   1) whisper-base      (~141MB, recommended - good quality, fast)
echo   2) whisper-small     (~466MB, better quality)
echo   3) whisper-medium    (~769MB, great quality)
echo   4) whisper-large-v3  (~1.5GB, best quality)
echo   5) Skip (download manually later)
echo.
set /p WHISPER_CHOICE="Enter choice [1-5] (default: 1): "
if "%WHISPER_CHOICE%"=="" set WHISPER_CHOICE=1

mkdir models 2>nul

if "%WHISPER_CHOICE%"=="1" (
    call :download_model "whisper-base" "whisper-base.gguf" "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin"
) else if "%WHISPER_CHOICE%"=="2" (
    call :download_model "whisper-small" "whisper-small.gguf" "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin"
) else if "%WHISPER_CHOICE%"=="3" (
    call :download_model "whisper-medium" "whisper-medium.gguf" "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin"
) else if "%WHISPER_CHOICE%"=="4" (
    call :download_model "whisper-large-v3" "whisper-large-v3.gguf" "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin"
) else (
    echo Skipping Whisper model download
    echo You can download manually:
    echo   powershell -Command "Invoke-WebRequest -Uri 'https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin' -OutFile 'models\whisper-base.gguf'"
)

REM ========================================================================
REM [6/7] Translation Setup (Optional)
REM ========================================================================
echo.
echo [6/7] Translation setup (optional)
echo.
echo WARNING: Windows audio capture (WASAPI) is not yet implemented.
echo Translation will not work until WASAPI support is added to the C backend.
echo.
echo Do you still want to set up translation models?
echo This enables real-time translation between 400+ languages using MADLAD-400
echo.
echo Options:
echo   1) Skip (transcription only)
echo   2) Install MADLAD-400 (requires Python and ~2GB PyTorch)
echo.
set /p TRANSLATION_CHOICE="Enter choice [1-2] (default: 1): "
if "%TRANSLATION_CHOICE%"=="" set TRANSLATION_CHOICE=1

if "%TRANSLATION_CHOICE%"=="2" (
    echo.
    echo Translation setup on Windows requires:
    echo   1. Python 3 installed and in PATH
    echo   2. Running: pip install torch transformers sentencepiece protobuf
    echo   3. Downloading and converting MADLAD-400 model
    echo.
    echo This is a manual process. See scripts/setup_madlad_translation.sh
    echo for the steps (can be adapted to Windows PowerShell).
    pause
) else (
    echo Skipping translation setup
)

REM ========================================================================
REM [7/7] Verification
REM ========================================================================
echo.
echo [7/7] Verifying installation...
echo.

set VERIFY_ERRORS=0

REM Check backend executable
if exist "build\Release\visualia.exe" (
    echo [OK] Backend executable: build\Release\visualia.exe
) else if exist "build\Debug\visualia.exe" (
    echo [OK] Backend executable: build\Debug\visualia.exe
) else (
    echo [!] Backend executable not found
    set /a VERIFY_ERRORS+=1
)

REM Check frontend dependencies
if exist "frontend\node_modules" (
    echo [OK] Frontend dependencies installed
) else (
    echo [!] Frontend dependencies missing
    set /a VERIFY_ERRORS+=1
)

REM Check for Whisper models
dir /b models\whisper-*.gguf >nul 2>nul
if %errorlevel% equ 0 (
    echo [OK] Whisper models found:
    for %%f in (models\whisper-*.gguf) do (
        echo   - %%~nxf
    )
) else (
    echo [!] No Whisper models found
)

echo.
if %VERIFY_ERRORS% equ 0 (
    echo ========================================
    echo Setup Complete!
    echo ========================================
    echo.
    echo IMPORTANT NOTES:
    echo   - Windows audio capture (WASAPI) is NOT implemented yet
    echo   - The backend will not capture audio on Windows
    echo   - Only the frontend UI can be tested currently
    echo.
    echo To run VisualIA frontend:
    echo   cd frontend
    echo   npm start
    echo.
) else (
    echo ========================================
    echo Setup completed with %VERIFY_ERRORS% error(s)
    echo ========================================
    echo Please review the errors above
    echo.
)

pause
exit /b 0

REM ========================================================================
REM Helper function to download models
REM ========================================================================
:download_model
set MODEL_NAME=%~1
set MODEL_FILE=%~2
set MODEL_URL=%~3

if exist "models\%MODEL_FILE%" (
    echo [!] %MODEL_FILE% already exists, skipping
    goto :eof
)

echo Downloading %MODEL_NAME%...
powershell -NoProfile -ExecutionPolicy Bypass -Command "& {$ProgressPreference = 'SilentlyContinue'; Invoke-WebRequest -Uri '%MODEL_URL%' -OutFile 'models\%MODEL_FILE%'; if ($?) { Write-Host '[OK] %MODEL_FILE% downloaded' } else { Write-Host '[ERROR] Download failed' -ForegroundColor Red }}"

goto :eof
