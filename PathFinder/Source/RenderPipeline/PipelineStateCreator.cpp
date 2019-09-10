#include "PipelineStateCreator.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateCreator::PipelineStateCreator(PipelineStateManager* manager)
        : mPipelineStateManager{ manager } {}

    HAL::RootSignature PipelineStateCreator::CloneBaseRootSignature()
    {
        return mPipelineStateManager->mBaseRootSignature.Clone();
    }

    void PipelineStateCreator::CreateGraphicsState(PSOName name, const std::function<void(GraphicsStateProxy& state)>& configurator)
    {
        assert_format(mPipelineStateManager->GetGraphicsPipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToSring(), " already exists.");

        GraphicsStateProxy proxy{};

        HAL::GraphicsPipelineState *defaultGraphicsState = &mPipelineStateManager->mDefaultGraphicsState;

        proxy.BlendState = defaultGraphicsState->GetBlendState();
        proxy.RasterizerState = defaultGraphicsState->GetRasterizerState();
        proxy.DepthStencilState = defaultGraphicsState->GetDepthStencilState();
        proxy.DepthStencilFormat = mPipelineStateManager->mDefaultRenderSurface.DepthStencilFormat();
        proxy.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;

        configurator(proxy);

        if (proxy.RenderTargetFormats.empty())
        {
            proxy.RenderTargetFormats.push_back(mPipelineStateManager->mDefaultRenderSurface.RenderTargetFormat());
        }
        
        HAL::GraphicsPipelineState newState = mPipelineStateManager->mDefaultGraphicsState.Clone();

        newState.SetBlendState(proxy.BlendState);
        newState.SetRasterizerState(proxy.RasterizerState);
        newState.SetDepthStencilState(proxy.DepthStencilState);
        newState.SetDepthStencilFormat(proxy.DepthStencilFormat);
        newState.SetPrimitiveTopology(proxy.PrimitiveTopology);
        newState.SetRenderTargetFormats(
            proxy.RenderTargetFormats.size() > 0 ? std::optional(proxy.RenderTargetFormats[0]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 1 ? std::optional(proxy.RenderTargetFormats[1]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 2 ? std::optional(proxy.RenderTargetFormats[2]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 3 ? std::optional(proxy.RenderTargetFormats[3]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 4 ? std::optional(proxy.RenderTargetFormats[4]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 5 ? std::optional(proxy.RenderTargetFormats[5]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 6 ? std::optional(proxy.RenderTargetFormats[6]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 7 ? std::optional(proxy.RenderTargetFormats[7]) : std::nullopt
        );

        newState.SetRootSignature(mPipelineStateManager->GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetShaders(mPipelineStateManager->mShaderManager->LoadShaders(proxy.ShaderFileNames));

        mPipelineStateManager->mGraphicPSOs.emplace(name, std::move(newState));
    }

    void PipelineStateCreator::CreateComputeState(PSOName name, const std::function<void(ComputeStateProxy& state)>& configurator)
    {
        assert_format(mPipelineStateManager->GetComputePipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToSring(), " already exists.");

        ComputeStateProxy proxy{};
        configurator(proxy);

        HAL::ComputePipelineState newState{ mPipelineStateManager->mDevice };

        newState.SetRootSignature(mPipelineStateManager->GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetShaders(mPipelineStateManager->mShaderManager->LoadShaders(proxy.ShaderFileNames));

        mPipelineStateManager->mComputePSOs.emplace(name, std::move(newState));
    }

    void PipelineStateCreator::CreateRayTracingState(PSOName name, const std::function<void(RayTracingStateProxy& state)>& configurator)
    {
        assert_format(mPipelineStateManager->GetRayTracingPipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToSring(), " already exists.");

        RayTracingStateProxy proxy{};
        configurator(proxy);

        HAL::RayTracingPipelineState newState{ mPipelineStateManager->mDevice };

        for (const RayTracingStateProxy::ShaderInfo& shaderInfo : proxy.ShaderInfos())
        {
            HAL::RayTracingShaderBundle rtShaders = mPipelineStateManager->mShaderManager->LoadShaders(shaderInfo.ShaderFileNames);
            const HAL::RootSignature* localRootSig = mPipelineStateManager->GetNamedRootSignatureOrNull(shaderInfo.LocalRootSignatureName);

            newState.AddShaders(rtShaders, shaderInfo.Config, localRootSig);
        }

        newState.SetConfig(proxy.PipelineConfig);
        newState.SetGlobalRootSignature(mPipelineStateManager->GetNamedRootSignatureOrDefault(proxy.GlobalRootSignatureName));

        mPipelineStateManager->mRayTracingPSOs.emplace(name, std::move(newState));
    }

}