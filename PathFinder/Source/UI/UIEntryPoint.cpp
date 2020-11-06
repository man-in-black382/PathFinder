#include "UIEntryPoint.hpp"

namespace PathFinder
{

    UIEntryPoint::UIEntryPoint(UIManager* uiManager)
        : mUIManager{ uiManager } {}

    void UIEntryPoint::CreateMandatoryViewControllers()
    {
        mSceneManipulatorVC = mUIManager->CreateViewController<SceneManipulatorViewController>();
    }

}
