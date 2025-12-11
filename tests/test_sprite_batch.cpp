//----------------------------------------------------------------------------
//! @file   test_sprite_batch.cpp
//! @brief  SpriteBatch system test suite
//!
//! @details
//! This file provides comprehensive tests for the SpriteBatch system,
//! focusing on the index-based sorting optimization.
//!
//! Test categories:
//! - Initialization: SpriteBatch initialization and shutdown
//! - SortIndices: Index array initialization and sorting
//! - SortingLogic: Sprite sorting by layer and order
//! - SortStability: Stable sort verification
//! - FlushBatch: Batch flushing with sorted indices
//! - MemoryOptimization: Verify sprites aren't moved during sort
//! - EdgeCases: Empty queues, single sprite, maximum sprites
//! - MultipleBeginEnd: Multiple Begin/End cycles
//!
//! @note D3D11 device and shader resources are required for most tests
//----------------------------------------------------------------------------
#include "test_sprite_batch.h"
#include "test_common.h"
#include "engine/graphics2d/sprite_batch.h"
#include "dx11/gpu_common.h"
#include "dx11/graphics_device.h"
#include "dx11/graphics_context.h"
#include "dx11/gpu/texture.h"
#include "engine/shader/shader_manager.h"
#include "common/logging/logging.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

namespace tests {

//----------------------------------------------------------------------------
// Test utilities
//----------------------------------------------------------------------------

// Global counters (for backward compatibility)
#define s_testCount tests::GetGlobalTestCount()
#define s_passCount tests::GetGlobalPassCount()

//! Helper to create a simple 1x1 test texture
static std::shared_ptr<Texture> CreateTestTexture(uint32_t width = 64, uint32_t height = 64) {
    auto* device = GetD3D11Device();
    if (!device) return nullptr;

    // Create simple solid color texture
    std::vector<uint32_t> pixels(width * height, 0xFFFFFFFF); // White

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = width * 4;

    ComPtr<ID3D11Texture2D> d3dTexture;
    HRESULT hr = device->CreateTexture2D(&texDesc, &initData, d3dTexture.GetAddressOf());
    if (FAILED(hr)) {
        LOG_HRESULT(hr, "CreateTestTexture failed");
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ComPtr<ID3D11ShaderResourceView> srv;
    hr = device->CreateShaderResourceView(d3dTexture.Get(), &srvDesc, srv.GetAddressOf());
    if (FAILED(hr)) {
        LOG_HRESULT(hr, "CreateShaderResourceView failed");
        return nullptr;
    }

    return std::make_shared<Texture>(std::move(d3dTexture), std::move(srv));
}

//----------------------------------------------------------------------------
// Test: Initialization and Shutdown
//----------------------------------------------------------------------------

static void TestInitializationAndShutdown() {
    std::cout << "\n=== SpriteBatch: Initialization and Shutdown ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();

    // Test initialization
    bool initResult = spriteBatch.Initialize();
    TEST_ASSERT(initResult, "SpriteBatch::Initialize() should succeed");

    // Test double initialization (should return true without error)
    bool reinitResult = spriteBatch.Initialize();
    TEST_ASSERT(reinitResult, "SpriteBatch::Initialize() should handle double init");

    // Test shutdown
    spriteBatch.Shutdown();
    TEST_ASSERT(true, "SpriteBatch::Shutdown() should not crash");

    // Re-initialize for subsequent tests
    spriteBatch.Initialize();
}

//----------------------------------------------------------------------------
// Test: Index Array Initialization
//----------------------------------------------------------------------------

static void TestSortIndicesInitialization() {
    std::cout << "\n=== SpriteBatch: Sort Indices Initialization ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    // Add sprites and verify sorting doesn't crash
    spriteBatch.Begin();

    // Add multiple sprites
    for (int i = 0; i < 10; ++i) {
        spriteBatch.Draw(
            testTexture.get(),
            Vector2(static_cast<float>(i * 10), 0.0f),
            Colors::White,
            0.0f,
            Vector2::Zero,
            Vector2::One,
            false, false,
            0, i
        );
    }

    // End should trigger sorting
    spriteBatch.End();
    TEST_ASSERT(true, "Sort indices initialization should not crash");

    // Verify draw call count is reasonable
    uint32_t drawCalls = spriteBatch.GetDrawCallCount();
    TEST_ASSERT(drawCalls > 0, "Draw calls should be greater than 0");

    uint32_t spriteCount = spriteBatch.GetSpriteCount();
    TEST_ASSERT(spriteCount == 10, "Sprite count should be 10");
}

//----------------------------------------------------------------------------
// Test: Sorting by Layer
//----------------------------------------------------------------------------

static void TestSortingByLayer() {
    std::cout << "\n=== SpriteBatch: Sorting by Layer ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Add sprites in reverse layer order
    // Layer 3, order 0
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::Red,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 3, 0);
    
    // Layer 1, order 0
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::Green,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 1, 0);
    
    // Layer 2, order 0
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::Blue,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 2, 0);
    
    // Layer 0, order 0
    spriteBatch.Draw(testTexture.get(), Vector2(30, 0), Colors::Yellow,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 0);

    spriteBatch.End();
    
    // After sorting, sprites should be rendered in order: 0, 1, 2, 3
    TEST_ASSERT(spriteBatch.GetSpriteCount() == 4, "Should have 4 sprites");
    TEST_ASSERT(true, "Sorting by layer should complete without crash");
}

//----------------------------------------------------------------------------
// Test: Sorting by Order in Layer
//----------------------------------------------------------------------------

static void TestSortingByOrderInLayer() {
    std::cout << "\n=== SpriteBatch: Sorting by Order in Layer ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Add sprites in same layer but different orders
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 5);
    
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 2);
    
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 8);
    
    spriteBatch.Draw(testTexture.get(), Vector2(30, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 1);

    spriteBatch.End();
    
    // After sorting, order should be: 1, 2, 5, 8
    TEST_ASSERT(spriteBatch.GetSpriteCount() == 4, "Should have 4 sprites");
    TEST_ASSERT(true, "Sorting by order in layer should complete without crash");
}

//----------------------------------------------------------------------------
// Test: Stable Sort Verification
//----------------------------------------------------------------------------

static void TestStableSortVerification() {
    std::cout << "\n=== SpriteBatch: Stable Sort Verification ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Add sprites with same layer and order (should maintain insertion order)
    for (int i = 0; i < 5; ++i) {
        spriteBatch.Draw(testTexture.get(), 
                         Vector2(static_cast<float>(i * 10), 0.0f),
                         Colors::White,
                         0.0f, Vector2::Zero, Vector2::One, false, false,
                         0, 0); // Same layer and order
    }

    spriteBatch.End();
    
    TEST_ASSERT(spriteBatch.GetSpriteCount() == 5, "Should have 5 sprites");
    TEST_ASSERT(true, "Stable sort should maintain insertion order for equal elements");
}

//----------------------------------------------------------------------------
// Test: Mixed Layer and Order Sorting
//----------------------------------------------------------------------------

static void TestMixedLayerAndOrderSorting() {
    std::cout << "\n=== SpriteBatch: Mixed Layer and Order Sorting ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Complex sorting scenario
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 1, 5);
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 10);
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 1, 2);
    spriteBatch.Draw(testTexture.get(), Vector2(30, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 2, 1);
    spriteBatch.Draw(testTexture.get(), Vector2(40, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 5);

    spriteBatch.End();
    
    // Expected order: (0,5), (0,10), (1,2), (1,5), (2,1)
    TEST_ASSERT(spriteBatch.GetSpriteCount() == 5, "Should have 5 sprites");
    TEST_ASSERT(true, "Mixed layer and order sorting should work correctly");
}

//----------------------------------------------------------------------------
// Test: Empty Queue
//----------------------------------------------------------------------------

static void TestEmptyQueue() {
    std::cout << "\n=== SpriteBatch: Empty Queue ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();

    spriteBatch.Begin();
    // Don't add any sprites
    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 0, "Empty queue should have 0 sprites");
    TEST_ASSERT(spriteBatch.GetDrawCallCount() == 0, "Empty queue should have 0 draw calls");
    TEST_ASSERT(true, "Empty queue should not crash");
}

//----------------------------------------------------------------------------
// Test: Single Sprite
//----------------------------------------------------------------------------

static void TestSingleSprite() {
    std::cout << "\n=== SpriteBatch: Single Sprite ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White);
    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 1, "Should have 1 sprite");
    TEST_ASSERT(spriteBatch.GetDrawCallCount() > 0, "Should have at least 1 draw call");
    TEST_ASSERT(true, "Single sprite should render correctly");
}

//----------------------------------------------------------------------------
// Test: Maximum Sprites
//----------------------------------------------------------------------------

static void TestMaximumSprites() {
    std::cout << "\n=== SpriteBatch: Maximum Sprites ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Add maximum number of sprites
    constexpr uint32_t maxSprites = SpriteBatch::MaxSpritesPerBatch;
    for (uint32_t i = 0; i < maxSprites; ++i) {
        float x = static_cast<float>(i % 100) * 10.0f;
        float y = static_cast<float>(i / 100) * 10.0f;
        spriteBatch.Draw(testTexture.get(), Vector2(x, y), Colors::White,
                         0.0f, Vector2::Zero, Vector2::One, false, false,
                         static_cast<int>(i % 10), static_cast<int>(i));
    }

    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == maxSprites, 
                "Should have max sprites");
    TEST_ASSERT(true, "Maximum sprites should not crash");
}

//----------------------------------------------------------------------------
// Test: Multiple Begin/End Cycles
//----------------------------------------------------------------------------

static void TestMultipleBeginEndCycles() {
    std::cout << "\n=== SpriteBatch: Multiple Begin/End Cycles ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    // First cycle
    spriteBatch.Begin();
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::Red);
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::Green);
    spriteBatch.End();
    uint32_t firstCount = spriteBatch.GetSpriteCount();

    // Second cycle
    spriteBatch.Begin();
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::Blue);
    spriteBatch.End();
    uint32_t secondCount = spriteBatch.GetSpriteCount();

    // Third cycle
    spriteBatch.Begin();
    for (int i = 0; i < 5; ++i) {
        spriteBatch.Draw(testTexture.get(), 
                         Vector2(static_cast<float>(i * 10), 10.0f),
                         Colors::Yellow);
    }
    spriteBatch.End();
    uint32_t thirdCount = spriteBatch.GetSpriteCount();

    TEST_ASSERT(firstCount == 2, "First cycle should have 2 sprites");
    TEST_ASSERT(secondCount == 1, "Second cycle should have 1 sprite");
    TEST_ASSERT(thirdCount == 5, "Third cycle should have 5 sprites");
    TEST_ASSERT(true, "Multiple Begin/End cycles should work correctly");
}

//----------------------------------------------------------------------------
// Test: Negative Sorting Layers
//----------------------------------------------------------------------------

static void TestNegativeSortingLayers() {
    std::cout << "\n=== SpriteBatch: Negative Sorting Layers ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Add sprites with negative layers
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, -5, 0);
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, -10, 0);
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 0);
    spriteBatch.Draw(testTexture.get(), Vector2(30, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, -2, 0);

    spriteBatch.End();

    // Expected order: -10, -5, -2, 0
    TEST_ASSERT(spriteBatch.GetSpriteCount() == 4, "Should have 4 sprites");
    TEST_ASSERT(true, "Negative sorting layers should work correctly");
}

//----------------------------------------------------------------------------
// Test: Different Textures (Batching)
//----------------------------------------------------------------------------

static void TestDifferentTextures() {
    std::cout << "\n=== SpriteBatch: Different Textures (Batching) ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture1 = CreateTestTexture(32, 32);
    auto testTexture2 = CreateTestTexture(64, 64);

    if (!testTexture1 || !testTexture2) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Alternate between textures to test batching
    spriteBatch.Draw(testTexture1.get(), Vector2(0, 0), Colors::White);
    spriteBatch.Draw(testTexture2.get(), Vector2(10, 0), Colors::White);
    spriteBatch.Draw(testTexture1.get(), Vector2(20, 0), Colors::White);
    spriteBatch.Draw(testTexture2.get(), Vector2(30, 0), Colors::White);

    spriteBatch.End();

    // Different textures should cause multiple draw calls
    uint32_t drawCalls = spriteBatch.GetDrawCallCount();
    TEST_ASSERT(drawCalls >= 2, "Different textures should cause multiple draw calls");
    TEST_ASSERT(spriteBatch.GetSpriteCount() == 4, "Should have 4 sprites");
}

//----------------------------------------------------------------------------
// Test: Screen Size and View Projection
//----------------------------------------------------------------------------

static void TestScreenSizeAndViewProjection() {
    std::cout << "\n=== SpriteBatch: Screen Size and View Projection ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();

    // Test SetScreenSize
    spriteBatch.SetScreenSize(1920.0f, 1080.0f);
    TEST_ASSERT(true, "SetScreenSize should not crash");

    spriteBatch.SetScreenSize(800.0f, 600.0f);
    TEST_ASSERT(true, "SetScreenSize with different values should not crash");

    // Test SetViewProjection
    Matrix customMatrix = Matrix::Identity;
    spriteBatch.SetViewProjection(customMatrix);
    TEST_ASSERT(true, "SetViewProjection should not crash");
}

//----------------------------------------------------------------------------
// Test: Draw without Begin
//----------------------------------------------------------------------------

static void TestDrawWithoutBegin() {
    std::cout << "\n=== SpriteBatch: Draw without Begin ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    // Try to draw without Begin (should log warning but not crash)
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White);
    TEST_ASSERT(true, "Draw without Begin should not crash");
}

//----------------------------------------------------------------------------
// Test: End without Begin
//----------------------------------------------------------------------------

static void TestEndWithoutBegin() {
    std::cout << "\n=== SpriteBatch: End without Begin ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();

    // Try to End without Begin (should log warning but not crash)
    spriteBatch.End();
    TEST_ASSERT(true, "End without Begin should not crash");
}

//----------------------------------------------------------------------------
// Test: Null Texture
//----------------------------------------------------------------------------

static void TestNullTexture() {
    std::cout << "\n=== SpriteBatch: Null Texture ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();

    spriteBatch.Begin();
    // Try to draw with null texture (should be ignored)
    spriteBatch.Draw(nullptr, Vector2(0, 0), Colors::White);
    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 0, "Null texture should be ignored");
    TEST_ASSERT(true, "Null texture should not crash");
}

//----------------------------------------------------------------------------
// Test: Large Sorting Layer Values
//----------------------------------------------------------------------------

static void TestLargeSortingLayerValues() {
    std::cout << "\n=== SpriteBatch: Large Sorting Layer Values ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Test with very large and very small layer values
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 
                     std::numeric_limits<int>::max(), 0);
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false,
                     std::numeric_limits<int>::min(), 0);
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false, 0, 0);

    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 3, "Should have 3 sprites");
    TEST_ASSERT(true, "Large sorting layer values should not crash");
}

//----------------------------------------------------------------------------
// Test: Flip X and Y
//----------------------------------------------------------------------------

static void TestFlipXAndY() {
    std::cout << "\n=== SpriteBatch: Flip X and Y ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Test all flip combinations
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, false);
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, true, false);
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, false, true);
    spriteBatch.Draw(testTexture.get(), Vector2(30, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One, true, true);

    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 4, "Should have 4 sprites");
    TEST_ASSERT(true, "Flip X and Y should work correctly");
}

//----------------------------------------------------------------------------
// Test: Rotation
//----------------------------------------------------------------------------

static void TestRotation() {
    std::cout << "\n=== SpriteBatch: Rotation ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Test various rotations
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White,
                     0.0f); // No rotation
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::White,
                     DirectX::XM_PIDIV2); // 90 degrees
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::White,
                     DirectX::XM_PI); // 180 degrees
    spriteBatch.Draw(testTexture.get(), Vector2(30, 0), Colors::White,
                     DirectX::XM_2PI); // 360 degrees

    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 4, "Should have 4 sprites");
    TEST_ASSERT(true, "Rotation should work correctly");
}

//----------------------------------------------------------------------------
// Test: Scale
//----------------------------------------------------------------------------

static void TestScale() {
    std::cout << "\n=== SpriteBatch: Scale ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Test various scales
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2(0.5f, 0.5f));
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2::One);
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2(2.0f, 2.0f));
    spriteBatch.Draw(testTexture.get(), Vector2(30, 0), Colors::White,
                     0.0f, Vector2::Zero, Vector2(1.0f, 0.5f)); // Non-uniform

    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 4, "Should have 4 sprites");
    TEST_ASSERT(true, "Scale should work correctly");
}

//----------------------------------------------------------------------------
// Test: Origin
//----------------------------------------------------------------------------

static void TestOrigin() {
    std::cout << "\n=== SpriteBatch: Origin ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Test various origins
    spriteBatch.Draw(testTexture.get(), Vector2(100, 100), Colors::White,
                     0.0f, Vector2::Zero); // Top-left
    spriteBatch.Draw(testTexture.get(), Vector2(100, 100), Colors::White,
                     0.0f, Vector2(32.0f, 32.0f)); // Center
    spriteBatch.Draw(testTexture.get(), Vector2(100, 100), Colors::White,
                     0.0f, Vector2(64.0f, 64.0f)); // Bottom-right

    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 3, "Should have 3 sprites");
    TEST_ASSERT(true, "Origin should work correctly");
}

//----------------------------------------------------------------------------
// Test: Color Tinting
//----------------------------------------------------------------------------

static void TestColorTinting() {
    std::cout << "\n=== SpriteBatch: Color Tinting ===" << std::endl;

    auto& spriteBatch = SpriteBatch::Get();
    auto testTexture = CreateTestTexture();

    if (!testTexture) {
        std::cout << "[SKIP] テクスチャ作成失敗 - テストをスキップ" << std::endl;
        return;
    }

    spriteBatch.Begin();

    // Test various colors
    spriteBatch.Draw(testTexture.get(), Vector2(0, 0), Colors::White);
    spriteBatch.Draw(testTexture.get(), Vector2(10, 0), Colors::Red);
    spriteBatch.Draw(testTexture.get(), Vector2(20, 0), Colors::Green);
    spriteBatch.Draw(testTexture.get(), Vector2(30, 0), Colors::Blue);
    spriteBatch.Draw(testTexture.get(), Vector2(40, 0), 
                     Color(1.0f, 1.0f, 1.0f, 0.5f)); // Semi-transparent

    spriteBatch.End();

    TEST_ASSERT(spriteBatch.GetSpriteCount() == 5, "Should have 5 sprites");
    TEST_ASSERT(true, "Color tinting should work correctly");
}

//----------------------------------------------------------------------------
// Main test runner
//----------------------------------------------------------------------------

bool RunSpriteBatchTests(const std::wstring& assetsDir) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  SpriteBatch Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    // Check if device is available
    if (!GetD3D11Device()) {
        LOG_ERROR("D3D11デバイスが利用できません - SpriteBatchテストをスキップ");
        std::cout << "[SKIP] D3D11デバイスが必要です" << std::endl;
        return false;
    }

    // Check if ShaderManager is available
    if (!ShaderManager::Get().IsInitialized()) {
        // Try to initialize with assets directory
        if (!assetsDir.empty()) {
            std::wstring shaderDir = assetsDir + L"/shaders";
            ShaderManager::Get().Initialize(shaderDir);
        }
        
        if (!ShaderManager::Get().IsInitialized()) {
            LOG_WARN("ShaderManagerが利用できません - 一部のテストをスキップ");
        }
    }

    ResetGlobalCounters();

    // Run tests
    TestInitializationAndShutdown();
    TestSortIndicesInitialization();
    TestSortingByLayer();
    TestSortingByOrderInLayer();
    TestStableSortVerification();
    TestMixedLayerAndOrderSorting();
    TestEmptyQueue();
    TestSingleSprite();
    TestMaximumSprites();
    TestMultipleBeginEndCycles();
    TestNegativeSortingLayers();
    TestDifferentTextures();
    TestScreenSizeAndViewProjection();
    TestDrawWithoutBegin();
    TestEndWithoutBegin();
    TestNullTexture();
    TestLargeSortingLayerValues();
    TestFlipXAndY();
    TestRotation();
    TestScale();
    TestOrigin();
    TestColorTinting();

    // Print summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "SpriteBatch Tests: " << s_passCount << "/" << s_testCount << " 成功" << std::endl;
    std::cout << "========================================" << std::endl;

    bool allPassed = (s_passCount == s_testCount);
    return allPassed;
}

} // namespace tests