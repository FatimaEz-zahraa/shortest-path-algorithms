@echo off
echo Setting up Emscripten environment...
call emsdk\emsdk_env.bat

echo Starting Web Server...
echo Open http://localhost:8080/index.html in your browser.
emrun --no_browser --port 8080 .
pause
