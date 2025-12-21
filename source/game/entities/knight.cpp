//----------------------------------------------------------------------------
//! @file   knight.cpp
//! @brief  Knight種族クラス実装
//----------------------------------------------------------------------------
#include "knight.h"
#include "engine/texture/texture_manager.h"
#include "engine/math/color.h"
#include "common/logging/logging.h"
#include <vector>

//----------------------------------------------------------------------------
Knight::Knight(const std::string& id)
    : Individual(id)
{
    // Knightはアニメーションなし（単一フレーム）
    animRows_ = 1;
    animCols_ = 1;
    animFrameInterval_ = 1;

    // ステータス設定（タンク型）
    maxHp_ = kDefaultHp;
    hp_ = maxHp_;
    attackDamage_ = kDefaultDamage;
    moveSpeed_ = kDefaultSpeed;
}

//----------------------------------------------------------------------------
void Knight::SetupTexture()
{
    // 白い■テクスチャを動的生成
    std::vector<uint32_t> pixels(kTextureSize * kTextureSize, 0xFFFFFFFF);
    texture_ = TextureManager::Get().Create2D(
        kTextureSize, kTextureSize,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D11_BIND_SHADER_RESOURCE,
        pixels.data(),
        kTextureSize * sizeof(uint32_t)
    );

    if (sprite_ && texture_) {
        sprite_->SetTexture(texture_.get());
        sprite_->SetSortingLayer(10);

        // 色を設定（白テクスチャに乗算）
        sprite_->SetColor(color_);

        // Pivot設定（中心）
        sprite_->SetPivot(
            static_cast<float>(kTextureSize) * 0.5f,
            static_cast<float>(kTextureSize) * 0.5f
        );

        // サイズ設定（少し大きめ）
        sprite_->SetSize(Vector2(48.0f, 48.0f));
    }
}

//----------------------------------------------------------------------------
void Knight::SetupCollider()
{
    // 基底クラスのコライダー設定を呼び出し
    Individual::SetupCollider();

    // Knightは少し大きめのコライダーにリサイズ
    if (collider_ != nullptr) {
        collider_->SetBounds(Vector2(-24, -24), Vector2(24, 24));
    }
}

//----------------------------------------------------------------------------
void Knight::SetColor(const Color& color)
{
    color_ = color;
    if (sprite_) {
        sprite_->SetColor(color_);
    }
}

//----------------------------------------------------------------------------
void Knight::Attack(Individual* target)
{
    if (!target || !target->IsAlive()) return;
    if (!IsAlive()) return;

    // Knightはアニメーションなし、即座にダメージ
    // TODO: 攻撃エフェクト追加

    // ダメージを与える
    target->TakeDamage(attackDamage_);

    LOG_INFO("[Knight] " + id_ + " attacks " + target->GetId() + " for " + std::to_string(attackDamage_) + " damage");
}
