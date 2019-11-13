#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "GPUCommandRecorder.hpp"
#include "RootConstantsUpdater.hpp"
#include "ResourceProvider.hpp"
#include "RenderSurfaceDescription.hpp"

namespace PathFinder
{

    class RenderContext
    {
    public:
        RenderContext(const Scene* scene, GPUCommandRecorder* graphicCommandRecorder, 
            RootConstantsUpdater* rootConstantsUpdater, ResourceProvider* resourceProvider,
            const RenderSurfaceDescription& defaultRenderSurface);

    private:
        const Scene* mScene;
        GPUCommandRecorder* mGrapicCommandRecorder;
        RootConstantsUpdater* mRootConstantsUpdater;
        ResourceProvider* mResourceProvider;
        RenderSurfaceDescription mDefaultRenderSurface;

    public:
        inline const Scene* GetScene() const { return mScene; }
        inline GPUCommandRecorder* GetCommandRecorder() const { return mGrapicCommandRecorder; }
        inline RootConstantsUpdater* GetConstantsUpdater() const { return mRootConstantsUpdater; }
        inline ResourceProvider* GetResourceProvider() const { return mResourceProvider; }
        inline const RenderSurfaceDescription& GetDefaultRenderSurfaceDesc() const { return mDefaultRenderSurface; }
    };

}
