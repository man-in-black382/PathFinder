#pragma once

#include "RenderSettings.hpp"

#include "SceneGPUStorage.hpp"
#include "../Scene/Scene.hpp"
#include "../UI/UIGPUStorage.hpp"

namespace PathFinder
{

    struct RenderPassContentMediator
    {
    public:
        RenderPassContentMediator(const UIGPUStorage* uiStorage, const SceneGPUStorage* meshStorage, const Scene* scene, const RenderSettingsController* settingsContainer)
            : mUIStorage{ uiStorage }, mSceneStorage{ meshStorage }, mScene{ scene }, mRenderSettingsContainer{ settingsContainer } {}

    private:
        const UIGPUStorage* mUIStorage;
        const SceneGPUStorage* mSceneStorage;
        const Scene* mScene;
        const RenderSettingsController* mRenderSettingsContainer;

    public:
        inline const UIGPUStorage* GetUIGPUStorage() const { return mUIStorage; };
        inline const SceneGPUStorage* GetSceneGPUStorage() const { return mSceneStorage; };
        inline const Scene* GetScene() const { return mScene; };
        inline const RenderSettings* GetSettings() const { return &mRenderSettingsContainer->AppliedSettings(); };
    };

}
