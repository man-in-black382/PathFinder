#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"

#include "RenderSurface.hpp"
#include "IPipelineStateManager.hpp"

#include <unordered_map>

namespace PathFinder
{

    class PipelineStateManager : public IPipelineStateManager
    {
    public:
        PipelineStateManager(HAL::Device* device, const RenderSurface& defaultRenderSurface);

        virtual GraphicsPipelineState CloneDefaultGraphicsState() override;
        virtual GraphicsPipelineState CloneExistingGraphicsState(PSOName name) override;
        virtual void StoreGraphicsState(PSOName name, const GraphicsPipelineState& pso) override;

        void CompileStates();

    private:
        void ConfigureDefaultStates();
        void BuildUniversalRootSignature();

        RenderSurface mDefaultRenderSurface;
        
        HAL::Device* mDevice;
        HAL::RootSignature mUniversalRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;

        std::unordered_map<PSOName, HAL::GraphicsPipelineState> mGraphicPSOs;
    };

}
