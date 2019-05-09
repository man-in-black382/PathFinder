#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>

namespace HAL
{
	
	class Shader {
	public:
		enum class PipelineStage { Vertex, Hull, Domain, Geometry, Pixel, Compute };

		Shader(const std::wstring& filePath, PipelineStage pipelineStage);

	private:
		Microsoft::WRL::ComPtr<ID3DBlob> mBlob;
		void* mBytecode;
		uint64_t mBytecodeSize;

	public:
		inline auto D3DBytecode() const { return D3D12_SHADER_BYTECODE{ mBytecode, mBytecodeSize }; }
	};

}

