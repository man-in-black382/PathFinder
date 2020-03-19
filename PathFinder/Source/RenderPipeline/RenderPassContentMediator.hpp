#pragma once

#include "UIGPUStorage.hpp"
#include "SceneGPUStorage.hpp"

#include "../Scene/Scene.hpp"

namespace PathFinder
{

    struct RenderPassContentMediator
    {
    public:
        RenderPassContentMediator(const UIGPUStorage* uiStorage, const SceneGPUStorage* meshStorage, const Scene* scene)
            : mUIStorage{ uiStorage }, mSceneStorage{ meshStorage }, mScene{ scene } {}

    private:
        const UIGPUStorage* mUIStorage;
        const SceneGPUStorage* mSceneStorage;
        const Scene* mScene;

    public:
        inline const UIGPUStorage* GetUIGPUStorage() const { return mUIStorage; };
        inline const SceneGPUStorage* GetSceneGPUStorage() const { return mSceneStorage; };
        inline const Scene* GetScene() const { return mScene; };
    };

}
