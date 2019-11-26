#pragma once

#include <functional>

#include "PipelineStateManager.hpp"

namespace PathFinder
{

    class PipelineStateCreator
    {
    public:
        PipelineStateCreator(PipelineStateManager* manager);

        HAL::RootSignature CloneBaseRootSignature();
        void StoreRootSignature(RootSignatureName name, HAL::RootSignature&& signature);
        void CreateGraphicsState(PSOName name, const PipelineStateManager::GraphicsStateConfigurator& configurator);
        void CreateComputeState(PSOName name, const PipelineStateManager::ComputeStateConfigurator& configurator);
        void CreateRayTracingState(PSOName name, const PipelineStateManager::RayTracingStateConfigurator& configurator);

    private:
        PipelineStateManager* mPipelineStateManager;
    };

}
