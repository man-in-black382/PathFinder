#pragma once

#include "UIGPUStorage.hpp"
#include "SceneGPUStorage.hpp"

#include "../Scene/Scene.hpp"

namespace PathFinder
{

    struct RenderPassContentMediator
    {
    public:
        RenderPassContentMediator(UIGPUStorage* uiStorage, SceneGPUStorage* meshStorage, Scene* scene)
            : mUIStorage{ uiStorage }, mSceneStorage{ meshStorage }, mScene{ scene } {}

    private:
        UIGPUStorage* mUIStorage;
        SceneGPUStorage* mSceneStorage;
        Scene* mScene;

    public:
        inline UIGPUStorage* GetUIGPUStorage() const { return mUIStorage; };
        inline SceneGPUStorage* GetSceneGPUStorage() const { return mSceneStorage; };
        inline Scene* GetScene() const { return mScene; };
    };

}
