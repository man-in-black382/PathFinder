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

    class RenderContext
    {
    public:
        RenderContext(
            const Scene* scene, 
            const MeshGPUStorage* meshStorage,
            const UIGPUStorage* uiStorage,
            GPUCommandRecorder* graphicCommandRecorder, 
            RootConstantsUpdater* rootConstantsUpdater, 
            ResourceProvider* resourceProvider,
            const RenderSurfaceDescription& defaultRenderSurface
        );

    private:
        const Scene* mScene;
        const MeshGPUStorage* mMeshStorage;
        const UIGPUStorage* mUIStorage;
        GPUCommandRecorder* mGrapicCommandRecorder;
        RootConstantsUpdater* mRootConstantsUpdater;
        ResourceProvider* mResourceProvider;
        RenderSurfaceDescription mDefaultRenderSurface;

    public:
        inline const Scene* GetScene() const { return mScene; }
        inline const MeshGPUStorage* GetMeshStorage() const { return mMeshStorage; }
        inline const UIGPUStorage* GetUIStorage() const { return mUIStorage; }
        inline GPUCommandRecorder* GetCommandRecorder() const { return mGrapicCommandRecorder; }
        inline RootConstantsUpdater* GetConstantsUpdater() const { return mRootConstantsUpdater; }
        inline ResourceProvider* GetResourceProvider() const { return mResourceProvider; }
        inline const RenderSurfaceDescription& GetDefaultRenderSurfaceDesc() const { return mDefaultRenderSurface; }
    };

}
