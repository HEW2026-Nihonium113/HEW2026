@echo off
call tools\_common.cmd :init

echo Visual Studio 2022 プロジェクトを生成中...
call tools\_common.cmd :generate_project
if errorlevel 1 exit /b 1

echo.
echo Debug ビルド中...
call tools\_common.cmd :setup_msbuild
if errorlevel 1 exit /b 1

msbuild build\HEW2026.sln -p:Configuration=Debug -p:Platform=x64 -m -v:minimal
if errorlevel 1 (
    echo [ERROR] ビルド失敗
    exit /b 1
)
echo [OK] ビルド成功
