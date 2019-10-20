#pragma once

#include <functional>

#include "PipelineStateManager.hpp"
#include "PipelineStateProxy.hpp"
#include "ShaderFileNames.hpp"

namespace PathFinder
{

    class PipelineStateCreator
    {
    public:
        using GraphicsStateConfigurator = std::function<void(GraphicsStateProxy&)>;
        using ComputeStateConfigurator = std::function<void(ComputeStateProxy&)>;
        using RayTracingStateConfigurator = std::function<void(RayTracingStateProxy&)>;

        PipelineStateCreator(PipelineStateManager* manager, const RenderSurfaceDescription& defaultRenderSurfaceDesc);

        HAL::RootSignature CloneBaseRootSignature();
        void StoreRootSignature(RootSignatureName name, HAL::RootSignature&& signature);
        void CreateGraphicsState(PSOName name, const GraphicsStateConfigurator& configurator);
        void CreateComputeState(PSOName name, const ComputeStateConfigurator& configurator);
        void CreateRayTracingState(PSOName name, const RayTracingStateConfigurator& configurator);

    private:
        PipelineStateManager* mPipelineStateManager;
        RenderSurfaceDescription mDefaultRenderSurfaceDesc;
    };

}
