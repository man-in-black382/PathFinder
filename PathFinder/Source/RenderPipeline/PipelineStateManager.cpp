#include "PipelineStateManager.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateManager::PipelineStateManager(HAL::Device* device, ShaderManager* shaderManager, const RenderSurfaceDescription& defaultRenderSurface)
        : mDevice{ device }, 
        mShaderManager{ shaderManager },
        mDefaultRenderSurface{ defaultRenderSurface }, 
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

    void PipelineStateManager::StorePipelineState(PSOName name, HAL::GraphicsPipelineState&& state)
    {
        mGraphicPSOs.emplace(name, std::move(state));
    }

    void PipelineStateManager::StorePipelineState(PSOName name, HAL::ComputePipelineState&& state)
    {
        mComputePSOs.emplace(name, std::move(state));
    }

    void PipelineStateManager::StorePipelineState(PSOName name, HAL::RayTracingPipelineState&& state)
    {
        mRayTracingPSOs.emplace(name, std::move(state));
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

    HAL::ComputePipelineState PipelineStateManager::CreateComputeState() const
    {
        return HAL::ComputePipelineState{ mDevice };
    }

    HAL::RayTracingPipelineState PipelineStateManager::CreateRayTracingState() const
    {
        return HAL::RayTracingPipelineState{ mDevice };
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
        mDefaultGraphicsState.SetDepthStencilFormat(mDefaultRenderSurface.DepthStencilFormat());
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