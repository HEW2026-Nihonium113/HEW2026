//----------------------------------------------------------------------------
//! @file   application.inl
//! @brief  アプリケーションテンプレート実装
//----------------------------------------------------------------------------
#pragma once

#include "renderer.h"
#include "common/logging/logging.h"
#include <thread>

//----------------------------------------------------------------------------
template<typename TGame>
void Application::Run(TGame& game)
{
    if (running_) {
        LOG_WARN("[Application] 既に実行中です");
        return;
    }
    running_ = true;
    shouldQuit_ = false;

    MainLoop(game);

    running_ = false;
}

//----------------------------------------------------------------------------
template<typename TGame>
void Application::MainLoop(TGame& game)
{
    Renderer& renderer = Renderer::Get();

    // フレームレート制限（60FPS = 16.67ms per frame）
    constexpr auto kTargetFrameTime = std::chrono::microseconds(16667);

    while (!shouldQuit_) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // メッセージ処理
        if (!window_->ProcessMessages()) {
            break;  // WM_QUIT
        }

        if (window_->ShouldClose()) {
            break;
        }

        // 最小化中はスリープ
        if (window_->IsMinimized()) {
            Sleep(10);
            continue;
        }

        // 時間更新
        UpdateTime();

        // 入力処理
        ProcessInput();

        // 通常更新
        game.Update();

        // 描画
        game.Render();

        // Present
        renderer.Present();

        // フレーム末処理
        game.EndFrame();

        ++frameCount_;

        // フレームレート制限：目標時間に達するまで待機
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto elapsed = frameEnd - frameStart;
        if (elapsed < kTargetFrameTime) {
            auto sleepTime = kTargetFrameTime - elapsed;
            std::this_thread::sleep_for(sleepTime);
        }
    }
}
