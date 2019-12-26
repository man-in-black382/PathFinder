#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "GPUCommandRecorder.hpp"
#include "RootConstantsUpdater.hpp"
#include "ResourceProvider.hpp"
#include "RenderSurfaceDescription.hpp"
#include "AssetResourceStorage.hpp"
#include "MeshGPUStorage.hpp"

namespace PathFinder
{

    class RenderContext
    {
    public:
        RenderContext(
            const Scene* scene, 
            const MeshGPUStorage* meshStorage,
            GPUCommandRecorder* graphicCommandRecorder, 
            RootConstantsUpdater* rootConstantsUpdater, 
            ResourceProvider* resourceProvider,
            const RenderSurfaceDescription& defaultRenderSurface
        );

    private:
        const Scene* mScene;
        const AssetResourceStorage* mAssetStorage;
        const MeshGPUStorage* mMeshStorage;
        GPUCommandRecorder* mGrapicCommandRecorder;
        RootConstantsUpdater* mRootConstantsUpdater;
        ResourceProvider* mResourceProvider;
        RenderSurfaceDescription mDefaultRenderSurface;

    public:
        inline const Scene* GetScene() const { return mScene; }
        inline const MeshGPUStorage* GetMeshStorage() const { return mMeshStorage; }
        inline GPUCommandRecorder* GetCommandRecorder() const { return mGrapicCommandRecorder; }
        inline RootConstantsUpdater* GetConstantsUpdater() const { return mRootConstantsUpdater; }
        inline ResourceProvider* GetResourceProvider() const { return mResourceProvider; }
        inline const RenderSurfaceDescription& GetDefaultRenderSurfaceDesc() const { return mDefaultRenderSurface; }
    };

}
