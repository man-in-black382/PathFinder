#pragma once

#include "../PipelineStateManager.hpp"

namespace PathFinder
{

    class RootSignatureCreator
    {
    public:
        RootSignatureCreator(PipelineStateManager* manager);

        void CreateRootSignature(RootSignatureName name, const PipelineStateManager::RootSignatureConfigurator& configurator);

    private:
        PipelineStateManager* mPipelineStateManager;
    };

}
