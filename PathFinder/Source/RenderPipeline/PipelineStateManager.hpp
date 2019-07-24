#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"

#include "RenderSurface.hpp"

#include <unordered_map>

namespace PathFinder
{

    class PipelineStateManager
    {
    public:
        using PSOName = Foundation::Name;

        PipelineStateManager(HAL::Device* device, const RenderSurface& defaultRenderSurface);

    private:
        void ConfigureCommonStates();
        void BuildCommonRootSignature();

        RenderSurface mDefaultRenderSurface;

        HAL::Device* mDevice;
        HAL::RootSignature mCommonRootSignature;

        std::unordered_map<PSOName, HAL::GraphicsPipelineState> mGraphicPSOs;
    };

}
