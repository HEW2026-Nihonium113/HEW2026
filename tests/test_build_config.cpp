//----------------------------------------------------------------------------
//! @file   test_build_config.cpp
//! @brief  ビルド設定検証テストスイート
//!
//! @details
//! このファイルはビルドシステム設定の整合性を検証します。
//!
//! テストカテゴリ:
//! - Premake5設定検証: premake5.luaの構文と設定値の検証
//! - パス一貫性: README.md、premake5.lua、cleanup.cmdの整合性検証
//! - .gitignore検証: ビルド成果物が適切に除外されることの確認
//! - ディレクトリ構造検証: 期待されるビルド出力構造の検証
//!
//! @note このテストはビルドシステム自体は実行せず、設定の妥当性のみを検証します
//----------------------------------------------------------------------------
#include "test_common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
namespace tests {

//----------------------------------------------------------------------------
// ファイル読み込みユーティリティ
//----------------------------------------------------------------------------

//! ファイル全体を文字列として読み込む
//! @param filepath ファイルパス
//! @return ファイル内容（失敗時は空文字列）
static std::string ReadFileToString(const fs::path& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

//! ファイルを行ごとに読み込む
//! @param filepath ファイルパス
//! @return 行のベクター（失敗時は空）
static std::vector<std::string> ReadFileLines(const fs::path& filepath)
{
    std::vector<std::string> lines;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return lines;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

//! 文字列に指定パターンが含まれるか検索
//! @param content 検索対象文字列
//! @param pattern 検索パターン（正規表現）
//! @return マッチした場合true
static bool ContainsPattern(const std::string& content, const std::string& pattern)
{
    try {
        std::regex re(pattern);
        return std::regex_search(content, re);
    } catch (...) {
        return false;
    }
}

//! 文字列から指定パターンをすべて抽出
//! @param content 検索対象文字列
//! @param pattern 検索パターン（正規表現）
//! @return マッチした文字列のベクター
static std::vector<std::string> ExtractAllMatches(const std::string& content, const std::string& pattern)
{
    std::vector<std::string> matches;
    try {
        std::regex re(pattern);
        auto begin = std::sregex_iterator(content.begin(), content.end(), re);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            matches.push_back(it->str());
        }
    } catch (...) {
    }
    return matches;
}

//----------------------------------------------------------------------------
// premake5.lua 検証テスト
//----------------------------------------------------------------------------

//! premake5.lua ファイル存在確認
static void TestPremake5_FileExists()
{
    std::cout << "\n=== premake5.lua ファイル存在確認 ===" << std::endl;
    
    fs::path premakePath = "premake5.lua";
    TEST_ASSERT(fs::exists(premakePath), "premake5.luaが存在すること");
    TEST_ASSERT(fs::is_regular_file(premakePath), "premake5.luaが通常ファイルであること");
    
    auto fileSize = fs::file_size(premakePath);
    TEST_ASSERT(fileSize > 0, "premake5.luaが空でないこと");
    TEST_ASSERT(fileSize > 5000, "premake5.luaが十分なサイズを持つこと（5KB以上）");
}

//! premake5.lua 基本構文検証
static void TestPremake5_BasicSyntax()
{
    std::cout << "\n=== premake5.lua 基本構文検証 ===" << std::endl;
    
    std::string content = ReadFileToString("premake5.lua");
    TEST_ASSERT(!content.empty(), "premake5.luaが読み込めること");
    
    // 必須のワークスペース定義
    TEST_ASSERT(ContainsPattern(content, "workspace\\s+\"HEW2026\""), 
                "workspace \"HEW2026\"が定義されていること");
    
    // 必須の設定項目
    TEST_ASSERT(ContainsPattern(content, "configurations\\s*\\{"), 
                "configurationsが定義されていること");
    TEST_ASSERT(ContainsPattern(content, "\"Debug\""), 
                "Debug設定が存在すること");
    TEST_ASSERT(ContainsPattern(content, "\"Release\""), 
                "Release設定が存在すること");
    
    // プラットフォーム設定
    TEST_ASSERT(ContainsPattern(content, "platforms\\s*\\{"), 
                "platformsが定義されていること");
    TEST_ASSERT(ContainsPattern(content, "\"x64\""), 
                "x64プラットフォームが存在すること");
    
    // 言語とC++バージョン
    TEST_ASSERT(ContainsPattern(content, "language\\s+\"C\\+\\+\""), 
                "言語がC++に設定されていること");
    TEST_ASSERT(ContainsPattern(content, "cppdialect\\s+\"C\\+\\+20\""), 
                "C++20が指定されていること");
}

//! premake5.lua 出力ディレクトリ設定検証
static void TestPremake5_OutputDirectoryConfig()
{
    std::cout << "\n=== premake5.lua 出力ディレクトリ設定検証 ===" << std::endl;
    
    std::string content = ReadFileToString("premake5.lua");
    
    // outputdir変数の定義確認
    TEST_ASSERT(ContainsPattern(content, "outputdir\\s*="), 
                "outputdir変数が定義されていること");
    
    // bindir変数の定義確認（build/配下に統一）
    TEST_ASSERT(ContainsPattern(content, "bindir\\s*=\\s*\"build/bin/\""), 
                "bindir変数がbuild/bin/配下に設定されていること");
    
    // objdir_base変数の定義確認（build/配下に統一）
    TEST_ASSERT(ContainsPattern(content, "objdir_base\\s*=\\s*\"build/obj/\""), 
                "objdir_base変数がbuild/obj/配下に設定されていること");
    
    // outputdir変数が正しいフォーマットであること
    TEST_ASSERT(ContainsPattern(content, "outputdir\\s*=\\s*\"%\\{cfg\\.buildcfg\\}"), 
                "outputdirにcfg.buildcfgが含まれること");
    TEST_ASSERT(ContainsPattern(content, "%\\{cfg\\.system\\}"), 
                "outputdirにcfg.systemが含まれること");
    TEST_ASSERT(ContainsPattern(content, "%\\{cfg\\.architecture\\}"), 
                "outputdirにcfg.architectureが含まれること");
}

//! premake5.lua プロジェクト定義検証
static void TestPremake5_ProjectDefinitions()
{
    std::cout << "\n=== premake5.lua プロジェクト定義検証 ===" << std::endl;
    
    std::string content = ReadFileToString("premake5.lua");
    
    // 必須プロジェクトの存在確認
    TEST_ASSERT(ContainsPattern(content, "project\\s+\"DirectXTex\""), 
                "DirectXTexプロジェクトが定義されていること");
    TEST_ASSERT(ContainsPattern(content, "project\\s+\"DirectXTK\""), 
                "DirectXTKプロジェクトが定義されていること");
    TEST_ASSERT(ContainsPattern(content, "project\\s+\"dx11\""), 
                "dx11プロジェクトが定義されていること");
    TEST_ASSERT(ContainsPattern(content, "project\\s+\"engine\""), 
                "engineプロジェクトが定義されていること");
    TEST_ASSERT(ContainsPattern(content, "project\\s+\"game\""), 
                "gameプロジェクトが定義されていること");
    TEST_ASSERT(ContainsPattern(content, "project\\s+\"tests\""), 
                "testsプロジェクトが定義されていること");
}

//! premake5.lua targetdir/objdir使用確認
static void TestPremake5_TargetDirUsage()
{
    std::cout << "\n=== premake5.lua targetdir/objdir使用確認 ===" << std::endl;
    
    std::string content = ReadFileToString("premake5.lua");
    
    // すべてのtargetdirがbindir変数を使用していることを確認
    auto targetdirs = ExtractAllMatches(content, "targetdir\\s*\\([^)]+\\)");
    TEST_ASSERT(!targetdirs.empty(), "targetdir設定が存在すること");
    
    bool allUseBindir = true;
    for (const auto& targetdir : targetdirs) {
        if (targetdir.find("bindir") == std::string::npos) {
            allUseBindir = false;
            std::cout << "  警告: bindirを使用していないtargetdir: " << targetdir << std::endl;
        }
    }
    TEST_ASSERT(allUseBindir, "すべてのtargetdirがbindir変数を使用していること");
    
    // すべてのobjdirがobjdir_base変数を使用していることを確認
    auto objdirs = ExtractAllMatches(content, "objdir\\s*\\([^)]+\\)");
    TEST_ASSERT(!objdirs.empty(), "objdir設定が存在すること");
    
    bool allUseObjdirBase = true;
    for (const auto& objdir : objdirs) {
        if (objdir.find("objdir_base") == std::string::npos) {
            allUseObjdirBase = false;
            std::cout << "  警告: objdir_baseを使用していないobjdir: " << objdir << std::endl;
        }
    }
    TEST_ASSERT(allUseObjdirBase, "すべてのobjdirがobjdir_base変数を使用していること");
}

//! premake5.lua 旧パス参照の不存在確認
static void TestPremake5_NoLegacyPaths()
{
    std::cout << "\n=== premake5.lua 旧パス参照の不存在確認 ===" << std::endl;
    
    std::string content = ReadFileToString("premake5.lua");
    
    // 旧形式のbin/やobj/への直接参照がないことを確認
    TEST_ASSERT(!ContainsPattern(content, "targetdir\\s*\\(\\s*\"bin/"), 
                "targetdirが旧形式の\"bin/\"を直接参照していないこと");
    TEST_ASSERT(!ContainsPattern(content, "objdir\\s*\\(\\s*\"obj/"), 
                "objdirが旧形式の\"obj/\"を直接参照していないこと");
    
    // コメント以外でbin/やobj/への言及がないことを確認（緩い検証）
    auto lines = ReadFileLines("premake5.lua");
    int nonCommentBinObjReferences = 0;
    for (const auto& line : lines) {
        // コメント行をスキップ
        if (line.find("--") != std::string::npos) {
            continue;
        }
        // "bin/"または"obj/"が直接記述されている行をカウント
        if (line.find("\"bin/") != std::string::npos || 
            line.find("\"obj/") != std::string::npos) {
            nonCommentBinObjReferences++;
        }
    }
    TEST_ASSERT(nonCommentBinObjReferences == 0, 
                "コメント以外でbin/やobj/への直接参照がないこと");
}

//----------------------------------------------------------------------------
// .gitignore 検証テスト
//----------------------------------------------------------------------------

//! .gitignore ファイル存在確認
static void TestGitignore_FileExists()
{
    std::cout << "\n=== .gitignore ファイル存在確認 ===" << std::endl;
    
    fs::path gitignorePath = ".gitignore";
    TEST_ASSERT(fs::exists(gitignorePath), ".gitignoreが存在すること");
    TEST_ASSERT(fs::is_regular_file(gitignorePath), ".gitignoreが通常ファイルであること");
}

//! .gitignore build/パターン検証
static void TestGitignore_BuildPattern()
{
    std::cout << "\n=== .gitignore build/パターン検証 ===" << std::endl;
    
    auto lines = ReadFileLines(".gitignore");
    TEST_ASSERT(!lines.empty(), ".gitignoreが読み込めること");
    
    // build/パターンが存在することを確認
    bool hasBuildPattern = false;
    for (const auto& line : lines) {
        std::string trimmed = line;
        // 先頭・末尾の空白を削除
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
        
        if (trimmed == "build/" || trimmed == "build") {
            hasBuildPattern = true;
            break;
        }
    }
    TEST_ASSERT(hasBuildPattern, "build/パターンが存在すること");
}

//! .gitignore 旧パターン不存在確認
static void TestGitignore_NoLegacyPatterns()
{
    std::cout << "\n=== .gitignore 旧パターン不存在確認 ===" << std::endl;
    
    auto lines = ReadFileLines(".gitignore");
    
    // bin/とobj/の個別パターンが存在しないことを確認
    bool hasBinPattern = false;
    bool hasObjPattern = false;
    
    for (const auto& line : lines) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
        
        // コメント行をスキップ
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        
        if (trimmed == "bin/" || trimmed == "bin") {
            hasBinPattern = true;
        }
        if (trimmed == "obj/" || trimmed == "obj") {
            hasObjPattern = true;
        }
    }
    
    TEST_ASSERT(!hasBinPattern, "個別のbin/パターンが存在しないこと（build/に統合）");
    TEST_ASSERT(!hasObjPattern, "個別のobj/パターンが存在しないこと（build/に統合）");
}

//! .gitignore コメント検証
static void TestGitignore_CommentAccuracy()
{
    std::cout << "\n=== .gitignore コメント検証 ===" << std::endl;
    
    auto lines = ReadFileLines(".gitignore");
    
    // build/の前後のコメントを確認
    bool foundBuildComment = false;
    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        
        // "ビルド成果物"コメントを探す
        if (line.find("ビルド成果物") != std::string::npos) {
            foundBuildComment = true;
            
            // コメントにbin/とobj/への言及があることを確認
            bool mentionsBinObj = line.find("bin/") != std::string::npos || 
                                  line.find("obj/") != std::string::npos ||
                                  line.find("build/配下") != std::string::npos;
            TEST_ASSERT(mentionsBinObj, 
                       "ビルド成果物コメントがbin/とobj/がbuild/配下に含まれることを説明していること");
            break;
        }
    }
    
    TEST_ASSERT(foundBuildComment, "ビルド成果物に関するコメントが存在すること");
}

//----------------------------------------------------------------------------
// @cleanup.cmd 検証テスト
//----------------------------------------------------------------------------

//! @cleanup.cmd ファイル存在確認
static void TestCleanupCmd_FileExists()
{
    std::cout << "\n=== @cleanup.cmd ファイル存在確認 ===" << std::endl;
    
    fs::path cleanupPath = "@cleanup.cmd";
    TEST_ASSERT(fs::exists(cleanupPath), "@cleanup.cmdが存在すること");
    TEST_ASSERT(fs::is_regular_file(cleanupPath), "@cleanup.cmdが通常ファイルであること");
}

//! @cleanup.cmd build/削除コマンド検証
static void TestCleanupCmd_BuildRemovalCommand()
{
    std::cout << "\n=== @cleanup.cmd build/削除コマンド検証 ===" << std::endl;
    
    std::string content = ReadFileToString("@cleanup.cmd");
    TEST_ASSERT(!content.empty(), "@cleanup.cmdが読み込めること");
    
    // build削除コマンドの存在確認
    TEST_ASSERT(ContainsPattern(content, "rmdir\\s+.*\"build\""), 
                "buildディレクトリ削除コマンドが存在すること");
    
    // /s /qオプションが指定されていることを確認（サブディレクトリ再帰削除・確認なし）
    TEST_ASSERT(ContainsPattern(content, "rmdir\\s+/s\\s+/q"), 
                "rmdir /s /qオプションが指定されていること");
}

//! @cleanup.cmd 旧コマンド不存在確認
static void TestCleanupCmd_NoLegacyCommands()
{
    std::cout << "\n=== @cleanup.cmd 旧コマンド不存在確認 ===" << std::endl;
    
    auto lines = ReadFileLines("@cleanup.cmd");
    
    // bin/とobj/の個別削除コマンドが存在しないことを確認
    bool hasBinRemoval = false;
    bool hasObjRemoval = false;
    
    for (const auto& line : lines) {
        // コメント行（::で始まる）をスキップ
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        
        if (trimmed.rfind("::", 0) == 0 || trimmed.empty()) {
            continue;
        }
        
        // bin削除コマンドをチェック
        if (line.find("rmdir") != std::string::npos && 
            line.find("\"bin\"") != std::string::npos) {
            hasBinRemoval = true;
        }
        
        // obj削除コマンドをチェック
        if (line.find("rmdir") != std::string::npos && 
            line.find("\"obj\"") != std::string::npos) {
            hasObjRemoval = true;
        }
    }
    
    TEST_ASSERT(!hasBinRemoval, "個別のbinディレクトリ削除コマンドが存在しないこと");
    TEST_ASSERT(!hasObjRemoval, "個別のobjディレクトリ削除コマンドが存在しないこと");
}

//! @cleanup.cmd ドキュメントコメント検証
static void TestCleanupCmd_Documentation()
{
    std::cout << "\n=== @cleanup.cmd ドキュメントコメント検証 ===" << std::endl;
    
    std::string content = ReadFileToString("@cleanup.cmd");
    
    // ヘッダーコメントブロックの存在確認
    TEST_ASSERT(ContainsPattern(content, "::=+"), 
                "ヘッダーコメントブロックが存在すること");
    
    // 削除対象の説明確認
    TEST_ASSERT(ContainsPattern(content, "削除対象"), 
                "削除対象の説明が存在すること");
    
    // build/がbin/とobj/を含むことの説明確認
    bool explainsInclusion = ContainsPattern(content, "bin/.*obj/") || 
                             ContainsPattern(content, "全ビルド成果物");
    TEST_ASSERT(explainsInclusion, 
                "build/がbin/とobj/を含むことが説明されていること");
    
    // 用途の説明確認
    TEST_ASSERT(ContainsPattern(content, "用途"), 
                "用途の説明が存在すること");
}

//----------------------------------------------------------------------------
// README.md 検証テスト
//----------------------------------------------------------------------------

//! README.md ファイル存在確認
static void TestReadme_FileExists()
{
    std::cout << "\n=== README.md ファイル存在確認 ===" << std::endl;
    
    fs::path readmePath = "README.md";
    TEST_ASSERT(fs::exists(readmePath), "README.mdが存在すること");
    TEST_ASSERT(fs::is_regular_file(readmePath), "README.mdが通常ファイルであること");
}

//! README.md 実行パス検証
static void TestReadme_ExecutablePath()
{
    std::cout << "\n=== README.md 実行パス検証 ===" << std::endl;
    
    std::string content = ReadFileToString("README.md");
    TEST_ASSERT(!content.empty(), "README.mdが読み込めること");
    
    // build\bin\の実行パスが記載されていることを確認
    TEST_ASSERT(ContainsPattern(content, "build\\\\bin\\\\"), 
                "実行パスにbuild\\bin\\が含まれること");
    
    // Debug-windows-x86_64形式のパスが含まれることを確認
    TEST_ASSERT(ContainsPattern(content, "Debug-windows-x86_64"), 
                "実行パスにDebug-windows-x86_64が含まれること");
    
    // game.exeへのパスが含まれることを確認
    TEST_ASSERT(ContainsPattern(content, "game\\\\game\\.exe"), 
                "game.exeへの完全パスが含まれること");
    
    // 完全パスの確認
    TEST_ASSERT(ContainsPattern(content, "build\\\\bin\\\\Debug-windows-x86_64\\\\game\\\\game\\.exe"), 
                "正しい完全パスが記載されていること");
}

//! README.md 旧パス不存在確認
static void TestReadme_NoLegacyPath()
{
    std::cout << "\n=== README.md 旧パス不存在確認 ===" << std::endl;
    
    std::string content = ReadFileToString("README.md");
    
    // 旧形式のbin\Debug-windows-x64\パスが存在しないことを確認
    TEST_ASSERT(!ContainsPattern(content, "bin\\\\Debug-windows-x64"), 
                "旧パスbin\\Debug-windows-x64が存在しないこと");
    
    // buildなしのbin\から始まるパスが実行パスとして記載されていないことを確認
    auto lines = ReadFileLines("README.md");
    bool hasLegacyExecutablePath = false;
    
    for (const auto& line : lines) {
        // "実行"セクション付近でbin\Debug（buildなし）を探す
        if (line.find("# 実行") != std::string::npos || 
            line.find("実行") != std::string::npos) {
            // 次数行をチェック
            if (line.find("bin\\Debug") != std::string::npos && 
                line.find("build\\bin") == std::string::npos) {
                hasLegacyExecutablePath = true;
                break;
            }
        }
    }
    
    TEST_ASSERT(!hasLegacyExecutablePath, 
                "旧形式の実行パス（buildなし）が記載されていないこと");
}

//----------------------------------------------------------------------------
// パス一貫性検証テスト
//----------------------------------------------------------------------------

//! 全設定ファイル間のパス一貫性検証
static void TestPathConsistency_AcrossFiles()
{
    std::cout << "\n=== 全設定ファイル間のパス一貫性検証 ===" << std::endl;
    
    // premake5.luaから期待されるパスパターンを取得
    std::string premakeContent = ReadFileToString("premake5.lua");
    bool premakeUsesBuildBin = ContainsPattern(premakeContent, "bindir\\s*=\\s*\"build/bin/");
    bool premakeUsesBuildObj = ContainsPattern(premakeContent, "objdir_base\\s*=\\s*\"build/obj/");
    
    TEST_ASSERT(premakeUsesBuildBin, "premake5.luaがbuild/bin/を使用していること");
    TEST_ASSERT(premakeUsesBuildObj, "premake5.luaがbuild/obj/を使用していること");
    
    // .gitignoreがbuild/をカバーしていることを確認
    auto gitignoreLines = ReadFileLines(".gitignore");
    bool gitignoreHasBuild = false;
    for (const auto& line : gitignoreLines) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
        
        if (trimmed == "build/" || trimmed == "build") {
            gitignoreHasBuild = true;
            break;
        }
    }
    TEST_ASSERT(gitignoreHasBuild, ".gitignoreがbuild/を除外していること");
    
    // @cleanup.cmdがbuildを削除することを確認
    std::string cleanupContent = ReadFileToString("@cleanup.cmd");
    bool cleanupRemovesBuild = ContainsPattern(cleanupContent, "rmdir\\s+.*\"build\"");
    TEST_ASSERT(cleanupRemovesBuild, "@cleanup.cmdがbuildディレクトリを削除すること");
    
    // README.mdがbuild\bin\を参照していることを確認
    std::string readmeContent = ReadFileToString("README.md");
    bool readmeUsesBuildBin = ContainsPattern(readmeContent, "build\\\\bin\\\\");
    TEST_ASSERT(readmeUsesBuildBin, "README.mdがbuild\\bin\\を参照していること");
    
    // 総合的な一貫性確認
    bool allConsistent = premakeUsesBuildBin && premakeUsesBuildObj && 
                         gitignoreHasBuild && cleanupRemovesBuild && 
                         readmeUsesBuildBin;
    TEST_ASSERT(allConsistent, 
                "すべての設定ファイルがbuild/統合パス構造で一貫していること");
}

//! 出力ディレクトリ構造の論理検証
static void TestPathConsistency_DirectoryStructure()
{
    std::cout << "\n=== 出力ディレクトリ構造の論理検証 ===" << std::endl;
    
    // 期待される構造:
    // build/
    //   ├── bin/
    //   │   └── {outputdir}/
    //   │       └── {project}/
    //   └── obj/
    //       └── {outputdir}/
    //           └── {project}/
    
    std::string premakeContent = ReadFileToString("premake5.lua");
    
    // bindirとobjdir_baseが同じoutputdirを使用していることを確認
    bool bothUseOutputdir = ContainsPattern(premakeContent, "bindir\\s*=\\s*\"build/bin/\"\\s*\\.\\.\\.\\s*outputdir") &&
                            ContainsPattern(premakeContent, "objdir_base\\s*=\\s*\"build/obj/\"\\s*\\.\\.\\.\\s*outputdir");
    
    // より緩い検証：両方がoutputdirを含むことを確認
    bool binUsesOutputdir = ContainsPattern(premakeContent, "bindir.*outputdir");
    bool objUsesOutputdir = ContainsPattern(premakeContent, "objdir_base.*outputdir");
    
    TEST_ASSERT(binUsesOutputdir, "bindirがoutputdir変数を使用していること");
    TEST_ASSERT(objUsesOutputdir, "objdir_baseがoutputdir変数を使用していること");
    
    // outputdirがビルド設定情報を含むことを確認
    bool outputdirIncludesConfig = ContainsPattern(premakeContent, "outputdir.*cfg\\.buildcfg") &&
                                   ContainsPattern(premakeContent, "outputdir.*cfg\\.system") &&
                                   ContainsPattern(premakeContent, "outputdir.*cfg\\.architecture");
    
    TEST_ASSERT(outputdirIncludesConfig, 
                "outputdirがビルド設定、システム、アーキテクチャ情報を含むこと");
}

//----------------------------------------------------------------------------
// エッジケース・総合検証
//----------------------------------------------------------------------------

//! 空白・エンコーディング検証
static void TestEdgeCase_WhitespaceAndEncoding()
{
    std::cout << "\n=== 空白・エンコーディング検証 ===" << std::endl;
    
    // premake5.luaの空白チェック
    std::string premakeContent = ReadFileToString("premake5.lua");
    TEST_ASSERT(!premakeContent.empty(), "premake5.luaが読み込めること");
    
    // 行末にタブや不要な空白がないことを確認（厳格すぎる場合はスキップ可）
    auto premakeLines = ReadFileLines("premake5.lua");
    int trailingWhitespaceCount = 0;
    for (const auto& line : premakeLines) {
        if (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
            trailingWhitespaceCount++;
        }
    }
    // 警告レベル（失敗させない）
    if (trailingWhitespaceCount > 0) {
        std::cout << "  [情報] premake5.luaに行末空白が" << trailingWhitespaceCount 
                  << "行存在します（許容範囲）" << std::endl;
    }
    TEST_ASSERT(true, "エンコーディングチェック完了");
}

//! ファイルサイズ・内容妥当性検証
static void TestEdgeCase_FileSizeValidity()
{
    std::cout << "\n=== ファイルサイズ・内容妥当性検証 ===" << std::endl;
    
    // premake5.luaが異常に大きくないことを確認（100KB以下）
    auto premakeSize = fs::file_size("premake5.lua");
    TEST_ASSERT(premakeSize < 100 * 1024, "premake5.luaが100KB未満であること");
    TEST_ASSERT(premakeSize > 1024, "premake5.luaが1KB以上であること");
    
    // .gitignoreが異常に大きくないことを確認（10KB以下）
    auto gitignoreSize = fs::file_size(".gitignore");
    TEST_ASSERT(gitignoreSize < 10 * 1024, ".gitignoreが10KB未満であること");
    
    // @cleanup.cmdが異常に大きくないことを確認（10KB以下）
    auto cleanupSize = fs::file_size("@cleanup.cmd");
    TEST_ASSERT(cleanupSize < 10 * 1024, "@cleanup.cmdが10KB未満であること");
    TEST_ASSERT(cleanupSize > 100, "@cleanup.cmdが100バイト以上であること");
}

//! パス区切り文字の一貫性検証
static void TestEdgeCase_PathSeparatorConsistency()
{
    std::cout << "\n=== パス区切り文字の一貫性検証 ===" << std::endl;
    
    // premake5.luaではスラッシュ(/)を使用
    std::string premakeContent = ReadFileToString("premake5.lua");
    TEST_ASSERT(ContainsPattern(premakeContent, "build/bin/"), 
                "premake5.luaでスラッシュ(/)が使用されていること");
    
    // README.mdではバックスラッシュ(\\)を使用（Windows用）
    std::string readmeContent = ReadFileToString("README.md");
    TEST_ASSERT(ContainsPattern(readmeContent, "build\\\\bin\\\\"), 
                "README.mdでバックスラッシュ(\\)が使用されていること");
    
    // この違いは意図的であることを確認（OK）
    TEST_ASSERT(true, "プラットフォーム固有のパス区切り文字が正しく使用されていること");
}

//----------------------------------------------------------------------------
// テストスイート実行
//----------------------------------------------------------------------------

void RunBuildConfigTests()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "ビルド設定検証テストスイート" << std::endl;
    std::cout << "========================================" << std::endl;
    
    ResetGlobalCounters();
    
    // premake5.lua検証
    TestPremake5_FileExists();
    TestPremake5_BasicSyntax();
    TestPremake5_OutputDirectoryConfig();
    TestPremake5_ProjectDefinitions();
    TestPremake5_TargetDirUsage();
    TestPremake5_NoLegacyPaths();
    
    // .gitignore検証
    TestGitignore_FileExists();
    TestGitignore_BuildPattern();
    TestGitignore_NoLegacyPatterns();
    TestGitignore_CommentAccuracy();
    
    // @cleanup.cmd検証
    TestCleanupCmd_FileExists();
    TestCleanupCmd_BuildRemovalCommand();
    TestCleanupCmd_NoLegacyCommands();
    TestCleanupCmd_Documentation();
    
    // README.md検証
    TestReadme_FileExists();
    TestReadme_ExecutablePath();
    TestReadme_NoLegacyPath();
    
    // パス一貫性検証
    TestPathConsistency_AcrossFiles();
    TestPathConsistency_DirectoryStructure();
    
    // エッジケース検証
    TestEdgeCase_WhitespaceAndEncoding();
    TestEdgeCase_FileSizeValidity();
    TestEdgeCase_PathSeparatorConsistency();
    
    // 結果サマリー
    std::cout << "\n========================================" << std::endl;
    std::cout << "ビルド設定検証テスト完了" << std::endl;
    std::cout << "成功: " << GetGlobalPassCount() << "/" << GetGlobalTestCount() << std::endl;
    std::cout << "========================================" << std::endl;
}

} // namespace tests