//----------------------------------------------------------------------------
//! @file   stage_background.h
//! @brief  ステージ背景 - レイヤー分けされた背景描画
//----------------------------------------------------------------------------
#pragma once

#include "dx11/gpu/texture.h"
#include "engine/math/math_types.h"
#include "engine/math/color.h"
#include <vector>
#include <string>
#include <random>

// 前方宣言
class SpriteBatch;

//----------------------------------------------------------------------------
//! @brief ステージ背景クラス
//! @details レイヤー分けされた背景を描画する
//!          - 背景(haikei): sortingLayer -200
//!          - 地面(ground): sortingLayer -150
//!          - 装飾(奥): sortingLayer -120
//!          - 装飾(中): sortingLayer -100
//!          - 装飾(手前): sortingLayer -80
//----------------------------------------------------------------------------
class StageBackground
{
public:
    //! @brief ステージ背景を初期化
    //! @param stageId ステージID (例: "stage1")
    //! @param screenWidth 画面幅
    //! @param screenHeight 画面高さ
    void Initialize(const std::string& stageId, float screenWidth, float screenHeight);

    //! @brief 背景を描画
    //! @param spriteBatch スプライトバッチ
    void Render(SpriteBatch& spriteBatch);

    //! @brief リソース解放
    void Shutdown();

private:
    //! @brief 装飾オブジェクト
    struct DecorationObject
    {
        TexturePtr texture;         //!< テクスチャ
        Vector2 position;           //!< 位置
        Vector2 scale;              //!< スケール
        float rotation;             //!< 回転（ラジアン）
        int sortingLayer;           //!< ソートレイヤー
    };

    //! @brief 装飾をランダム配置
    //! @param stageId ステージID
    //! @param screenWidth 画面幅
    //! @param screenHeight 画面高さ
    void PlaceDecorations(const std::string& stageId, float screenWidth, float screenHeight);

    //! @brief 装飾を追加
    //! @param texture テクスチャ
    //! @param position 位置
    //! @param sortingLayer ソートレイヤー
    //! @param scale スケール（デフォルト1.0）
    //! @param rotation 回転（デフォルト0.0）
    void AddDecoration(TexturePtr texture, const Vector2& position, int sortingLayer,
                       const Vector2& scale = Vector2::One, float rotation = 0.0f);

    // 地面テクスチャ（タイル用）
    TexturePtr groundTexture_;

    // 装飾オブジェクト（タイル地面 + 装飾）
    std::vector<DecorationObject> decorations_;

    // 乱数生成器
    std::mt19937 rng_;

    // 画面サイズ
    float screenWidth_ = 0.0f;
    float screenHeight_ = 0.0f;
};
