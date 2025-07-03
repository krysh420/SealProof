@echo off
echo [✓] Running SealProof Setup...

:: Step 1: Compile monitor.cpp
echo [✓] Compiling monitor.cpp to monitor.exe...
g++ monitor.cpp -o monitor.exe -mwindows
if %errorlevel% neq 0 (
    echo [X] monitor.cpp compilation failed.
    pause
    exit /b
)

:: Step 2: Compile update_config.cpp
echo [✓] Compiling update_config.cpp to update_config.exe...
g++ update_config.cpp -o update_config.exe -mwindows
if %errorlevel% neq 0 (
    echo [X] update_config.cpp compilation failed.
    pause
    exit /b
)

:: Step 3: Create ProgramData directory
set TARGET_DIR=C:\ProgramData\SealProof
if not exist "%TARGET_DIR%" (
    mkdir "%TARGET_DIR%"
    echo [✓] Created %TARGET_DIR%
)

:: Step 4: Copy necessary files
echo [✓] Copying files to ProgramData...
copy /Y monitor.exe "%TARGET_DIR%\monitor.exe" >nul
copy /Y config.txt "%TARGET_DIR%\config.txt" >nul

:: Step 5: Generate initial hashes
echo [✓] Generating initial hashes...
"%TARGET_DIR%\monitor.exe" --generate
if %errorlevel% neq 0 (
    echo [X] Initial hash generation failed.
    pause
    exit /b
)

:: Step 6: Add registry entry for startup
echo [✓] Registering monitor.exe to run at startup via Registry...
reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "SealProof" /t REG_SZ /d "\"%TARGET_DIR%\monitor.exe\"" /f

echo [✓] Setup complete!
pause
