#include "UIEntryPoint.hpp"

namespace PathFinder
{

    UIEntryPoint::UIEntryPoint(UIManager* uiManager)
        : mUIManager{ uiManager } {}

    void UIEntryPoint::CreateMandatoryViewControllers()
    {
        mMainMenuVC = mUIManager->CreateViewController<MainMenuViewController>();
        mSceneManipulatorVC = mUIManager->CreateViewController<SceneManipulatorViewController>();
    }

}
