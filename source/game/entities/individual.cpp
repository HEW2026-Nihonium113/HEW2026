//----------------------------------------------------------------------------
//! @file   individual.cpp
//! @brief  Individual基底クラス実装
//----------------------------------------------------------------------------
#include "individual.h"
#include "player.h"
#include "engine/c_systems/sprite_batch.h"
#include "common/logging/logging.h"

//----------------------------------------------------------------------------
Individual::Individual(const std::string& id)
    : id_(id)
{
}

//----------------------------------------------------------------------------
Individual::~Individual()
{
    Shutdown();
}

//----------------------------------------------------------------------------
void Individual::Initialize(const Vector2& position)
{
    // GameObject作成
    gameObject_ = std::make_unique<GameObject>(id_);

    // Transform2D
    transform_ = gameObject_->AddComponent<Transform2D>();
    transform_->SetPosition(position);

    // SpriteRenderer
    sprite_ = gameObject_->AddComponent<SpriteRenderer>();

    // テクスチャセットアップ（派生クラスで実装）
    SetupTexture();

    // Animator（派生クラスで設定された値を使用）
    if (animRows_ > 1 || animCols_ > 1) {
        animator_ = gameObject_->AddComponent<Animator>(animRows_, animCols_, animFrameInterval_);
        SetupAnimator();
    }

    // Collider
    SetupCollider();

    LOG_INFO("[Individual] " + id_ + " initialized");
}

//----------------------------------------------------------------------------
void Individual::Shutdown()
{
    gameObject_.reset();
    transform_ = nullptr;
    sprite_ = nullptr;
    animator_ = nullptr;
    collider_ = nullptr;
    texture_.reset();
    ownerGroup_ = nullptr;
}

//----------------------------------------------------------------------------
void Individual::Update(float dt)
{
    if (!gameObject_ || !IsAlive()) return;

    // 実際の速度 = 目標速度 + 分離オフセット
    Vector2 actualVelocity = desiredVelocity_ + separationOffset_;

    // 位置更新
    if (transform_ && (actualVelocity.x != 0.0f || actualVelocity.y != 0.0f)) {
        Vector2 pos = transform_->GetPosition();
        pos.x += actualVelocity.x * dt;
        pos.y += actualVelocity.y * dt;
        transform_->SetPosition(pos);
    }

    // GameObjectの更新（Animatorの更新など）
    gameObject_->Update(dt);
}

//----------------------------------------------------------------------------
void Individual::Render(SpriteBatch& spriteBatch)
{
    if (!transform_ || !sprite_ || !IsAlive()) return;

    if (animator_) {
        spriteBatch.Draw(*sprite_, *transform_, *animator_);
    } else {
        spriteBatch.Draw(*sprite_, *transform_);
    }
}

//----------------------------------------------------------------------------
void Individual::AttackPlayer(Player* target)
{
    // デフォルト実装: 直接ダメージ（近接攻撃）
    if (!target || !target->IsAlive()) return;
    if (!IsAlive()) return;

    target->TakeDamage(attackDamage_);
    LOG_INFO("[Individual] " + id_ + " attacks Player");
}

//----------------------------------------------------------------------------
void Individual::TakeDamage(float damage)
{
    if (!IsAlive()) return;

    hp_ -= damage;
    if (hp_ < 0.0f) {
        hp_ = 0.0f;
    }

    if (hp_ <= 0.0f) {
        action_ = IndividualAction::Death;
        LOG_INFO("[Individual] " + id_ + " died");
        // TODO: OnIndividualDiedイベント発行
    }
}

//----------------------------------------------------------------------------
Vector2 Individual::GetPosition() const
{
    if (transform_) {
        return transform_->GetPosition();
    }
    return Vector2::Zero;
}

//----------------------------------------------------------------------------
void Individual::SetPosition(const Vector2& position)
{
    if (transform_) {
        transform_->SetPosition(position);
    }
}

//----------------------------------------------------------------------------
void Individual::SetupAnimator()
{
    // デフォルト実装: 各行1フレーム
    // 派生クラスでオーバーライドして具体的なアニメーション設定
}

//----------------------------------------------------------------------------
void Individual::SetupCollider()
{
    if (!gameObject_) return;

    collider_ = gameObject_->AddComponent<Collider2D>(Vector2(32, 32));
    collider_->SetLayer(0x04);  // Individual用レイヤー
    collider_->SetMask(0x04);   // 他のIndividualと衝突

    // 衝突コールバック（デバッグ用）
    collider_->SetOnCollisionEnter([this](Collider2D* /*self*/, Collider2D* /*other*/) {
        // LOG_INFO("[Individual] " + id_ + " collision enter");
    });
}

//----------------------------------------------------------------------------
void Individual::CalculateSeparation(const std::vector<Individual*>& others)
{
    separationOffset_ = Vector2::Zero;

    if (!IsAlive()) return;

    Vector2 myPos = GetPosition();

    for (Individual* other : others) {
        if (!other || other == this || !other->IsAlive()) continue;

        Vector2 otherPos = other->GetPosition();
        Vector2 diff = Vector2(myPos.x - otherPos.x, myPos.y - otherPos.y);
        float distance = diff.Length();

        // 距離が分離半径内かつ0より大きい場合
        if (distance < separationRadius_ && distance > 0.001f) {
            // 離れる方向に力を加える
            diff.Normalize();
            float strength = (separationRadius_ - distance) / separationRadius_;
            separationOffset_.x += diff.x * strength * separationForce_;
            separationOffset_.y += diff.y * strength * separationForce_;
        }
    }
}
