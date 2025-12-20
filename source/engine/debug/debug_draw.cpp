//----------------------------------------------------------------------------
//! @file   debug_draw.cpp
//! @brief  デバッグ描画ユーティリティ実装（Debugビルドのみ）
//----------------------------------------------------------------------------
#include "debug_draw.h"

#ifdef _DEBUG

#include "engine/c_systems/sprite_batch.h"
#include "engine/texture/texture_manager.h"
#include <cmath>

//----------------------------------------------------------------------------
DebugDraw& DebugDraw::Get()
{
    static DebugDraw instance;
    return instance;
}

//----------------------------------------------------------------------------
void DebugDraw::EnsureInitialized()
{
    if (initialized_) return;

    // 1x1の白テクスチャを作成
    uint32_t whitePixel = 0xFFFFFFFF;
    whiteTexture_ = TextureManager::Get().Create2D(
        1, 1,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D11_BIND_SHADER_RESOURCE,
        &whitePixel,
        sizeof(uint32_t)
    );

    initialized_ = true;
}

//----------------------------------------------------------------------------
void DebugDraw::DrawRectOutline(
    const Vector2& center,
    const Vector2& size,
    const Color& color,
    float lineWidth)
{
    float left = center.x - size.x * 0.5f;
    float top = center.y - size.y * 0.5f;

    DrawRectOutlineTopLeft(Vector2(left, top), size, color, lineWidth);
}

//----------------------------------------------------------------------------
void DebugDraw::DrawRectOutlineTopLeft(
    const Vector2& topLeft,
    const Vector2& size,
    const Color& color,
    float lineWidth)
{
    EnsureInitialized();
    if (!whiteTexture_) return;

    SpriteBatch& batch = SpriteBatch::Get();
    Texture* tex = whiteTexture_.get();

    float left = topLeft.x;
    float top = topLeft.y;
    float right = left + size.x;
    float bottom = top + size.y;

    // 上辺
    batch.Draw(tex, Vector2(left, top), color, 0.0f,
        Vector2(0, 0), Vector2(size.x, lineWidth), false, false, 100, 0);
    // 下辺
    batch.Draw(tex, Vector2(left, bottom - lineWidth), color, 0.0f,
        Vector2(0, 0), Vector2(size.x, lineWidth), false, false, 100, 0);
    // 左辺
    batch.Draw(tex, Vector2(left, top), color, 0.0f,
        Vector2(0, 0), Vector2(lineWidth, size.y), false, false, 100, 0);
    // 右辺
    batch.Draw(tex, Vector2(right - lineWidth, top), color, 0.0f,
        Vector2(0, 0), Vector2(lineWidth, size.y), false, false, 100, 0);
}

//----------------------------------------------------------------------------
void DebugDraw::DrawRectFilled(
    const Vector2& center,
    const Vector2& size,
    const Color& color)
{
    EnsureInitialized();
    if (!whiteTexture_) return;

    SpriteBatch& batch = SpriteBatch::Get();

    float left = center.x - size.x * 0.5f;
    float top = center.y - size.y * 0.5f;

    batch.Draw(whiteTexture_.get(), Vector2(left, top), color, 0.0f,
        Vector2(0, 0), size, false, false, 100, 0);
}

//----------------------------------------------------------------------------
void DebugDraw::DrawLine(
    const Vector2& start,
    const Vector2& end,
    const Color& color,
    float lineWidth)
{
    EnsureInitialized();
    if (!whiteTexture_) return;

    SpriteBatch& batch = SpriteBatch::Get();

    Vector2 diff = end - start;
    float length = diff.Length();
    if (length < 0.001f) return;

    // 角度計算
    float angle = std::atan2(diff.y, diff.x);

    // 線の中心位置
    Vector2 center = (start + end) * 0.5f;

    // 回転付きで描画
    batch.Draw(whiteTexture_.get(), center, color, angle,
        Vector2(0.5f, 0.5f), Vector2(length, lineWidth), false, false, 100, 0);
}

#endif // _DEBUG
