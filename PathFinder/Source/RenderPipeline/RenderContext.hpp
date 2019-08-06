#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "IGraphicsDevice.hpp"
#include "RootConstantsUpdater.hpp"

namespace PathFinder
{

    class RenderContext
    {
    public:
        RenderContext(const Scene* scene, IGraphicsDevice* graphicsDevice, RootConstantsUpdater* rootConstantsUpdater);

    private:
        const Scene* mScene;
        IGraphicsDevice* mGraphicsDevice;
        RootConstantsUpdater* mRootConstantsUpdater;

    public:
        inline const Scene* World() const { return mScene; }
        inline IGraphicsDevice* GraphicsDevice() const { return mGraphicsDevice; }
        inline RootConstantsUpdater* ConstantsUpdater() const { return mRootConstantsUpdater; }
    };

}
