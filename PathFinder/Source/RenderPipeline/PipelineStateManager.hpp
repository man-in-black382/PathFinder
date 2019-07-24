#pragma once

#include "../HardwareAbstractionLayer/PipelineState.hpp"
#include "RenderSurface.hpp"

#include <unordered_map>

namespace PathFinder
{

    class PipelineStateManager
    {
    public:
        PipelineStateManager(HAL::Device* device, const RenderSurface& defaultRenderSurface);

    private:
        void ConfigureCommonStates();
        void BuildCommonRootSignature();

        RenderSurface mDefaultRenderSurface;

        HAL::Device* mDevice;
        HAL::RootSignature mCommonRootSignature;
        HAL::GraphicsPipelineState mDepthOnlyState;
        HAL::GraphicsPipelineState mGBufferState;
        HAL::GraphicsPipelineState mPostprocessState;
    };

}
