#pragma once

#include "../Foundation/Name.hpp"

#include "ResourceScheduler.hpp"
#include "ResourceProvider.hpp"
#include "GraphicsDevice.hpp"
#include "RenderContext.hpp"
#include "PipelineStateCreator.hpp"

#include "RenderPasses/PipelineNames.hpp"

namespace PathFinder
{

    class RenderPass
    {
    public:
        enum class Purpose
        {
            // Every-frame render pass.
            Default, 
            // One-time render pass to setup common states and/or resources.
            Setup, 
            // One-time render pass for asset preprocessing.
            // Asset resources will be specifically transitioned
            // to appropriate states before and after execution of such passes.
            AssetProcessing 
        };

        RenderPass(Foundation::Name name, Purpose purpose = Purpose::Default);
        virtual ~RenderPass() = 0;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator);
        virtual void ScheduleResources(ResourceScheduler* scheduler);
        virtual void Render(RenderContext* context);

    private:
        Foundation::Name mName;
        Purpose mPurpose = Purpose::Default;

    public:
        inline Foundation::Name Name() const { return mName; }
        inline Purpose PurposeInPipeline() const { return mPurpose; }
    };

}
