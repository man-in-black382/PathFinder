#pragma once

#include "PipelineStateManager.hpp"
#include "PipelineStateProxy.hpp"
#include "ShaderFileNames.hpp"

namespace PathFinder
{

    class PipelineStateCreator
    {
    public:
        PipelineStateCreator(PipelineStateManager* manager);

        HAL::RootSignature CloneBaseRootSignature();
        void StoreRootSignature(RootSignatureName name, const HAL::RootSignature& signature);
        void CreateGraphicsState(PSOName name, const std::function<void(GraphicsStateProxy & state)>& configurator);
        void CreateComputeState(PSOName name, const std::function<void(ComputeStateProxy & state)>& configurator);
        void CreateRayTracingState(PSOName name, const std::function<void(RayTracingStateProxy & state)>& configurator);

    private:
        PipelineStateManager* mPipelineStateManager;
    };

}
