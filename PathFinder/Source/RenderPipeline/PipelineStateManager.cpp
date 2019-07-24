#include "PipelineStateManager.hpp"

namespace PathFinder
{

    PipelineStateManager::PipelineStateManager(HAL::Device* device, const RenderSurface& defaultRenderSurface)
        : mDevice{ device }, mDefaultRenderSurface{ defaultRenderSurface } {}

    void PipelineStateManager::ConfigureCommonStates()
    {
        mGBufferState.GetBlendState().SetBlendingEnabled(false);
        mGBufferState.GetDepthStencilState().SetDepthTestEnabled(true);
        mGBufferState.GetDepthStencilState().SetDepthWriteEnabled(true);
        mGBufferState.SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList);
        mGBufferState.GetRasterizerState().SetCullMode(HAL::RasterizerState::CullMode::Back);
        mGBufferState.GetRasterizerState().SetFillMode(HAL::RasterizerState::FillMode::Solid);
        mGBufferState.SetDepthStencilFormat(mDefaultRenderSurface.DepthStencilFormat());
    }

    void PipelineStateManager::BuildCommonRootSignature()
    {
        mCommonRootSignature.Compile(*mDevice);
    }

}