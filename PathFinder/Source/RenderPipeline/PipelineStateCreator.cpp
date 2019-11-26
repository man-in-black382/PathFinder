#include "PipelineStateCreator.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateCreator::PipelineStateCreator(PipelineStateManager* manager)
        : mPipelineStateManager{ manager } {}

    HAL::RootSignature PipelineStateCreator::CloneBaseRootSignature()
    {
        return mPipelineStateManager->BaseRootSignature().Clone();
    }

    void PipelineStateCreator::StoreRootSignature(RootSignatureName name, HAL::RootSignature&& signature)
    {
        signature.SetDebugName(name.ToString());
        mPipelineStateManager->StoreRootSignature(name, std::move(signature)); 
    }

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