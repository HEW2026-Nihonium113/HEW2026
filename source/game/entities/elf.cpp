//----------------------------------------------------------------------------
//! @file   elf.cpp
//! @brief  Elf種族クラス実装
//----------------------------------------------------------------------------
#include "elf.h"
#include "player.h"
#include "arrow_manager.h"
#include "engine/texture/texture_manager.h"
#include "common/logging/logging.h"

//----------------------------------------------------------------------------
Elf::Elf(const std::string& id)
    : Individual(id)
{
    // アニメーション設定
    animRows_ = kAnimRows;
    animCols_ = kAnimCols;
    animFrameInterval_ = 6;

    // ステータス設定
    maxHp_ = kDefaultHp;
    hp_ = maxHp_;
    attackDamage_ = kDefaultDamage;
    moveSpeed_ = kDefaultSpeed;
}

//----------------------------------------------------------------------------
void Elf::SetupTexture()
{
    // elf_sprite.pngをロード
    texture_ = TextureManager::Get().LoadTexture2D("elf_sprite.png");

    if (sprite_ && texture_) {
        sprite_->SetTexture(texture_.get());
        sprite_->SetSortingLayer(10);

        // スケール設定
        if (transform_) {
            transform_->SetScale(0.3f);
        }

        // Pivot設定（スプライトシートのフレーム中心）
        float frameWidth = static_cast<float>(texture_->Width()) / kAnimCols;
        float frameHeight = static_cast<float>(texture_->Height()) / kAnimRows;
        sprite_->SetPivotFromCenter(frameWidth, frameHeight, 0.0f, 0.0f);
    }
}

//----------------------------------------------------------------------------
void Elf::SetupAnimator()
{
    if (!animator_) return;

    // アニメーション行設定
    // Row 0: Idle (1フレーム)
    // Row 1: Walk (4フレーム)
    // Row 2: Attack (3フレーム)
    // Row 3: Death (2フレーム)
    animator_->SetRowFrameCount(0, 1, 12);   // Idle: 1フレーム, 12F間隔
    animator_->SetRowFrameCount(1, 4, 6);    // Walk: 4フレーム, 6F間隔
    animator_->SetRowFrameCount(2, 3, 8);    // Attack: 3フレーム, 8F間隔
    animator_->SetRowFrameCount(3, 2, 10);   // Death: 2フレーム, 10F間隔

    // 初期状態はIdle
    animator_->SetRow(0);
    animator_->SetLooping(true);
}

//----------------------------------------------------------------------------
void Elf::Attack(Individual* target)
{
    if (!target || !target->IsAlive()) return;
    if (!IsAlive()) return;

    // 攻撃アニメーション開始
    if (animator_) {
        animator_->SetRow(2);  // Attack行
        animator_->SetLooping(false);
        animator_->Reset();
    }

    // 矢を発射
    Vector2 startPos = GetPosition();
    ArrowManager::Get().Shoot(this, target, startPos, attackDamage_);

    LOG_INFO("[Elf] " + id_ + " shoots arrow at " + target->GetId());
}

//----------------------------------------------------------------------------
void Elf::AttackPlayer(Player* target)
{
    if (!target || !target->IsAlive()) return;
    if (!IsAlive()) return;

    // 攻撃アニメーション開始
    if (animator_) {
        animator_->SetRow(2);  // Attack行
        animator_->SetLooping(false);
        animator_->Reset();
    }

    // 矢を発射（プレイヤー対象）
    Vector2 startPos = GetPosition();
    ArrowManager::Get().ShootAtPlayer(this, target, startPos, attackDamage_);

    LOG_INFO("[Elf] " + id_ + " shoots arrow at Player");
}
