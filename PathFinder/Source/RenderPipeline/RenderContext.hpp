#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "IGraphicsDevice.hpp"
#include "RootConstantsUpdater.hpp"
#include "ResourceProvider.hpp"

namespace PathFinder
{

    class RenderContext
    {
    public:
        RenderContext(const Scene* scene, IGraphicsDevice* graphicsDevice, RootConstantsUpdater* rootConstantsUpdater, ResourceProvider* resourceProvider);

    private:
        const Scene* mScene;
        IGraphicsDevice* mGraphicsDevice;
        RootConstantsUpdater* mRootConstantsUpdater;
        ResourceProvider* mResourceProvider;

    public:
        inline const Scene* GetScene() const { return mScene; }
        inline IGraphicsDevice* GetGraphicsDevice() const { return mGraphicsDevice; }
        inline RootConstantsUpdater* GetConstantsUpdater() const { return mRootConstantsUpdater; }
        inline ResourceProvider* GetResourceProvider() const { return mResourceProvider; }
    };

}
