#include "PipelineStateManager.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateManager::PipelineStateManager(HAL::Device* device, const RenderSurface& defaultRenderSurface)
        : mDevice{ device }, mDefaultRenderSurface{ defaultRenderSurface }
    {
        ConfigureDefaultStates();
        BuildBaseRootSignature();
    }

    HAL::GraphicsPipelineState PipelineStateManager::CloneExistingGraphicsState(PSOName name)
    {
        return mGraphicPSOs[name].Clone();
    }

    HAL::ComputePipelineState PipelineStateManager::CloneDefaultComputeState()
    {
        return mDefaultComputeState.Clone();
    }

    HAL::ComputePipelineState PipelineStateManager::CloneExistingComputeState(PSOName name)
    {
        return mComputePSOs[name].Clone();
    }

    HAL::GraphicsPipelineState PipelineStateManager::CloneDefaultGraphicsState()
    {
        return mDefaultGraphicsState.Clone();
    }

    HAL::RootSignature PipelineStateManager::CloneBaseRootSignature()
    {
        return mBaseRootSignature;
    }

    void PipelineStateManager::StoreComputeState(PSOName name, const HAL::ComputePipelineState& pso, RootSignatureName assosiatedSignatureName)
    {
        mComputePSOs[name] = pso;
        mComputePSOs[name].SetRootSignature(&GetRootSignature(assosiatedSignatureName));
    }

    void PipelineStateManager::StoreGraphicsState(PSOName name, const HAL::GraphicsPipelineState& pso, RootSignatureName assosiatedSignatureName)
    {
        mGraphicPSOs[name] = pso;
        mGraphicPSOs[name].SetRootSignature(&GetRootSignature(assosiatedSignatureName));
    }

    void PipelineStateManager::StoreRootSignature(RootSignatureName name, const HAL::RootSignature& signature)
    {
        assert_format(mRootSignatures.find(name) == mRootSignatures.end(), "Rewriting root signatures is forbidden. ", name.ToSring(), " already exists.");
        mRootSignatures[name] = signature;
    }

    const HAL::RootSignature& PipelineStateManager::GetRootSignature(RootSignatureName name) const
    {
        auto it = mRootSignatures.find(name);
        assert_format(it != mRootSignatures.end(), "Root signature ", name.ToSring(), " does not exist");
        return it->second;
    }

    const HAL::PipelineState& PipelineStateManager::GetPipelineState(PSOName name) const
    {
        if (mGraphicPSOs.find(name) != mGraphicPSOs.end()) return mGraphicPSOs.at(name);
        if (mComputePSOs.find(name) != mComputePSOs.end()) return mComputePSOs.at(name);

        assert_format("Pipeline state ", name.ToSring(), " does not exist");
    }

    void PipelineStateManager::CompileStates()
    {
        for (auto& nameSigPair : mRootSignatures)
        {
            HAL::RootSignature& signature = nameSigPair.second;
            signature.Compile(*mDevice);
        }

        for (auto& nameStatePair : mGraphicPSOs)
        {
            HAL::GraphicsPipelineState& pso = nameStatePair.second;
            pso.Compile(*mDevice);
        }

        for (auto& nameStatePair : mComputePSOs)
        {
            HAL::ComputePipelineState& pso = nameStatePair.second;
            pso.Compile(*mDevice);
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
        // Texture2D       Textures2D[]        : register(t0, space0);
        // Texture3D       Textures3D[]        : register(t0, space1);
        // Texture2DArray  Texture2DArrays[]   : register(t0, space2);
        //
        // ConstantBuffer<GlobalData>      GlobalDataCB    : register(b0, space0);
        // ConstantBuffer<FrameData>       FrameDataCB     : register(b1, space0);
        // ConstantBuffer<PassDataType>    PassDataCB      : register(b2, space0);
        
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

        // Global data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootDescriptorParameter{ 0, 0 });

        // Frame-specific data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootDescriptorParameter{ 1, 0 });

        // Pass-specific data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootDescriptorParameter{ 2, 0 });
    }

}