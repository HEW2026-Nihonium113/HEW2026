@echo off
call tools\_common.cmd :init

echo Current directory: %CD%
echo.

:: Generate project if needed
if not exist "build\HEW2026.sln" (
    echo Generating project...
    call tools\_common.cmd :generate_project
    if errorlevel 1 (
        pause
        exit /b 1
    )
) else (
    echo [OK] build\HEW2026.sln exists
)

:: Build if exe doesn't exist
if exist "bin\Debug-windows-x86_64\game\game.exe" goto :skip_build

echo.
echo Building Debug...
call tools\_common.cmd :find_msbuild_exe
if errorlevel 1 (
    pause
    exit /b 1
)
echo Using: %MSBUILD_PATH%
"%MSBUILD_PATH%" build\HEW2026.sln -p:Configuration=Debug -p:Platform=x64 -m -v:minimal
if errorlevel 1 (
    echo [ERROR] Build failed
    pause
    exit /b 1
)
echo [OK] Build succeeded
goto :open_vs

:skip_build
echo [OK] game.exe already exists

:open_vs

echo.
echo Opening Visual Studio...
start "" "build\HEW2026.sln"
