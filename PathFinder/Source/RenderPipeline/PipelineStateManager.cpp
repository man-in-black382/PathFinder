#include "PipelineStateManager.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateManager::PipelineStateManager(HAL::Device* device, ShaderManager* shaderManager, const RenderSurface& defaultRenderSurface)
        : mDevice{ device }, 
        mShaderManager{ shaderManager },
        mDefaultRenderSurface{ defaultRenderSurface }, 
        mBaseRootSignature{ device },
        mDefaultGraphicsState{ device }
    {
        ConfigureDefaultStates();
        BuildBaseRootSignature();
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
        assert_format(signature, "Root signature ", name->ToSring(), " doesn't exist");
        return signature;
    }

    const HAL::RootSignature* PipelineStateManager::GetNamedRootSignatureOrNull(std::optional<RootSignatureName> name) const
    {
        if (!name) return nullptr;

        const HAL::RootSignature* signature = GetRootSignature(*name);
        assert_format(signature, "Root signature ", name->ToSring(), " doesn't exist");
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

    void PipelineStateManager::CompileStates()
    {
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
        // BaseRootSignature.hlsl
        //
        // ConstantBuffer<GlobalData>      GlobalDataCB    : register(b0, space0);
        // ConstantBuffer<FrameData>       FrameDataCB     : register(b1, space0);
        // ConstantBuffer<PassDataType>    PassDataCB      : register(b2, space0);
        //
        // Texture2D       Textures2D[]        : register(t0, space0);
        // Texture3D       Textures3D[]        : register(t0, space1);
        // Texture2DArray  Texture2DArrays[]   : register(t0, space2);
        //
        
        // Global data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootDescriptorParameter{ 0, 0 });

        // Frame-specific data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootDescriptorParameter{ 1, 0 });

        // Pass-specific data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootDescriptorParameter{ 2, 0 });

        // Unbounded Texture2D range
        HAL::RootDescriptorTableParameter textures2D;
        textures2D.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 0 });
        mBaseRootSignature.AddDescriptorTableParameter(textures2D);

        // Unbounded Texture3D range
        HAL::RootDescriptorTableParameter textures3D;
        textures3D.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 1 });
        mBaseRootSignature.AddDescriptorTableParameter(textures3D);

        // Unbounded Texture2DArray range
        HAL::RootDescriptorTableParameter texture2DArrays;
        texture2DArrays.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 2 });
        mBaseRootSignature.AddDescriptorTableParameter(texture2DArrays);

        // Unbounded RWTexture2D range
        HAL::RootDescriptorTableParameter RWTextures2D;
        RWTextures2D.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 0 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures2D);

        // Unbounded RWTexture3D range
        HAL::RootDescriptorTableParameter RWTextures3D;
        RWTextures3D.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 1 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures3D);

        // Unbounded RWTexture2DArray range
        HAL::RootDescriptorTableParameter RWTexture2DArrays;
        RWTexture2DArrays.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 2 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTexture2DArrays);
    }

}