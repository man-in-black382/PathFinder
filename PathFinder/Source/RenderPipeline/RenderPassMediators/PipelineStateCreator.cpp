#include "PipelineStateCreator.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateCreator::PipelineStateCreator(PipelineStateManager* manager)
        : mPipelineStateManager{ manager } {}

    void PipelineStateCreator::CreateGraphicsState(PSOName name, const PipelineStateManager::GraphicsStateConfigurator& configurator)
    {
        mPipelineStateManager->CreateGraphicsState(name, configurator);
    }

    void PipelineStateCreator::CreateComputeState(PSOName name, const PipelineStateManager::ComputeStateConfigurator& configurator)
    {
        mPipelineStateManager->CreateComputeState(name, configurator);
    }

    void PipelineStateCreator::CreateRayTracingState(PSOName name, const PipelineStateManager::RayTracingStateConfigurator& configurator)
    {
        mPipelineStateManager->CreateRayTracingState(name, configurator);
    }

}