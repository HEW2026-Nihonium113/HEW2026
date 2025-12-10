@echo off
call tools\_common.cmd :init
call tools\_common.cmd :check_project || exit /b 1
call tools\_common.cmd :setup_msbuild || exit /b 1

echo Debug ビルド中...
msbuild build\HEW2026.sln -p:Configuration=Debug -p:Platform=x64 -m -v:minimal
if errorlevel 1 (
    echo [ERROR] ビルド失敗
    exit /b 1
)
echo [OK] ビルド成功
