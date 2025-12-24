//----------------------------------------------------------------------------
//! @file   stage_loader.h
//! @brief  ステージデータ読み込み
//----------------------------------------------------------------------------
#pragma once

#include "stage_data.h"
#include <string>
#include <vector>

//----------------------------------------------------------------------------
//! @brief ステージローダークラス
//! @details テキストファイルからステージデータを読み込む
//!
//! ファイル形式:
//! @code
//! [Stage]
//! name = ステージ名
//! playerX = 640
//! playerY = 360
//!
//! [Groups]
//! group1 = Elf, 3, 200, 200, 100, 300
//! group2 = Knight, 2, 600, 400, 80, 250
//!
//! [Bonds]
//! bond1 = group1, group2, Basic
//! @endcode
//----------------------------------------------------------------------------
class StageLoader
{
public:
    //! @brief ファイルからステージデータを読み込む
    //! @param filePath ファイルパス（例: "assets:/stages/stage1.txt"）
    //! @return 読み込んだデータ（失敗時は空のStageData）
    [[nodiscard]] static StageData Load(const std::string& filePath);

private:
    //! @brief 文字列の前後の空白を削除
    [[nodiscard]] static std::string Trim(const std::string& str);

    //! @brief 文字列をカンマで分割
    [[nodiscard]] static std::vector<std::string> SplitByComma(const std::string& str);

    //! @brief Groupの行をパース
    [[nodiscard]] static GroupData ParseGroup(const std::string& id, const std::string& value);

    //! @brief Bondの行をパース
    [[nodiscard]] static BondData ParseBond(const std::string& value);
};
