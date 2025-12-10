@echo off
call tools\_common.cmd :init

echo 現在のディレクトリ: %CD%
echo.

:: Generate project if needed
if not exist "build\HEW2026.sln" (
    echo プロジェクト生成中...
    call tools\_common.cmd :generate_project
    if errorlevel 1 (
        pause
        exit /b 1
    )
) else (
    echo [OK] build\HEW2026.sln 確認済み
)

:: Build if exe doesn't exist
if exist "bin\Debug-windows-x86_64\game\game.exe" goto :skip_build

echo.
echo Debug ビルド中...
call tools\_common.cmd :find_msbuild_exe
if errorlevel 1 (
    pause
    exit /b 1
)
echo 使用: %MSBUILD_PATH%
"%MSBUILD_PATH%" build\HEW2026.sln -p:Configuration=Debug -p:Platform=x64 -m -v:minimal
if errorlevel 1 (
    echo [ERROR] ビルド失敗
    pause
    exit /b 1
)
echo [OK] ビルド成功
goto :open_vs

:skip_build
echo [OK] game.exe 確認済み

:open_vs

echo.
echo Visual Studio を起動中...
start "" "build\HEW2026.sln"
