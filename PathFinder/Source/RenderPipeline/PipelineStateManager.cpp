#include "PipelineStateManager.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateManager::PipelineStateManager(HAL::Device* device, ShaderManager* shaderManager, const RenderSurfaceDescription& defaultRenderSurface)
        : mDevice{ device }, 
        mShaderManager{ shaderManager },
        mDefaultRenderSurfaceDesc{ defaultRenderSurface }, 
        mBaseRootSignature{ device },
        mDefaultGraphicsState{ device }
    {
        ConfigureDefaultStates();
        BuildBaseRootSignature();
    }

    void PipelineStateManager::StoreRootSignature(RootSignatureName name, HAL::RootSignature&& signature)
    {
        mRootSignatures.emplace(name, std::move(signature));
    }

    void PipelineStateManager::CreateGraphicsState(PSOName name, const GraphicsStateConfigurator& configurator)
    {
        assert_format(GetGraphicsPipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        GraphicsStateProxy proxy{};

        const HAL::GraphicsPipelineState* defaultGraphicsState = &DefaultGraphicsState();

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

        HAL::GraphicsPipelineState newState = DefaultGraphicsState().Clone();

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

        newState.SetRootSignature(GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetShaders(mShaderManager->LoadShaders(proxy.ShaderFileNames));

        newState.SetDebugName(name.ToString());

        mGraphicPSOs.emplace(name, std::move(newState));
    }

    void PipelineStateManager::CreateComputeState(PSOName name, const ComputeStateConfigurator& configurator)
    {
        assert_format(GetComputePipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        ComputeStateProxy proxy{};
        configurator(proxy);

        HAL::ComputePipelineState newState{ mDevice };

        newState.SetRootSignature(GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetShaders(mShaderManager->LoadShaders(proxy.ShaderFileNames));

        newState.SetDebugName(name.ToString());

        mComputePSOs.emplace(name, std::move(newState));
    }

    void PipelineStateManager::CreateRayTracingState(PSOName name, const RayTracingStateConfigurator& configurator)
    {
        assert_format(GetRayTracingPipelineState(name) == nullptr, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        RayTracingStateProxy proxy{};
        configurator(proxy);

        HAL::RayTracingPipelineState newState{ mDevice };

        for (const RayTracingStateProxy::ShaderInfo& shaderInfo : proxy.ShaderInfos())
        {
            HAL::RayTracingShaderBundle rtShaders = mShaderManager->LoadShaders(shaderInfo.ShaderFileNames);
            const HAL::RootSignature* localRootSig = GetNamedRootSignatureOrNull(shaderInfo.LocalRootSignatureName);

            newState.AddShaders(rtShaders, shaderInfo.Config, localRootSig);
        }

        newState.SetConfig(proxy.PipelineConfig);
        newState.SetGlobalRootSignature(GetNamedRootSignatureOrDefault(proxy.GlobalRootSignatureName));

        newState.SetDebugName(name.ToString());

        mRayTracingPSOs.emplace(name, std::move(newState));
    }

    const HAL::RootSignature* PipelineStateManager::GetRootSignature(RootSignatureName name) const
    {
        auto it = mRootSignatures.find(name);
        if (it == mRootSignatures.end()) return nullptr;
        return &it->second;
    }

    const HAL::RootSignature* PipelineStateManager::GetNamedRootSignatureOrDefault(std::optional<RootSignatureName> name) const
    {
        if (!name) return &mBaseRootSignature;

        const HAL::RootSignature* signature = GetRootSignature(*name);
        assert_format(signature, "Root signature ", name->ToString(), " doesn't exist");
        return signature;
    }

    const HAL::RootSignature* PipelineStateManager::GetNamedRootSignatureOrNull(std::optional<RootSignatureName> name) const
    {
        if (!name) return nullptr;

        const HAL::RootSignature* signature = GetRootSignature(*name);
        assert_format(signature, "Root signature ", name->ToString(), " doesn't exist");
        return signature;
    }

    const HAL::GraphicsPipelineState* PipelineStateManager::GetGraphicsPipelineState(PSOName name) const
    {
        auto it = mGraphicPSOs.find(name);
        if (it == mGraphicPSOs.end()) return nullptr;
        return &it->second;
    }

    const HAL::ComputePipelineState* PipelineStateManager::GetComputePipelineState(PSOName name) const
    {
        auto it = mComputePSOs.find(name);
        if (it == mComputePSOs.end()) return nullptr;
        return &it->second;
    }

    const HAL::RayTracingPipelineState* PipelineStateManager::GetRayTracingPipelineState(PSOName name) const
    {
        auto it = mRayTracingPSOs.find(name);
        if (it == mRayTracingPSOs.end()) return nullptr;
        return &it->second;
    }

    const HAL::RootSignature& PipelineStateManager::BaseRootSignature() const
    {
        return mBaseRootSignature;
    }

    const HAL::GraphicsPipelineState& PipelineStateManager::DefaultGraphicsState() const
    {
        return mDefaultGraphicsState;
    }

    void PipelineStateManager::CompileStates()
    {
        mBaseRootSignature.Compile();

        for (auto& nameSigPair : mRootSignatures)
        {
            HAL::RootSignature& signature = nameSigPair.second;
            signature.Compile();
        }

        for (auto& nameStatePair : mGraphicPSOs)
        {
            HAL::GraphicsPipelineState& pso = nameStatePair.second;
            pso.Compile();
        }

        for (auto& nameStatePair : mComputePSOs)
        {
            HAL::ComputePipelineState& pso = nameStatePair.second;
            pso.Compile();
        }

        for (auto& nameStatePair : mRayTracingPSOs)
        {
            HAL::RayTracingPipelineState& pso = nameStatePair.second;
            pso.Compile();
        }
    }

    void PipelineStateManager::ConfigureDefaultStates()
    {
        mDefaultGraphicsState.GetBlendState().SetBlendingEnabled(false);
        mDefaultGraphicsState.GetDepthStencilState().SetDepthTestEnabled(true);
        mDefaultGraphicsState.GetDepthStencilState().SetDepthWriteEnabled(true);
        mDefaultGraphicsState.SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList);
        mDefaultGraphicsState.GetRasterizerState().SetCullMode(HAL::RasterizerState::CullMode::Back);
        mDefaultGraphicsState.GetRasterizerState().SetFillMode(HAL::RasterizerState::FillMode::Solid);
        mDefaultGraphicsState.SetDepthStencilFormat(mDefaultRenderSurfaceDesc.DepthStencilFormat());
        mDefaultGraphicsState.SetRootSignature(&mBaseRootSignature);
    }

    void PipelineStateManager::BuildBaseRootSignature()
    {
        // See BaseRootSignature.hlsl for reference

        // Global data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 0, 10 }); 

        // Frame-specific data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 1, 10 });

        // Pass-specific data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 2, 10 });

        // Unbounded Texture2D range
        HAL::RootDescriptorTableParameter textures2D;
        textures2D.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 10 });
        mBaseRootSignature.AddDescriptorTableParameter(textures2D);

        // Unbounded Texture2D<uint4> range
        HAL::RootDescriptorTableParameter textures2DUInt4;
        textures2DUInt4.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 11 });
        mBaseRootSignature.AddDescriptorTableParameter(textures2DUInt4);

        // Unbounded Texture3D range
        HAL::RootDescriptorTableParameter textures3D;
        textures3D.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 12 }); 
        mBaseRootSignature.AddDescriptorTableParameter(textures3D);

        // Unbounded Texture2DArray range
        HAL::RootDescriptorTableParameter texture2DArrays;
        texture2DArrays.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 13 });
        mBaseRootSignature.AddDescriptorTableParameter(texture2DArrays);

        // Unbounded RWTexture2D range
        HAL::RootDescriptorTableParameter RWTextures2DFloat4;
        RWTextures2DFloat4.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 10 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures2DFloat4);

        // Unbounded RWTexture2D<uint4> range
        HAL::RootDescriptorTableParameter RWTextures2DUInt4;
        RWTextures2DUInt4.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 11 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures2DUInt4);

        // Unbounded RWTexture3D range
        HAL::RootDescriptorTableParameter RWTextures3D;
        RWTextures3D.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 12 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures3D);

        // Unbounded RWTexture2DArray range
        HAL::RootDescriptorTableParameter RWTexture2DArrays;
        RWTexture2DArrays.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 13 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTexture2DArrays);

        mBaseRootSignature.AddStaticSampler(HAL::StaticSampler::AnisotropicClamp(0));
        mBaseRootSignature.AddStaticSampler(HAL::StaticSampler::LinearClamp(1));
        mBaseRootSignature.AddStaticSampler(HAL::StaticSampler::PointClamp(2));
    }

}