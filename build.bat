@echo off
echo Building ngTool.exe...
echo.

cd /d "%~dp0"

if not exist "src\simple-main.cpp" (
    echo ERROR: Source file not found
    pause
    exit /b 1
)

if not exist "src\resource.rc" (
    echo ERROR: Resource file not found
    pause
    exit /b 1
)

cd src

echo Step 1: Generate icon if needed...
if not exist "icon.ico" (
    if exist "create_icon.c" (
        gcc create_icon.c -o create_icon.exe
        if exist "create_icon.exe" (
            create_icon.exe
            del create_icon.exe
        )
    )
)

echo Step 2: Compile resource file...
windres resource.rc -o resource.o
if %errorlevel% neq 0 (
    echo ERROR: Resource compilation failed
    cd ..
    pause
    exit /b 1
)

echo Step 3: Compile main program...
g++ -O2 -s -mwindows -o ngTool.exe simple-main.cpp resource.o -lgdi32 -luser32 -lkernel32 -lshell32 -lole32

if exist "ngTool.exe" (
    echo.
    echo SUCCESS: Build completed!
    echo.
    if exist "..\ngTool.exe" del "..\ngTool.exe"
    move ngTool.exe ..
    cd ..
    dir ngTool.exe | findstr ngTool.exe
    echo.
    echo Ready to use: ngTool.exe
) else (
    echo.
    echo ERROR: Build failed!
    cd ..
)

echo.
pause
