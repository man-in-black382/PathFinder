#pragma once

#include <RenderPipeline/PipelineResourceStorage.hpp>
#include <RenderPipeline/RenderEngine.hpp>
#include <RenderPipeline/RenderPassContentMediator.hpp>
#include <RenderPipeline/RenderDevice.hpp>
#include <RenderPipeline/RenderSettings.hpp>
#include <Scene/Scene.hpp>

namespace PathFinder
{
   
    // Dependencies that might be needed by view models
    struct UIDependencies
    {
        UIDependencies(
            const PipelineResourceStorage* resourceStorage, 
            RenderEngine<RenderPassContentMediator>* renderEngine,
            RenderSettings* renderSettings,
            const RenderDevice* renderDevice,
            Scene* scene)
            :
            ResourceStorage{ resourceStorage },
            RenderEngine{ renderEngine },
            Device{ renderDevice },
            ScenePtr{ scene },
            UserRenderSettings{ renderSettings }
        {}

        const PipelineResourceStorage* const ResourceStorage;
        RenderEngine<RenderPassContentMediator>* RenderEngine;
        Scene* const ScenePtr;
        const RenderDevice* const Device;
        RenderSettings* UserRenderSettings;
    };

}
