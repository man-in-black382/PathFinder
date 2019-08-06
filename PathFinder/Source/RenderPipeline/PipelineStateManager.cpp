#include "PipelineStateManager.hpp"

namespace PathFinder
{

    PipelineStateManager::PipelineStateManager(HAL::Device* device, const RenderSurface& defaultRenderSurface)
        : mDevice{ device }, mDefaultRenderSurface{ defaultRenderSurface } 
    {
        ConfigureDefaultStates();
        BuildUniversalRootSignature();
    }

    HAL::GraphicsPipelineState PipelineStateManager::CloneExistingGraphicsState(PSOName name)
    {
        return mGraphicPSOs[name].Clone();
    }

    void PipelineStateManager::StoreGraphicsState(PSOName name, const HAL::GraphicsPipelineState& pso)
    {
        mGraphicPSOs[name] = pso;
        mGraphicPSOs[name].SetRootSignature(&mUniversalRootSignature);
    }

    const HAL::PipelineState& PipelineStateManager::GetPipelineState(PSOName name) const 
    {
        auto stateIt = mGraphicPSOs.find(name);
        if (stateIt == mGraphicPSOs.end()) {
            throw std::invalid_argument("Pipeline state does not exist");
        }

        return stateIt->second;
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
        HAL::RootDescriptorParameter rootCB{ 0, 0 };
        mUniversalRootSignature.AddDescriptorParameter(rootCB);
    }

}