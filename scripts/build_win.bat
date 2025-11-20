@echo off
REM VisualIA Windows Build Script
REM Builds C backend with whisper.cpp/llama.cpp and installs frontend dependencies

setlocal enabledelayedexpansion

REM Get script directory and project root
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
cd /d "%PROJECT_ROOT%"

REM Parse arguments
set BUILD_TYPE=Release
set CLEAN_BUILD=0

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="--clean" (
    set CLEAN_BUILD=1
    shift
    goto :parse_args
)
echo Unknown option: %~1
echo Usage: %~nx0 [--debug] [--clean]
exit /b 1
:args_done

echo ========================================
echo Building VisualIA (Windows)
echo ========================================
echo.

REM ========================================================================
REM [1/3] Build C Backend
REM ========================================================================
echo [1/3] Building C backend (%BUILD_TYPE% mode)...
echo.

REM Use Windows-specific build directory
set BUILD_DIR=build-windows

REM Clean build directory if requested
if %CLEAN_BUILD%==1 (
    echo Cleaning Windows build directory...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
)

REM Preserve Linux build if it exists in "build"
if exist "build" (
    if not exist "build-linux" (
        echo [INFO] Found existing Linux build, preserving as build-linux...
        ren build build-linux
    )
)

REM Create build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Detect Visual Studio version
set VS_GENERATOR=
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion 2^>nul`) do (
    set VS_VERSION=%%i
)

if defined VS_VERSION (
    for /f "tokens=1 delims=." %%a in ("%VS_VERSION%") do set VS_MAJOR=%%a

    if "%VS_MAJOR%"=="17" (
        set VS_GENERATOR=Visual Studio 17 2022
    ) else if "%VS_MAJOR%"=="16" (
        set VS_GENERATOR=Visual Studio 16 2019
    ) else (
        set VS_GENERATOR=Visual Studio 17 2022
    )
) else (
    echo [ERROR] Visual Studio not found
    echo Please install Visual Studio 2019 or later with C++ tools
    cd ..
    exit /b 1
)

echo Using: %VS_GENERATOR%
echo Build directory: %BUILD_DIR%
echo.

REM Configure with CMake
echo Configuring build with CMake...
cmake .. -G "%VS_GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed
    cd ..
    exit /b 1
)

REM Detect number of CPU cores
for /f "skip=1" %%p in ('wmic cpu get NumberOfLogicalProcessors') do (
    set CORES=%%p
    goto :cores_done
)
:cores_done
if not defined CORES set CORES=4

echo Using %CORES% parallel jobs
echo.

REM Build
echo Building backend...
cmake --build . --config %BUILD_TYPE% -j %CORES%
if %errorlevel% neq 0 (
    echo [ERROR] Build failed
    cd ..
    exit /b 1
)

cd ..

REM Verify executable exists
if exist "%BUILD_DIR%\Release\visualia.exe" (
    for %%I in ("%BUILD_DIR%\%BUILD_TYPE%\visualia.exe") do set SIZE=%%~zI
    set /a SIZE_KB=!SIZE! / 1024
    echo [OK] Backend built successfully (!SIZE_KB! KB)
    echo     %BUILD_DIR%\%BUILD_TYPE%\visualia.exe
) else (
    echo [ERROR] Backend executable not found
    exit /b 1
)

REM ========================================================================
REM [2/3] Install Frontend Dependencies
REM ========================================================================
echo.
echo [2/3] Installing frontend dependencies...
echo.

if not exist "frontend" (
    echo [ERROR] Frontend directory not found
    exit /b 1
)

cd frontend

REM Install npm dependencies
call npm install --quiet
if %errorlevel% neq 0 (
    echo [ERROR] npm install failed
    cd ..
    exit /b 1
)

cd ..
echo [OK] Frontend dependencies installed

REM ========================================================================
REM [3/3] Model Status Check
REM ========================================================================
echo.
echo [3/3] Checking AI models...
echo.

if not exist models mkdir models

REM Check for Whisper models
dir /b models\whisper-*.gguf >nul 2>nul
if %errorlevel% equ 0 (
    echo [OK] Whisper models:
    for %%f in (models\whisper-*.gguf) do (
        for %%I in ("%%f") do set SIZE=%%~zI
        set /a SIZE_MB=!SIZE! / 1048576
        echo   - %%~nxf (!SIZE_MB! MB)
    )
) else (
    echo [!] No Whisper models found
    echo.
    echo   Download required for speech recognition:
    echo     powershell -Command "Invoke-WebRequest -Uri 'https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin' -OutFile 'models\whisper-base.gguf'"
    echo.
    echo   Or run setup to download interactively:
    echo     scripts\setup_win.bat
)

REM Check for translation models
dir /b models\madlad*.gguf >nul 2>nul
if %errorlevel% equ 0 (
    echo.
    echo [OK] Translation models (MADLAD-400):
    for %%f in (models\madlad*.gguf) do (
        for %%I in ("%%f") do set SIZE=%%~zI
        set /a SIZE_MB=!SIZE! / 1048576
        echo   - %%~nxf (!SIZE_MB! MB)
    )
) else (
    dir /b models\mt5-*.gguf >nul 2>nul
    if !errorlevel! equ 0 (
        echo.
        echo [!] Translation models (MT5):
        for %%f in (models\mt5-*.gguf) do (
            for %%I in ("%%f") do set SIZE=%%~zI
            set /a SIZE_MB=!SIZE! / 1048576
            echo   - %%~nxf (!SIZE_MB! MB)
        )
        echo.
        echo   Note: MT5 base models may not work for translation.
        echo   Consider using MADLAD-400 instead.
    ) else (
        echo.
        echo [!] No translation models found (optional)
        echo.
        echo   Note: Translation requires WASAPI implementation (not yet available on Windows)
    )
)

REM ========================================================================
REM Build Summary
REM ========================================================================
echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Build artifacts:
echo   Backend:  %BUILD_DIR%\%BUILD_TYPE%\visualia.exe
echo   Frontend: frontend\node_modules\
echo.

REM Show preserved Linux build info
if exist "build-linux" (
    echo [INFO] Linux build preserved in: build-linux\
    echo.
)

echo WARNING: Windows audio capture (WASAPI) is not implemented yet.
echo The backend will not capture audio on Windows.
echo.
echo To run VisualIA frontend (UI only):
echo   cd frontend
echo   npm start
echo.

exit /b 0