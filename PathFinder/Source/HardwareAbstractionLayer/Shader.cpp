#include "Shader.hpp"
#include "Utils.h"

#include <d3dcompiler.h>
#include <dxcapi.h>

namespace HAL
{

    Shader::Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const std::wstring& entryPoint, Stage stage)
        : mBlob{ blob }, mEntryPoint{ entryPoint }, mStage{ stage } 
    {
        mExport.Flags = D3D12_EXPORT_FLAG_NONE;
        mExport.Name = mEntryPoint.c_str();
        mExport.ExportToRename = mExport.Name;

        mDXILLibrary.DXILLibrary = D3DBytecode();
        mDXILLibrary.NumExports = 1;
        mDXILLibrary.pExports = &mExport;
    }

    ShaderBundle::ShaderBundle(Shader* vs, Shader* ps, Shader* ds, Shader* hs, Shader* gs, Shader* cs)
        : mVertexShader{ vs }, mPixelShader{ ps }, mDomainShader{ ds }, mHullShader{ hs }, mGeometryShader{ gs }, mComputeShader{ cs } {}

    RayTracingShaderBundle::RayTracingShaderBundle(Shader* rayGeneration, Shader* closestHit, Shader* anyHit, Shader* miss, Shader* intersection)
        : mRayGenerationShader{ rayGeneration }, mClosestHitShader{ closestHit }, mAnyHitShader{ anyHit }, mMissShader{ miss }, mIntersectionShader{ intersection } {}

}



