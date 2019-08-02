#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "IGraphicsDevice.hpp"

namespace PathFinder
{

    class RenderContext
    {
    public:
        RenderContext(const Scene* scene, IGraphicsDevice* graphicsDevice);

    private:
        const Scene* mScene;
        IGraphicsDevice* mGraphicsDevice;

    public:
        inline const Scene* World() const { return mScene; }
        inline IGraphicsDevice* GraphicsDevice() const { return mGraphicsDevice; }
    };

}
