#include "Shader.hpp"
#include "Utils.h"

#include <d3dcompiler.h>

namespace HAL
{

	Shader::Shader(const std::wstring& filePath, PipelineStage pipelineStage)
	{
		std::string entryPoint;
		std::string featureLevel;

		switch (pipelineStage) {
		case PipelineStage::Vertex:		entryPoint = "VSMain"; featureLevel = "VS_5_0"; break;
		case PipelineStage::Hull:		entryPoint = "HSMain"; featureLevel = "HS_5_0"; break;
		case PipelineStage::Domain:		entryPoint = "DSMain"; featureLevel = "DS_5_0"; break;
		case PipelineStage::Geometry:	entryPoint = "GSMain"; featureLevel = "GS_5_0"; break;
		case PipelineStage::Pixel:		entryPoint = "PSMain"; featureLevel = "PS_5_0"; break;
		case PipelineStage::Compute:	entryPoint = "CSMain"; featureLevel = "CS_5_0"; break;
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

