//----------------------------------------------------------------------------
//! @file   test_sprite_batch.h
//! @brief  SpriteBatch system test declarations
//----------------------------------------------------------------------------
#pragma once

#include <string>

namespace tests {

//! Run all SpriteBatch system tests
//! @return true if all tests passed
//! @note Requires D3D11 device and shader resources
bool RunSpriteBatchTests(const std::wstring& assetsDir = L"");

} // namespace tests