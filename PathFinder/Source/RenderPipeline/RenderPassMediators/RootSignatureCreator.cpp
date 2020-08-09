#include "RootSignatureCreator.hpp"

namespace PathFinder
{

    RootSignatureCreator::RootSignatureCreator(PipelineStateManager* manager)
        : mPipelineStateManager{ manager } {}

    void RootSignatureCreator::CreateRootSignature(RootSignatureName name, const PipelineStateManager::RootSignatureConfigurator& configurator)
    {
        mPipelineStateManager->CreateRootSignature(name, configurator);
    }

}