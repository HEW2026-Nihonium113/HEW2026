:: 共通処理 - 他のスクリプトから call で呼び出す
:: 使用例: call tools\_common.cmd :setup_msbuild

goto %~1

:init
    chcp 65001 >nul
    cd /d "%~dp0.."
    exit /b 0

:check_project
    if not exist "build\HEW2026.sln" (
        echo プロジェクトが見つかりません。先に @make_project.cmd を実行してください。
        exit /b 1
    )
    exit /b 0

:setup_msbuild
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "%VSWHERE%" (
        echo [ERROR] vswhere.exe が見つかりません。Visual Studio 2022 をインストールしてください。
        exit /b 1
    )
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find Common7\Tools\VsDevCmd.bat`) do (
        set "VSCMD_PATH=%%i"
    )
    if not defined VSCMD_PATH (
        echo [ERROR] Visual Studio が見つかりません
        exit /b 1
    )
    call "%VSCMD_PATH%" -arch=amd64 >nul 2>&1
    exit /b 0

:find_msbuild_exe
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "%VSWHERE%" (
        echo [ERROR] vswhere.exe が見つかりません。Visual Studio 2022 をインストールしてください。
        exit /b 1
    )
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        set "MSBUILD_PATH=%%i"
    )
    if not defined MSBUILD_PATH (
        echo [ERROR] MSBuild が見つかりません。Visual Studio 2022 の C++ ワークロードをインストールしてください。
        exit /b 1
    )
    exit /b 0

:generate_project
    :: 日本語パス対策: TEMPにジャンクションを作成
    for /f %%a in ('powershell -command "[guid]::NewGuid().ToString()"') do set "GUID=%%a"
    mklink /j "%TEMP%\%GUID%" "%~dp0.." >nul
    pushd "%TEMP%\%GUID%"
    tools\premake5.exe vs2022
    set "PREMAKE_RESULT=%errorlevel%"
    popd
    rmdir "%TEMP%\%GUID%"
    if %PREMAKE_RESULT% neq 0 (
        echo [ERROR] プロジェクト生成に失敗しました
        exit /b 1
    )
    echo [OK] build\HEW2026.sln を生成しました
    exit /b 0
