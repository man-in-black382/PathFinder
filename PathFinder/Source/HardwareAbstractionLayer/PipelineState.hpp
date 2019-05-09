#pragma once

#include "Device.hpp"
#include "RootSignature.hpp"
#include "Shader.hpp"
#include "RasterizerState.hpp"
#include "BlendState.hpp"
#include "DepthStencilState.hpp"
#include "PrimitiveTopology.hpp"
#include "ResourceFormat.hpp"
#include "InputAssemblerLayout.hpp"

#include <variant>

namespace HAL
{
	class PipelineState {
	public:
		virtual ~PipelineState() = 0;

	protected:
		Microsoft::WRL::ComPtr<ID3D12PipelineState> mState;

	private:
		inline const auto D3DState() const { return mState.Get(); }
	};

	class GraphicsPipelineState : public PipelineState
	{
	public:
		using RenderTargetFormat = std::variant<ResourceFormat::Color, ResourceFormat::TypelessColor>;

		GraphicsPipelineState(
			const Device& device,
			const RootSignature& rootSignature,
			const Shader& vertexShader,
			const Shader& pixelShader,
			const Shader* domainShader,
			const Shader* hullShader,
			const Shader* geometryShader,
			const BlendState& blendState,
			const RasterizerState& rasterizerState,
			const DepthStencilState& depthStencilState,
			PrimitiveTopology primitiveTopology,
			const std::array<RenderTargetFormat, 8>& renderTargetFormats,
			ResourceFormat::DepthStencil depthStencilFormat,
			const InputAssemblerLayout& inputLayout
		);

		~GraphicsPipelineState() = default;
	};

	class ComputePipelineState {
	private:
		D3D12_COMPUTE_PIPELINE_STATE_DESC mDesc;
	};

}

