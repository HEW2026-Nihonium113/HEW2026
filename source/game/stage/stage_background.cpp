//----------------------------------------------------------------------------
//! @file   stage_background.cpp
//! @brief  ステージ背景実装
//----------------------------------------------------------------------------
#include "stage_background.h"
#include "engine/texture/texture_manager.h"
#include "engine/c_systems/sprite_batch.h"
#include "common/logging/logging.h"
#include <algorithm>

//----------------------------------------------------------------------------
void StageBackground::Initialize(const std::string& stageId, float screenWidth, float screenHeight)
{
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;

    // 乱数初期化
    std::random_device rd;
    rng_ = std::mt19937(rd());

    // テクスチャパスのベース（TextureManagerは相対パス）
    std::string basePath = stageId + "/";

    // 地面テクスチャ読み込み（タイル配置用）
    groundTexture_ = TextureManager::Get().LoadTexture2D(basePath + "ground.png");
    if (groundTexture_) {
        float texW = static_cast<float>(groundTexture_->Width());
        float texH = static_cast<float>(groundTexture_->Height());

        // つなぎ目を目立たなくするためオーバーラップ
        float overlapX = texW * 0.05f;  // 5%オーバーラップ
        float overlapY = texH * 0.05f;
        float stepX = texW - overlapX;
        float stepY = texH - overlapY;

        // ステージ全体をカバーするタイル数を計算
        int tilesX = static_cast<int>(std::ceil(screenWidth / stepX)) + 1;
        int tilesY = static_cast<int>(std::ceil(screenHeight / stepY)) + 1;

        // タイルを配置（装飾として追加）
        for (int y = 0; y < tilesY; ++y) {
            for (int x = 0; x < tilesX; ++x) {
                Vector2 pos(x * stepX + texW * 0.5f, y * stepY + texH * 0.5f);
                AddDecoration(groundTexture_, pos, -200, Vector2::One, 0.0f);
            }
        }

        LOG_INFO("[StageBackground] Ground tiles: " + std::to_string(tilesX) + "x" + std::to_string(tilesY));
    }

    // 装飾を配置
    PlaceDecorations(stageId, screenWidth, screenHeight);

    LOG_INFO("[StageBackground] Initialized with " + std::to_string(decorations_.size()) + " decorations");
}

//----------------------------------------------------------------------------
void StageBackground::PlaceDecorations(const std::string& stageId, float screenWidth, float screenHeight)
{
    std::string basePath = stageId + "/";

    // 分布設定
    std::uniform_real_distribution<float> xDist(0.0f, screenWidth);
    std::uniform_real_distribution<float> yFullDist(screenHeight * 0.3f, screenHeight);
    std::uniform_real_distribution<float> yGroundDist(screenHeight * 0.6f, screenHeight * 0.95f);
    std::uniform_real_distribution<float> scaleDist(0.8f, 1.2f);
    std::uniform_real_distribution<float> smallScaleDist(0.5f, 1.0f);
    std::uniform_real_distribution<float> rotationDist(-0.1f, 0.1f);
    std::uniform_int_distribution<int> countDist5_8(5, 8);
    std::uniform_int_distribution<int> countDist10_15(10, 15);
    std::uniform_int_distribution<int> countDist15_25(15, 25);

    // 遺跡・木（奥レイヤー -120）
    {
        std::vector<std::string> bigObjects = {
            "ruins fragment.png",
            "ruins fragment 2.png",
            "ruins fragment 3.png",
            "tree.png"
        };

        int count = countDist5_8(rng_);
        std::uniform_int_distribution<size_t> objDist(0, bigObjects.size() - 1);

        for (int i = 0; i < count; ++i) {
            TexturePtr tex = TextureManager::Get().LoadTexture2D(basePath + bigObjects[objDist(rng_)]);
            if (tex) {
                Vector2 pos(xDist(rng_), yFullDist(rng_));
                Vector2 scale(scaleDist(rng_), scaleDist(rng_));
                AddDecoration(tex, pos, -120, scale, rotationDist(rng_));
            }
        }
    }

    // 草・石（中レイヤー -100）
    {
        std::vector<std::string> mediumObjects = {
            "grass big.png",
            "grass long.png",
            "stone 1.png",
            "stone 2.png",
            "stone 3.png",
            "stone 4.png",
            "stone 5.png",
            "stone 6.png",
            "stone 7.png",
            "stone 8.png"
        };

        int count = countDist10_15(rng_);
        std::uniform_int_distribution<size_t> objDist(0, mediumObjects.size() - 1);

        for (int i = 0; i < count; ++i) {
            TexturePtr tex = TextureManager::Get().LoadTexture2D(basePath + mediumObjects[objDist(rng_)]);
            if (tex) {
                Vector2 pos(xDist(rng_), yGroundDist(rng_));
                Vector2 scale(scaleDist(rng_), scaleDist(rng_));
                AddDecoration(tex, pos, -100, scale, rotationDist(rng_));
            }
        }
    }

    // 葉・木片・焚火・小さい草（手前レイヤー -80）
    {
        std::vector<std::string> smallObjects = {
            "grass small.png",
            "leaf 1.png",
            "leaf 2.png",
            "leaf 3.png",
            "leaf 4.png",
            "leaf 5.png",
            "leaf 6.png",
            "leaf 7.png",
            "leaf 8.png",
            "wood chips 1.png",
            "wood chips 2.png",
            "wood chips 3.png",
            "wood chips 4.png",
            "wood chips 5.png",
            "wood chips 6.png"
        };

        int count = countDist15_25(rng_);
        std::uniform_int_distribution<size_t> objDist(0, smallObjects.size() - 1);

        for (int i = 0; i < count; ++i) {
            TexturePtr tex = TextureManager::Get().LoadTexture2D(basePath + smallObjects[objDist(rng_)]);
            if (tex) {
                Vector2 pos(xDist(rng_), yFullDist(rng_));
                Vector2 scale(smallScaleDist(rng_), smallScaleDist(rng_));
                AddDecoration(tex, pos, -80, scale, rotationDist(rng_));
            }
        }

        // 焚火（1つだけ、画面中央付近）
        TexturePtr bonfire = TextureManager::Get().LoadTexture2D(basePath + "bonfire.png");
        if (bonfire) {
            Vector2 pos(screenWidth * 0.5f + xDist(rng_) * 0.2f - screenWidth * 0.1f,
                        screenHeight * 0.75f);
            AddDecoration(bonfire, pos, -80, Vector2::One, 0.0f);
        }
    }
}

//----------------------------------------------------------------------------
void StageBackground::AddDecoration(TexturePtr texture, const Vector2& position,
                                     int sortingLayer, const Vector2& scale, float rotation)
{
    DecorationObject obj;
    obj.texture = texture;
    obj.position = position;
    obj.scale = scale;
    obj.rotation = rotation;
    obj.sortingLayer = sortingLayer;
    decorations_.push_back(std::move(obj));
}

//----------------------------------------------------------------------------
void StageBackground::Render(SpriteBatch& spriteBatch)
{
    // 装飾描画（タイル地面 + 装飾オブジェクト）
    for (const DecorationObject& obj : decorations_) {
        if (!obj.texture) continue;

        float texW = static_cast<float>(obj.texture->Width());
        float texH = static_cast<float>(obj.texture->Height());
        Vector2 origin(texW * 0.5f, texH * 0.5f);

        spriteBatch.Draw(
            obj.texture.get(),
            obj.position,
            Color(1.0f, 1.0f, 1.0f, 1.0f),
            obj.rotation,
            origin,
            obj.scale,
            false,
            false,
            obj.sortingLayer,
            0
        );
    }
}

//----------------------------------------------------------------------------
void StageBackground::Shutdown()
{
    decorations_.clear();
    groundTexture_.reset();

    LOG_INFO("[StageBackground] Shutdown");
}
