#pragma once

#include <Foundation/Name.hpp>

#include "RenderPassMetadata.hpp"
#include "RenderContext.hpp"

#include "RenderPassMediators/ResourceScheduler.hpp"

namespace PathFinder
{

    template <class ContentMediator>
    class RenderSubPass
    {
    public:
        RenderSubPass(Foundation::Name name)
            : mMetadata{ name, RenderPassPurpose::Default } {};

        virtual ~RenderSubPass() = 0;

        virtual void ScheduleResources(ResourceScheduler* scheduler) {};
        virtual void Render(RenderContext<ContentMediator>* context) {};

    private:
        RenderPassMetadata mMetadata;

    public:
        inline const RenderPassMetadata& Metadata() const { return mMetadata; }
    };

    template <class ContentMediator>
    RenderSubPass<ContentMediator>::~RenderSubPass() {}

}