#include "PipelineStateManager.hpp"

namespace PathFinder
{

    PipelineStateManager::PipelineStateManager(HAL::Device* device, const RenderSurface& defaultRenderSurface)
        : mDevice{ device }, mDefaultRenderSurface{ defaultRenderSurface } {}

    HAL::GraphicsPipelineState PipelineStateManager::CloneExistingGraphicsState(PSOName name)
    {
        return mGraphicPSOs[name].Clone();
    }

    void PipelineStateManager::StoreGraphicsState(PSOName name, const HAL::GraphicsPipelineState& pso)
    {
        mGraphicPSOs[name] = pso;
        mGraphicPSOs[name].SetRootSignature(&mUniversalRootSignature);
    }

    HAL::PipelineState& PipelineStateManager::GetPipelineState(PSOName name)
    {
        return mGraphicPSOs[name];
    }

    void PipelineStateManager::CompileStates()
    {
        mUniversalRootSignature.Compile(*mDevice);

        for (auto& nameStatePair : mGraphicPSOs)
        {
            HAL::GraphicsPipelineState& pso = nameStatePair.second;
            pso.Compile(*mDevice);
        }
    }

    HAL::GraphicsPipelineState PipelineStateManager::CloneDefaultGraphicsState()
    {
        return mDefaultGraphicsState.Clone();
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
        mDefaultGraphicsState.SetRootSignature(&mUniversalRootSignature);
    }

    void PipelineStateManager::BuildUniversalRootSignature()
    {
        
    }

}