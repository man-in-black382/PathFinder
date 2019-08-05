#pragma once

#include "../Foundation/Name.hpp"

#include "ResourceScheduler.hpp"
#include "ResourceProvider.hpp"
#include "IShaderManager.hpp"
#include "IPipelineStateManager.hpp"
#include "GraphicsDevice.hpp"
#include "RenderContext.hpp"

#include "RenderPasses/PipelineNames.hpp"

namespace PathFinder
{

    class RenderPass
    {
    public:

        RenderPass(Foundation::Name name) : mName{ name } {}

        virtual void SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager) = 0;
        virtual void ScheduleResources(ResourceScheduler* scheduler) = 0;
        virtual void Render(RenderContext* context) = 0;

    private:
        Foundation::Name mName;

    public:
        inline Foundation::Name Name() const { return mName; }
    };

}
