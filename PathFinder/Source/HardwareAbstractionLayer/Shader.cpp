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
        case PipelineStage::Vertex:		entryPoint = "VSMain"; featureLevel = "vs_5_0"; macro.Name = "VSEntryPoint"; break;
        case PipelineStage::Hull:		entryPoint = "HSMain"; featureLevel = "HS_5_0"; macro.Name = "HSEntryPoint"; break;
        case PipelineStage::Domain:		entryPoint = "DSMain"; featureLevel = "DS_5_0"; macro.Name = "DSEntryPoint"; break;
        case PipelineStage::Geometry:	entryPoint = "GSMain"; featureLevel = "GS_5_0"; macro.Name = "GSEntryPoint"; break;
        case PipelineStage::Pixel:		entryPoint = "PSMain"; featureLevel = "PS_5_0"; macro.Name = "PSEntryPoint"; break;
        case PipelineStage::Compute:	entryPoint = "CSMain"; featureLevel = "CS_5_0"; macro.Name = "CSEntryPoint"; break;
        }

        uint32_t compilerFlags = 0; 

#if defined(DEBUG) || defined(_DEBUG)   
        compilerFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        ThrowIfFailed(D3DCompileFromFile(filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), featureLevel.c_str(), compilerFlags, 0, &mBlob, &errors));

        mBytecode = mBlob->GetBufferPointer();
        mBytecodeSize = mBlob->GetBufferSize();
    }

}

