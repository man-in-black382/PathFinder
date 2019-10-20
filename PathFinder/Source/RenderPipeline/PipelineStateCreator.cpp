#include "PipelineStateCreator.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateCreator::PipelineStateCreator(PipelineStateManager* manager, const RenderSurfaceDescription& defaultRenderSurfaceDesc)
        : mPipelineStateManager{ manager }, mDefaultRenderSurfaceDesc{ defaultRenderSurfaceDesc } {}

    HAL::RootSignature PipelineStateCreator::CloneBaseRootSignature()
    {
        return mPipelineStateManager->BaseRootSignature().Clone();
    }

    void PipelineStateCreator::StoreRootSignature(RootSignatureName name, HAL::RootSignature&& signature)
    {
        signature.SetDebugName(name.ToString());
        mPipelineStateManager->StoreRootSignature(name, std::move(signature)); 
    }

    void PipelineStateCreator::CreateGraphicsState(PSOName name, const GraphicsStateConfigurator& configurator)
    {
        assert_format(mPipelineStateManager->GetGraphicsPipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        GraphicsStateProxy proxy{};

        const HAL::GraphicsPipelineState *defaultGraphicsState = &mPipelineStateManager->DefaultGraphicsState();

        proxy.BlendState = defaultGraphicsState->GetBlendState();
        proxy.RasterizerState = defaultGraphicsState->GetRasterizerState();
        proxy.DepthStencilState = defaultGraphicsState->GetDepthStencilState();
        proxy.DepthStencilFormat = mDefaultRenderSurfaceDesc.DepthStencilFormat();
        proxy.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;

        configurator(proxy);

        if (proxy.RenderTargetFormats.empty())
        {
            proxy.RenderTargetFormats.push_back(mDefaultRenderSurfaceDesc.RenderTargetFormat());
        }
        
        HAL::GraphicsPipelineState newState = mPipelineStateManager->DefaultGraphicsState().Clone();

        newState.SetBlendState(proxy.BlendState);
        newState.SetRasterizerState(proxy.RasterizerState);
        newState.SetDepthStencilState(proxy.DepthStencilState);
        newState.SetDepthStencilFormat(proxy.DepthStencilFormat);
        newState.SetPrimitiveTopology(proxy.PrimitiveTopology);
        newState.SetInputAssemblerLayout(proxy.InputLayout);

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
        newState.SetShaders(mPipelineStateManager->GetShaderManager()->LoadShaders(proxy.ShaderFileNames));

        newState.SetDebugName(name.ToString());

        mPipelineStateManager->StorePipelineState(name, std::move(newState));
    }

    void PipelineStateCreator::CreateComputeState(PSOName name, const ComputeStateConfigurator& configurator)
    {
        assert_format(mPipelineStateManager->GetComputePipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        ComputeStateProxy proxy{};
        configurator(proxy);

        HAL::ComputePipelineState newState = mPipelineStateManager->CreateComputeState();

        newState.SetRootSignature(mPipelineStateManager->GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetShaders(mPipelineStateManager->GetShaderManager()->LoadShaders(proxy.ShaderFileNames));

        newState.SetDebugName(name.ToString());

        mPipelineStateManager->StorePipelineState(name, std::move(newState));
    }

    void PipelineStateCreator::CreateRayTracingState(PSOName name, const RayTracingStateConfigurator& configurator)
    {
        assert_format(mPipelineStateManager->GetRayTracingPipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        RayTracingStateProxy proxy{};
        configurator(proxy);

        HAL::RayTracingPipelineState newState = mPipelineStateManager->CreateRayTracingState();

        for (const RayTracingStateProxy::ShaderInfo& shaderInfo : proxy.ShaderInfos())
        {
            HAL::RayTracingShaderBundle rtShaders = mPipelineStateManager->GetShaderManager()->LoadShaders(shaderInfo.ShaderFileNames);
            const HAL::RootSignature* localRootSig = mPipelineStateManager->GetNamedRootSignatureOrNull(shaderInfo.LocalRootSignatureName);

            newState.AddShaders(rtShaders, shaderInfo.Config, localRootSig);
        }

        newState.SetConfig(proxy.PipelineConfig);
        newState.SetGlobalRootSignature(mPipelineStateManager->GetNamedRootSignatureOrDefault(proxy.GlobalRootSignatureName));

        newState.SetDebugName(name.ToString());

        mPipelineStateManager->StorePipelineState(name, std::move(newState));
    }

}