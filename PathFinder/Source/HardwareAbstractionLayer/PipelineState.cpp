#include "PipelineState.hpp"
#include "Utils.h"

#include "../Foundation/Visitor.hpp"

#include <d3d12.h>

namespace HAL
{

    PipelineState::~PipelineState() {}

    void GraphicsPipelineState::Compile(const Device& device)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

        desc.pRootSignature = mRootSignature->D3DSignature();
        desc.VS = mVertexShader->D3DBytecode();
        desc.PS = mPixelShader->D3DBytecode();

        if (mDomainShader) desc.DS = mDomainShader->D3DBytecode();
        if (mHullShader) desc.HS = mHullShader->D3DBytecode();
        if (mGeometryShader) desc.GS = mGeometryShader->D3DBytecode();

        desc.BlendState = mBlendState.D3DState();
        desc.RasterizerState = mRasterizerState.D3DState();
        desc.DepthStencilState = mDepthStencilState.D3DState();
        desc.InputLayout = mInputLayout.D3DLayout();
        desc.PrimitiveTopologyType = D3DPrimitiveTopologyType(mPrimitiveTopology);
        desc.NumRenderTargets = mRenderTargetFormats.size();
        desc.DSVFormat = ResourceFormat::D3DFormat(mDepthStencilFormat);
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleMask = 0xFFFFFFFF;

        for (auto& keyValue : mRenderTargetFormats)
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

    GraphicsPipelineState GraphicsPipelineState::Clone() const
    {
        GraphicsPipelineState newState = *this;
        newState.mState = nullptr;
        return newState;
    }

}
