//----------------------------------------------------------------------------
//! @file   stage_loader.cpp
//! @brief  ステージデータ読み込み実装
//----------------------------------------------------------------------------
#include "stage_loader.h"
#include "engine/fs/file_system_manager.h"
#include "common/logging/logging.h"
#include <sstream>

//----------------------------------------------------------------------------
std::string StageLoader::Trim(const std::string& str)
{
    if (str.empty())
    {
        return str;
    }

    size_t start = 0;
    size_t end = str.size();

    // 先頭の空白をスキップ
    while (start < end && (str[start] == ' ' || str[start] == '\t' || str[start] == '\r'))
    {
        start++;
    }

    // 末尾の空白をスキップ
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\r'))
    {
        end--;
    }

    return str.substr(start, end - start);
}

//----------------------------------------------------------------------------
std::vector<std::string> StageLoader::SplitByComma(const std::string& str)
{
    std::vector<std::string> result;
    std::string current;

    for (size_t i = 0; i < str.size(); i++)
    {
        if (str[i] == ',')
        {
            result.push_back(Trim(current));
            current.clear();
        }
        else
        {
            current += str[i];
        }
    }

    // 最後の要素を追加
    std::string trimmed = Trim(current);
    if (!trimmed.empty())
    {
        result.push_back(trimmed);
    }

    return result;
}

//----------------------------------------------------------------------------
GroupData StageLoader::ParseGroup(const std::string& id, const std::string& value)
{
    GroupData data;
    data.id = id;

    // カンマで分割: "Elf, 3, 200, 200, 100, 300"
    std::vector<std::string> parts = SplitByComma(value);

    // 最低5要素必要（種族, 個体数, X, Y, 脅威度）
    if (parts.size() >= 5)
    {
        data.species = parts[0];

        try
        {
            data.count = std::stoi(parts[1]);
            data.x = std::stof(parts[2]);
            data.y = std::stof(parts[3]);
            data.threat = std::stof(parts[4]);

            // 6番目があれば索敵範囲
            if (parts.size() >= 6)
            {
                data.detectionRange = std::stof(parts[5]);
            }
        }
        catch (const std::exception& e)
        {
            LOG_WARN("[StageLoader] グループデータのパースエラー: " + id + " - " + e.what());
        }
    }
    else
    {
        LOG_WARN("[StageLoader] グループデータが不足: " + id + " (" + std::to_string(parts.size()) + "個の要素)");
    }

    return data;
}

//----------------------------------------------------------------------------
BondData StageLoader::ParseBond(const std::string& value)
{
    BondData data;

    // カンマで分割: "group1, group2, Basic"
    std::vector<std::string> parts = SplitByComma(value);

    if (parts.size() >= 2)
    {
        data.fromId = parts[0];
        data.toId = parts[1];

        // 3番目があれば縁タイプ
        if (parts.size() >= 3)
        {
            data.type = parts[2];
        }
        else
        {
            data.type = "Basic";  // デフォルト
        }
    }
    else
    {
        LOG_WARN("[StageLoader] 縁データが不足");
    }

    return data;
}

//----------------------------------------------------------------------------
StageData StageLoader::Load(const std::string& filePath)
{
    StageData stageData;

    // ファイルを読み込む（ReadFileAsTextでstd::stringを取得）
    std::string content = FileSystemManager::Get().ReadFileAsText(filePath);

    if (content.empty())
    {
        LOG_ERROR("[StageLoader] ステージファイルが読めない: " + filePath);
        return stageData;
    }

    LOG_DEBUG("[StageLoader] ステージファイル読み込み開始: " + filePath);

    // 現在のセクション
    std::string currentSection;

    // 1行ずつ処理
    std::istringstream stream(content);
    std::string line;
    int lineNumber = 0;

    while (std::getline(stream, line))
    {
        lineNumber++;
        line = Trim(line);

        // 空行はスキップ
        if (line.empty())
        {
            continue;
        }

        // コメント行はスキップ（#で始まる）
        if (line[0] == '#')
        {
            continue;
        }

        // セクション判定（[Stage], [Groups], [Bonds]）
        if (line.size() >= 2 && line[0] == '[' && line[line.size() - 1] == ']')
        {
            currentSection = line.substr(1, line.size() - 2);
            LOG_DEBUG("[StageLoader] セクション: " + currentSection);
            continue;
        }

        // = で分割（key = value）
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
        {
            LOG_WARN("[StageLoader] 行" + std::to_string(lineNumber) + ": '=' が見つからない: " + line);
            continue;
        }

        std::string key = Trim(line.substr(0, eqPos));
        std::string value = Trim(line.substr(eqPos + 1));

        // セクションごとに処理
        if (currentSection == "Stage")
        {
            if (key == "name")
            {
                stageData.name = value;
            }
            else if (key == "playerX")
            {
                try
                {
                    stageData.playerX = std::stof(value);
                }
                catch (...)
                {
                    LOG_WARN("[StageLoader] playerX のパースエラー: " + value);
                }
            }
            else if (key == "playerY")
            {
                try
                {
                    stageData.playerY = std::stof(value);
                }
                catch (...)
                {
                    LOG_WARN("[StageLoader] playerY のパースエラー: " + value);
                }
            }
        }
        else if (currentSection == "Groups")
        {
            GroupData group = ParseGroup(key, value);
            if (!group.species.empty())
            {
                stageData.groups.push_back(group);
                LOG_DEBUG("[StageLoader] グループ追加: " + group.id + " (" + group.species + " x" + std::to_string(group.count) + ")");
            }
        }
        else if (currentSection == "Bonds")
        {
            BondData bond = ParseBond(value);
            if (!bond.fromId.empty() && !bond.toId.empty())
            {
                stageData.bonds.push_back(bond);
                LOG_DEBUG("[StageLoader] 縁追加: " + bond.fromId + " <-> " + bond.toId + " (" + bond.type + ")");
            }
        }
    }

    std::string stageName = stageData.name.empty() ? "(無名)" : stageData.name;
    LOG_INFO("[StageLoader] ステージ読み込み完了: " + stageName +
             " (グループ: " + std::to_string(stageData.groups.size()) +
             ", 縁: " + std::to_string(stageData.bonds.size()) + ")");

    return stageData;
}
