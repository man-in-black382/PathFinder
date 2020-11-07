#pragma once

#include "UIManager.hpp"
#include "SceneManipulatorViewController.hpp"
#include "MainMenuViewController.hpp"

namespace PathFinder
{
   
    class UIEntryPoint
    {
    public:
        UIEntryPoint(UIManager* uiManager);

        void CreateMandatoryViewControllers();

    private:
        UIManager* mUIManager;

        std::shared_ptr<MainMenuViewController> mMainMenuVC;
        std::shared_ptr<SceneManipulatorViewController> mSceneManipulatorVC;
    };

}
