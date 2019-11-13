#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "GPUCommandRecorder.hpp"
#include "RootConstantsUpdater.hpp"
#include "ResourceProvider.hpp"
#include "RenderSurfaceDescription.hpp"
#include "AssetResourceStorage.hpp"
#include "VertexStorage.hpp"

namespace PathFinder
{

    class RenderContext
    {
    public:
        RenderContext(
            const Scene* scene, 
            const AssetResourceStorage* assetStorage,
            const VertexStorage* vertexStorage,
            GPUCommandRecorder* graphicCommandRecorder, 
            RootConstantsUpdater* rootConstantsUpdater, 
            ResourceProvider* resourceProvider,
            const RenderSurfaceDescription& defaultRenderSurface
        );

    private:
        const Scene* mScene;
        const AssetResourceStorage* mAssetStorage;
        const VertexStorage* mVertexStorage;
        GPUCommandRecorder* mGrapicCommandRecorder;
        RootConstantsUpdater* mRootConstantsUpdater;
        ResourceProvider* mResourceProvider;
        RenderSurfaceDescription mDefaultRenderSurface;

    public:
        inline const Scene* GetScene() const { return mScene; }
        inline const AssetResourceStorage* GetAssetStorage() const { return mAssetStorage; }
        inline const VertexStorage* GetVertexStorage() const { return mVertexStorage; }
        inline GPUCommandRecorder* GetCommandRecorder() const { return mGrapicCommandRecorder; }
        inline RootConstantsUpdater* GetConstantsUpdater() const { return mRootConstantsUpdater; }
        inline ResourceProvider* GetResourceProvider() const { return mResourceProvider; }
        inline const RenderSurfaceDescription& GetDefaultRenderSurfaceDesc() const { return mDefaultRenderSurface; }
    };

}
