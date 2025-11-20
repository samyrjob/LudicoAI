@echo off
REM ======================================================
REM VisualIA Windows Setup Script
REM Supports: Windows 10/11 with Visual Studio 2019+
REM ======================================================

setlocal enabledelayedexpansion

REM -----------------------------
REM Get script directory and project root
REM -----------------------------
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
cd /d "%PROJECT_ROOT%"

echo ========================================
echo VisualIA Windows Setup
echo ========================================

REM ========================================================================
REM [1/7] Platform Detection
REM ========================================================================
echo [1/7] Detecting platform...

ver | findstr /i "windows" >nul
if %errorlevel% neq 0 (
    echo [!] Unsupported platform
    exit /b 1
) else (
    echo [OK] Platform: Windows
)

REM ========================================================================
REM [2/7] Check System Dependencies
REM ========================================================================
echo.
echo [2/7] Checking system dependencies...

set ERRORS=0
set MISSING_DEPS=

REM Check Visual Studio (VS Build Tools)
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

REM Check CMake
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

REM Check Git
where git >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] Git not found
    set MISSING_DEPS=!MISSING_DEPS! git
    set /a ERRORS+=1
) else (
    for /f "tokens=3" %%v in ('git --version') do (
        echo [OK] Git %%v
    )
)

REM Check Node.js
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] Node.js not found
    set MISSING_DEPS=!MISSING_DEPS! nodejs
    set /a ERRORS+=1
) else (
    for /f %%v in ('node --version') do (
        echo [OK] Node.js %%v
    )
)

REM Check Python (optional)
where python >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] Python not found (optional, needed for translation)
) else (
    for /f "tokens=2" %%v in ('python --version 2^>^&1') do (
        echo [OK] Python %%v
    )
)

REM If missing dependencies, suggest Chocolatey install
if %ERRORS% gtr 0 (
    echo.
    echo Missing dependencies detected: %MISSING_DEPS%
    echo You can install them using Chocolatey (https://chocolatey.org/)
)

REM ========================================================================
REM [3/7] Initialize Git Submodules
REM ========================================================================
echo.
echo [3/7] Initializing git submodules...
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
echo Options:
echo   1) whisper-base (~141MB, recommended)
echo   2) whisper-small (~466MB)
echo   3) whisper-medium (~769MB)
echo   4) whisper-large-v3 (~1.5GB)
echo   5) Skip

set /p WHISPER_CHOICE="Enter choice [1-5] (default: 1): "
if "%WHISPER_CHOICE%"=="" set WHISPER_CHOICE=1

mkdir models 2>nul

REM Function replacement using call label
if "%WHISPER_CHOICE%"=="1" call :download_model "whisper-base" "whisper-base.gguf" "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin"
if "%WHISPER_CHOICE%"=="2" call :download_model "whisper-small" "whisper-small.gguf" "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin"
if "%WHISPER_CHOICE%"=="3" call :download_model "whisper-medium" "whisper-medium.gguf" "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin"
if "%WHISPER_CHOICE%"=="4" call :download_model "whisper-large-v3" "whisper-large-v3.gguf" "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin"
if "%WHISPER_CHOICE%"=="5" echo Skipping Whisper model download

REM ========================================================================
REM [6/7] Translation Setup (Optional)
REM ========================================================================
echo.
echo [6/7] Translation setup skipped on Windows (manual steps)
REM You can call setup_madlad_translation.sh in WSL or PowerShell manually

REM ========================================================================
REM [7/7] Verification
REM ========================================================================
echo.
set VERIFY_ERRORS=0

if exist "build\Release\visualia.exe" (
    echo [OK] Backend executable found
) else (
    echo [!] Backend executable not found
    set /a VERIFY_ERRORS+=1
)

if exist "frontend\node_modules" (
    echo [OK] Frontend dependencies installed
) else (
    echo [!] Frontend dependencies missing
    set /a VERIFY_ERRORS+=1
)

dir /b models\whisper-*.gguf >nul 2>nul
if %errorlevel% equ 0 (
    echo [OK] Whisper models found
) else (
    echo [!] No Whisper models found
)

if %VERIFY_ERRORS% equ 0 (
    echo Setup Complete!
) else (
    echo Setup completed with %VERIFY_ERRORS% error(s)
)

pause
exit /b 0

REM =========================
REM Helper function
REM =========================
:download_model
set MODEL_NAME=%~1
set MODEL_FILE=%~2
set MODEL_URL=%~3

if exist "models\%MODEL_FILE%" (
    echo [!] %MODEL_FILE% already exists, skipping
    goto :eof
)

echo Downloading %MODEL_NAME%...
powershell -NoProfile -ExecutionPolicy Bypass -Command "& {Invoke-WebRequest -Uri '%MODEL_URL%' -OutFile 'models\%MODEL_FILE%'}"
if %errorlevel% neq 0 (
    echo [ERROR] Download failed
)
goto :eof
