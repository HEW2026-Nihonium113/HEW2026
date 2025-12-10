//----------------------------------------------------------------------------
//! @file   camera2d.h
//! @brief  2Dカメラコンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "component.h"
#include "game_object.h"
#include "transform2d.h"
#include "engine/scene/math_types.h"

//============================================================================
//! @brief 2Dカメラコンポーネント
//!
//! 2D空間でのビュー変換を管理する。
//! Transform2Dコンポーネントと連携し、位置・回転はTransform2Dから取得。
//! ズームとビューポートサイズはCamera2D固有の設定。
//!
//! @note 同じGameObjectにTransform2Dが必要
//============================================================================
class Camera2D : public Component {
public:
    Camera2D() = default;

    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param viewportWidth ビューポート幅
    //! @param viewportHeight ビューポート高さ
    //------------------------------------------------------------------------
    Camera2D(float viewportWidth, float viewportHeight)
        : viewportWidth_(viewportWidth), viewportHeight_(viewportHeight) {}

    //------------------------------------------------------------------------
    // Component オーバーライド
    //------------------------------------------------------------------------

    void OnAttach() override {
        transform_ = GetOwner()->GetComponent<Transform2D>();
    }

    //------------------------------------------------------------------------
    // 位置（Transform2Dに委譲）
    //------------------------------------------------------------------------

    [[nodiscard]] Vector2 GetPosition() const noexcept {
        return transform_ ? transform_->GetPosition() : Vector2::Zero;
    }

    void SetPosition(const Vector2& position) noexcept {
        if (transform_) transform_->SetPosition(position);
    }

    void SetPosition(float x, float y) noexcept {
        if (transform_) transform_->SetPosition(x, y);
    }

    void Translate(const Vector2& delta) noexcept {
        if (transform_) transform_->Translate(delta);
    }

    //------------------------------------------------------------------------
    // 回転（Transform2Dに委譲）
    //------------------------------------------------------------------------

    [[nodiscard]] float GetRotation() const noexcept {
        return transform_ ? transform_->GetRotation() : 0.0f;
    }

    [[nodiscard]] float GetRotationDegrees() const noexcept {
        return transform_ ? transform_->GetRotationDegrees() : 0.0f;
    }

    void SetRotation(float radians) noexcept {
        if (transform_) transform_->SetRotation(radians);
    }

    void SetRotationDegrees(float degrees) noexcept {
        if (transform_) transform_->SetRotationDegrees(degrees);
    }

    //------------------------------------------------------------------------
    // ズーム（Camera2D固有）
    //------------------------------------------------------------------------

    [[nodiscard]] float GetZoom() const noexcept { return zoom_; }

    void SetZoom(float zoom) noexcept {
        zoom_ = (zoom > 0.001f) ? zoom : 0.001f;
    }

    //------------------------------------------------------------------------
    // ビューポート
    //------------------------------------------------------------------------

    [[nodiscard]] float GetViewportWidth() const noexcept { return viewportWidth_; }
    [[nodiscard]] float GetViewportHeight() const noexcept { return viewportHeight_; }

    void SetViewportSize(float width, float height) noexcept {
        viewportWidth_ = width;
        viewportHeight_ = height;
    }

    //------------------------------------------------------------------------
    // 行列
    //------------------------------------------------------------------------

    //! @brief ビュー行列を取得
    [[nodiscard]] Matrix GetViewMatrix() const {
        return BuildViewMatrix();
    }

    //! @brief ビュープロジェクション行列を取得
    [[nodiscard]] Matrix GetViewProjectionMatrix() const {
        Matrix view = BuildViewMatrix();
        Matrix projection = Matrix::CreateOrthographicOffCenter(
            0.0f, viewportWidth_,
            viewportHeight_, 0.0f,
            0.0f, 1.0f
        );
        Matrix viewProj = view * projection;
        return viewProj.Transpose();  // シェーダー用に転置
    }

    //------------------------------------------------------------------------
    // 座標変換
    //------------------------------------------------------------------------

    //! @brief スクリーン座標をワールド座標に変換
    [[nodiscard]] Vector2 ScreenToWorld(const Vector2& screenPos) const {
        Matrix viewProj = GetViewProjectionMatrix();
        Matrix invViewProj = viewProj.Invert();

        float ndcX = (screenPos.x / viewportWidth_) * 2.0f - 1.0f;
        float ndcY = 1.0f - (screenPos.y / viewportHeight_) * 2.0f;

        Vector3 worldPos = Vector3::Transform(Vector3(ndcX, ndcY, 0.0f), invViewProj);
        return Vector2(worldPos.x, worldPos.y);
    }

    //! @brief ワールド座標をスクリーン座標に変換
    [[nodiscard]] Vector2 WorldToScreen(const Vector2& worldPos) const {
        Matrix viewProj = GetViewProjectionMatrix();
        Vector3 ndcPos = Vector3::Transform(Vector3(worldPos.x, worldPos.y, 0.0f), viewProj);

        float screenX = (ndcPos.x + 1.0f) * 0.5f * viewportWidth_;
        float screenY = (1.0f - ndcPos.y) * 0.5f * viewportHeight_;
        return Vector2(screenX, screenY);
    }

    //! @brief カメラが映す領域の境界を取得
    void GetWorldBounds(Vector2& outMin, Vector2& outMax) const {
        outMin = ScreenToWorld(Vector2::Zero);
        outMax = ScreenToWorld(Vector2(viewportWidth_, viewportHeight_));
    }

    //------------------------------------------------------------------------
    // ユーティリティ
    //------------------------------------------------------------------------

    //! @brief 指定位置を画面中央に映すようにカメラを移動
    void LookAt(const Vector2& target) {
        SetPosition(target);
    }

    //! @brief カメラを対象に追従（スムーズ）
    void Follow(const Vector2& target, float smoothing = 0.1f) {
        Vector2 pos = GetPosition();
        Vector2 diff = target - pos;
        Translate(diff * Clamp(smoothing, 0.0f, 1.0f));
    }

private:
    [[nodiscard]] Matrix BuildViewMatrix() const {
        Vector2 position = GetPosition();
        float rotation = GetRotation();

        float halfWidth = viewportWidth_ * 0.5f;
        float halfHeight = viewportHeight_ * 0.5f;

        Matrix translation = Matrix::CreateTranslation(-position.x, -position.y, 0.0f);
        Matrix rot = Matrix::CreateRotationZ(-rotation);
        Matrix scale = Matrix::CreateScale(zoom_, zoom_, 1.0f);
        Matrix centerOffset = Matrix::CreateTranslation(halfWidth, halfHeight, 0.0f);

        return translation * rot * scale * centerOffset;
    }

    Transform2D* transform_ = nullptr;  //!< 位置・回転の参照先
    float zoom_ = 1.0f;

    float viewportWidth_ = 1280.0f;
    float viewportHeight_ = 720.0f;
};
