#pragma once

#include "../Foundation/Name.hpp"

#include "RenderPassMetadata.hpp"
#include "ResourceScheduler.hpp"
#include "ResourceProvider.hpp"
#include "RenderDevice.hpp"
#include "RenderContext.hpp"
#include "PipelineStateCreator.hpp"
#include "RootSignatureCreator.hpp"
#include "SubPassScheduler.hpp"

namespace PathFinder
{

    template <class ContentMediator>
    class RenderPass
    {
    public:
        RenderPass(Foundation::Name name, RenderPassPurpose purpose = RenderPassPurpose::Default)
            : mMetadata{ name, purpose } {};

        virtual ~RenderPass() = 0;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) {};
        virtual void ScheduleResources(ResourceScheduler* scheduler) {};
        virtual void ScheduleSubPasses(SubPassScheduler<ContentMediator>* scheduler) {};
        virtual void Render(RenderContext<ContentMediator>* context) {};

    private:
        RenderPassMetadata mMetadata;

    public:
        inline const RenderPassMetadata& Metadata() const { return mMetadata; }
    };

    template <class ContentMediator>
    RenderPass<ContentMediator>::~RenderPass() {}

}