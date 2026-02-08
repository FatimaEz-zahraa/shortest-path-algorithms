@echo off
setlocal
echo ========================================
echo  Dijkstra WebAssembly Build Setup
echo ========================================

REM 1. Check for Emscripten SDK
if not exist "emsdk" (
    echo [INFO] Emscripten SDK not found. Cloning...
    git clone https://github.com/emscripten-core/emsdk.git
    if %errorlevel% neq 0 (
        echo [ERROR] Git clone failed. Please install Git.
        pause
        exit /b 1
    )
)

cd emsdk

REM 2. Try to activate first (Check if we are ready)
echo [INFO] Checking for Emscripten tools...
call emsdk.bat activate latest >nul 2>&1
if %errorlevel% equ 0 goto environment

REM 3. If activate failed, install then activate
echo [INFO] Tools not ready or not installed. Installing latest Emscripten SDK (This may take a while)...
call emsdk.bat install latest
if %errorlevel% neq 0 (
    echo [ERROR] Emscripten installation failed.
    cd ..
    pause
    exit /b 1
)

echo [INFO] Activating latest tools...
call emsdk.bat activate latest
if %errorlevel% neq 0 (
    echo [ERROR] Emscripten activation failed.
    cd ..
    pause
    exit /b 1
)

:environment
REM 4. Activate Environment for this session
echo [INFO] Setting up environment variables...
call emsdk_env.bat

cd ..

REM 5. Compile
echo ========================================
echo  Compiling C++ to WebAssembly...
echo ========================================

REM Check if emcc is actually reachable now
where emcc >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] 'emcc' is still not found in PATH.
    echo The environment setup might have failed.
    pause
    exit /b 1
)

call emcc Bindings.cpp Grid.cpp Algorithms.cpp GraphUtils.cpp -o dijkstra.js -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 --bind -O3 -std=c++17
if %errorlevel% neq 0 (
    echo [ERROR] Compilation Failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo  SUCCESS!
echo ========================================
echo  To run the app:
echo  1. Open a terminal in this folder.
echo  2. Run: python -m http.server
echo  3. Open http://localhost:8000 in your browser.
echo.
pause
