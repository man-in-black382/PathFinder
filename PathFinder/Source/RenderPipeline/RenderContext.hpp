#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "GraphicCommandRecorder.hpp"
#include "RootConstantsUpdater.hpp"
#include "ResourceProvider.hpp"

namespace PathFinder
{

    class RenderContext
    {
    public:
        RenderContext(const Scene* scene, GraphicCommandRecorder* graphicCommandRecorder, RootConstantsUpdater* rootConstantsUpdater, ResourceProvider* resourceProvider);

    private:
        const Scene* mScene;
        GraphicCommandRecorder* mGrapicCommandRecorder;
        RootConstantsUpdater* mRootConstantsUpdater;
        ResourceProvider* mResourceProvider;

    public:
        inline const Scene* GetScene() const { return mScene; }
        inline GraphicCommandRecorder* GetCommandRecorder() const { return mGrapicCommandRecorder; }
        inline RootConstantsUpdater* GetConstantsUpdater() const { return mRootConstantsUpdater; }
        inline ResourceProvider* GetResourceProvider() const { return mResourceProvider; }
    };

}
