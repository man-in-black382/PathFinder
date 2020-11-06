#pragma once

#include <RenderPipeline/PipelineResourceStorage.hpp>
#include <RenderPipeline/RenderEngine.hpp>
#include <RenderPipeline/RenderPassContentMediator.hpp>
#include <Scene/Scene.hpp>

namespace PathFinder
{
   
    // Dependencies that might be needed by view models
    struct UIDependencies
    {
        UIDependencies(
            const PipelineResourceStorage* resourceStorage, 
            RenderEngine<RenderPassContentMediator>::Event* preRenderEvent,
            RenderEngine<RenderPassContentMediator>::Event* postRenderEvent,
            Scene* scene)
            :
            ResourceStorage{ resourceStorage },
            PreRenderEvent{ preRenderEvent },
            PostRenderEvent{ postRenderEvent },
            ScenePtr{ scene } {}

        const PipelineResourceStorage* const ResourceStorage;
        RenderEngine<RenderPassContentMediator>::Event* const PreRenderEvent;
        RenderEngine<RenderPassContentMediator>::Event* const PostRenderEvent;
        Scene* const ScenePtr;
    };

}
