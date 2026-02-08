@echo off
REM Try to find g++ in common MSYS2 location if not in PATH
if exist "C:\msys64\ucrt64\bin\g++.exe" (
    set "CXX=C:\msys64\ucrt64\bin\g++.exe"
) else if exist "C:\Program Files\CodeBlocks\MinGW\bin\g++.exe" (
    set "CXX=C:\Program Files\CodeBlocks\MinGW\bin\g++.exe"
) else (
    set "CXX=g++"
)


echo Building GUI application...
"%CXX%" -o dijikstra.exe main.cpp Grid.cpp Algorithms.cpp GraphUtils.cpp -lgdi32 -luser32 -lcomdlg32 -static
if %errorlevel% neq 0 (
    echo Compilation Failed!
    exit /b %errorlevel%
)
echo Compilation Successful. Run dijkstra.exe to start.
