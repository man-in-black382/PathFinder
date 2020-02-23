#pragma once

#include "UIGPUStorage.hpp"
#include "MeshGPUStorage.hpp"

#include "../Scene/Scene.hpp"

namespace PathFinder
{

    struct RenderPassContentMediator
    {
    public:
        RenderPassContentMediator(const UIGPUStorage* uiStorage, const MeshGPUStorage* meshStorage, const Scene* scene)
            : mUIStorage{ uiStorage }, mMeshStorage{ meshStorage }, mScene{ scene } {}

    private:
        const UIGPUStorage* mUIStorage;
        const MeshGPUStorage* mMeshStorage;
        const Scene* mScene;
    };

}
