#pragma once

#include "UIGPUStorage.hpp"
#include "MeshGPUStorage.hpp"

#include "../Scene/Scene.hpp"

namespace PathFinder
{

    struct RenderPassContentMediator
    {
    public:
        RenderPassContentMediator(UIGPUStorage* uiStorage, MeshGPUStorage* meshStorage, Scene* scene)
            : mUIStorage{ uiStorage }, mMeshStorage{ meshStorage }, mScene{ scene } {}

    private:
        UIGPUStorage* mUIStorage;
        MeshGPUStorage* mMeshStorage;
        Scene* mScene;

    public:
        inline UIGPUStorage* GetUIStorage() const { return mUIStorage; };
        inline MeshGPUStorage* GetMeshStorage() const { return mMeshStorage; };
        inline Scene* GetScene() const { return mScene; };
    };

}
