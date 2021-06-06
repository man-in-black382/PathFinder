#pragma once

#ifndef PBRT_IS_WINDOWS
#define PBRT_IS_WINDOWS 1
#endif

#ifndef PBRT_HAS_INTRIN_H
#define PBRT_HAS_INTRIN_H 1
#endif

#ifndef PBRT_IS_MSVC
#define PBRT_IS_MSVC 1
#endif

#ifndef _AMD64_
#define _AMD64_ 1
#endif

#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
#endif

#ifndef GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_LEFT_HANDED 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <icecream/icecream.hpp>
#include <d3d12.h>
#include <Foundation/Assert.hpp>
#include <Foundation/Timer.hpp>

// For Microsoft's Agility SDK
extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 4; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }
