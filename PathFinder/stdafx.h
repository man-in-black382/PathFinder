#pragma once

#include <icecream/icecream.hpp>
#include <d3d12.h>
#include <Foundation/Assert.hpp>
#include <Foundation/Timer.hpp>

extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 4; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }