#pragma once

#include "RenderSettings.hpp"

#include <Scene/SceneGPUStorage.hpp>
#include <Scene/Scene.hpp>
#include <UI/UIGPUStorage.hpp>
#include <IO/Input.hpp>
#include <Utility/DisplaySettingsController.hpp>

namespace PathFinder
{

    struct RenderPassContentMediator
    {
    public:
        RenderPassContentMediator(
            const UIGPUStorage* uiStorage, 
            const SceneGPUStorage* meshStorage, 
            const Scene* scene, 
            const Input* input, 
            const DisplaySettingsController* displaySettingsController,
            const RenderSettingsController* settingsContainer)
            : 
            mUIStorage{ uiStorage },
            mSceneStorage{ meshStorage },
            mScene{ scene }, 
            mInput{ input }, 
            mDisplaySettingsController{ displaySettingsController },
            mRenderSettingsContainer{ settingsContainer } {}

    private:
        const UIGPUStorage* mUIStorage;
        const SceneGPUStorage* mSceneStorage;
        const Scene* mScene;
        const Input* mInput;
        const DisplaySettingsController* mDisplaySettingsController;
        const RenderSettingsController* mRenderSettingsContainer;

    public:
        inline const UIGPUStorage* GetUIGPUStorage() const { return mUIStorage; };
        inline const SceneGPUStorage* GetSceneGPUStorage() const { return mSceneStorage; };
        inline const Scene* GetScene() const { return mScene; };
        inline const Input* GetInput() const { return mInput; };
        inline const DisplaySettingsController* DisplayController() const { return mDisplaySettingsController; };
        inline const RenderSettings* GetSettings() const { return &mRenderSettingsContainer->AppliedSettings(); };
    };

}
