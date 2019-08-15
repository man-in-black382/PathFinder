#include "Shader.hpp"
#include "Utils.h"

#include <d3dcompiler.h>

namespace HAL
{

    Shader::Shader(const std::wstring& filePath, PipelineStage pipelineStage)
    {
        std::string entryPoint;
        std::string featureLevel;
        D3D_SHADER_MACRO macro{};
        macro.Definition = "";

        switch (pipelineStage) {
        case PipelineStage::Vertex:		entryPoint = "VSMain"; featureLevel = "vs_5_1"; macro.Name = "VSEntryPoint"; break;
        case PipelineStage::Hull:		entryPoint = "HSMain"; featureLevel = "hs_5_1"; macro.Name = "HSEntryPoint"; break;
        case PipelineStage::Domain:		entryPoint = "DSMain"; featureLevel = "ds_5_1"; macro.Name = "DSEntryPoint"; break;
        case PipelineStage::Geometry:	entryPoint = "GSMain"; featureLevel = "gs_5_1"; macro.Name = "GSEntryPoint"; break;
        case PipelineStage::Pixel:		entryPoint = "PSMain"; featureLevel = "ps_5_1"; macro.Name = "PSEntryPoint"; break;
        case PipelineStage::Compute:	entryPoint = "CSMain"; featureLevel = "cs_5_1"; macro.Name = "CSEntryPoint"; break;
        }

        uint32_t compilerFlags = 0;  

#if defined(DEBUG) || defined(_DEBUG)    
        compilerFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
         
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        ThrowIfFailed(D3DCompileFromFile(filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), featureLevel.c_str(), compilerFlags, 0, &mBlob, &errors));
        if (errors) OutputDebugStringA((char*)errors->GetBufferPointer());
         
        mBytecode = mBlob->GetBufferPointer();
        mBytecodeSize = mBlob->GetBufferSize();
    }

    Shader::Shader(const std::string& filePath, PipelineStage pipelineStage)
        : Shader(std::wstring(filePath.begin(), filePath.end()), pipelineStage) {}

    ShaderBundle::ShaderBundle(Shader* vs, Shader* ps, Shader* ds, Shader* hs, Shader* gs, Shader* cs)
        : mVertexShader{ vs }, mPixelShader{ ps }, mDomainShader{ ds }, mHullShader{ hs }, mGeometryShader{ gs }, mComputeShader{ cs } {}

}

