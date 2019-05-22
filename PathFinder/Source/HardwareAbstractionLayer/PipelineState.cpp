#include "PipelineState.hpp"
#include "Utils.h"

#include "../Foundation/Visitor.hpp"

#include <d3d12.h>

namespace HAL
{

    PipelineState::PipelineState(const RootSignature& assisiatedRootSignature)
        : mAssosiatedRootSignature(assisiatedRootSignature){}

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
        const InputAssemblerLayout& inputLayout,
        const RenderTargetFormatMap& renderTargetFormats,
        ResourceFormat::DepthStencil depthStencilFormat,
        PrimitiveTopology primitiveTopology)
        :
        PipelineState(rootSignature)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

        desc.pRootSignature = rootSignature.D3DSignature();
        desc.VS = vertexShader.D3DBytecode();
        desc.PS = pixelShader.D3DBytecode();

        if (domainShader) desc.DS = domainShader->D3DBytecode();
        if (hullShader) desc.HS = hullShader->D3DBytecode();
        if (geometryShader) desc.GS = geometryShader->D3DBytecode();

        desc.BlendState = blendState.D3DState();
        desc.RasterizerState = rasterizerState.D3DState();
        desc.DepthStencilState = depthStencilState.D3DState();
        desc.InputLayout = inputLayout.D3DLayout();
        desc.PrimitiveTopologyType = D3DPrimitiveTopologyType(primitiveTopology);
        desc.NumRenderTargets = renderTargetFormats.size();
        desc.DSVFormat = ResourceFormat::D3DFormat(depthStencilFormat);
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        for (auto& keyValue : renderTargetFormats)
        {
            auto rtIdx = std::underlying_type<RenderTarget>::type(keyValue.first);
            std::visit([&desc, rtIdx](auto&& format) { 
                desc.RTVFormats[rtIdx] = ResourceFormat::D3DFormat(format);
            }, keyValue.second);
        }

        //desc.SampleDesc
        //desc.IBStripCutValue;
        //desc.SampleMask;
        //desc.StreamOutput;
        /*desc.NodeMask;
        desc.CachedPSO;*/

#if defined(DEBUG) || defined(_DEBUG) 
        desc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
        
        ThrowIfFailed(device.D3DPtr()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mState)));
    }

}
