#include "Shader.hpp"
#include "Utils.h"

#include <d3dcompiler.h>
#include <dxcapi.h>

namespace HAL
{

    Shader::Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const std::wstring& entryPoint, Stage stage)
        : mBlob{ blob }, mEntryPoint{ entryPoint }, mStage{ stage } {}

    GraphicsShaderBundle::GraphicsShaderBundle(Shader* vs, Shader* ps, Shader* ds, Shader* hs, Shader* gs)
        : mVertexShader{ vs }, mPixelShader{ ps }, mDomainShader{ ds }, mHullShader{ hs }, mGeometryShader{ gs } {}
    
    ComputeShaderBundle::ComputeShaderBundle(Shader* cs)
        : mComputeShader{ cs } {}

    RayTracingShaderBundle::RayTracingShaderBundle(Shader* rayGeneration, Shader* closestHit, Shader* anyHit, Shader* miss, Shader* intersection)
        : mRayGenerationShader{ rayGeneration }, mClosestHitShader{ closestHit }, mAnyHitShader{ anyHit }, mMissShader{ miss }, mIntersectionShader{ intersection } {}

}



