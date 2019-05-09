#include "PipelineState.hpp"

#include <d3d12.h>

namespace HAL
{

	PipelineState::~PipelineState() {}

	GraphicsPipelineState::GraphicsPipelineState(
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
		const InputAssemblerLayout& inputLayout)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;

		desc.pRootSignature;
		desc.VS;
		desc.PS;
		desc.DS;
		desc.HS;
		desc.GS;
		desc.StreamOutput;
		desc.BlendState;
		//desc.SampleMask;
		desc.RasterizerState;
		desc.DepthStencilState;
		desc.InputLayout;
		desc.IBStripCutValue;
		desc.PrimitiveTopologyType;
		desc.NumRenderTargets;
		desc.RTVFormats[8];
		desc.DSVFormat;
		desc.SampleDesc;
		/*desc.NodeMask;
		desc.CachedPSO;*/

#if defined(DEBUG) || defined(_DEBUG) 
		desc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
		
		device.D3DPtr()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mState));
	}

}
