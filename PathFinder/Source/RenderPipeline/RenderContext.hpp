#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "GPUCommandRecorder.hpp"
#include "RootConstantsUpdater.hpp"
#include "ResourceProvider.hpp"
#include "RenderSurfaceDescription.hpp"
#include "UIGPUStorage.hpp"
#include "MeshGPUStorage.hpp"

namespace PathFinder
{

    template <class ContentMediator>
    class RenderContext
    {
    public:
        RenderContext(
            GPUCommandRecorder* graphicCommandRecorder, 
            RootConstantsUpdater* rootConstantsUpdater, 
            ResourceProvider* resourceProvider,
            const RenderSurfaceDescription& defaultRenderSurface)
            :
            mGrapicCommandRecorder{ graphicCommandRecorder },
            mRootConstantsUpdater{ rootConstantsUpdater },
            mResourceProvider{ resourceProvider },
            mDefaultRenderSurface{ defaultRenderSurface } {}

        void SetContentMediator(ContentMediator* mediator) { mContent = mediator; }

    private:
        ContentMediator* mContent;

        GPUCommandRecorder* mGrapicCommandRecorder;
        RootConstantsUpdater* mRootConstantsUpdater;
        ResourceProvider* mResourceProvider;
        RenderSurfaceDescription mDefaultRenderSurface;

    public:
        inline ContentMediator* GetContent() const { return mContent; }
        inline GPUCommandRecorder* GetCommandRecorder() const { return mGrapicCommandRecorder; }
        inline RootConstantsUpdater* GetConstantsUpdater() const { return mRootConstantsUpdater; }
        inline ResourceProvider* GetResourceProvider() const { return mResourceProvider; }
        inline const RenderSurfaceDescription& GetDefaultRenderSurfaceDesc() const { return mDefaultRenderSurface; }
    };

}
