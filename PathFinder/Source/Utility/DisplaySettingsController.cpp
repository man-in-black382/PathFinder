#include "DisplaySettingsController.hpp"

namespace PathFinder
{

    DisplaySettingsController::DisplaySettingsController(HAL::DisplayAdapter* adapter, HAL::SwapChain* swapChain, HWND windowHandle)
        : mDisplayAdapter{ adapter }, mWindowHandle{ windowHandle }, mSwapChain{ swapChain }
    {
        FindPrimaryDisplay();
    }

    void DisplaySettingsController::HandleMessage(const MSG& winMsg)
    {
        switch (winMsg.message)
        {
        case WM_SIZE: HandleSizeChange(); break;
        case WM_MOVE: HandleMove(); break;
        default: break;
        }
    }

    void DisplaySettingsController::SetEnableHDRWhenSupported(bool enabled)
    {
        bool needsHDRUpdate = (enabled != mEnableHDRWhenSupported) && mPrimaryDisplay->IsHDRSupported();
        mEnableHDRWhenSupported = enabled;

        if (needsHDRUpdate)
        {
            mSwapChain->SetDisplay(mPrimaryDisplay, mEnableHDRWhenSupported);
        }
    }

    void DisplaySettingsController::HandleSizeChange()
    {
        FetchWindowRect();
        FindPrimaryDisplay();
    }

    void DisplaySettingsController::HandleMove()
    {
        FetchWindowRect();
        FindPrimaryDisplay();
    }

    void DisplaySettingsController::FindPrimaryDisplay()
    {
        float bestIntersectArea = -1;

        mDisplayAdapter->RefetchDisplaysIfNeeded();

        const HAL::Display* newPrimaryDisplay = mPrimaryDisplay;

        for (const HAL::Display& display : mDisplayAdapter->Displays())
        {
            float intersectionArea = 0.0;
            mWindowRect.Intersects(display.DesktopRect(), intersectionArea);

            if (intersectionArea > bestIntersectArea)
            {
                newPrimaryDisplay = &display;
                bestIntersectArea = intersectionArea;
            }
        }

        if (mPrimaryDisplay)
        {
            // Update HDR state only when HDR became supported and we also want it enabled or if support is dropped
            bool needsHDRUpdate =
                (newPrimaryDisplay->IsHDRSupported() != mPrimaryDisplay->IsHDRSupported()) &&
                (newPrimaryDisplay->IsHDRSupported() && mEnableHDRWhenSupported || !newPrimaryDisplay->IsHDRSupported());

            if (needsHDRUpdate)
            {
                mSwapChain->SetDisplay(newPrimaryDisplay, mEnableHDRWhenSupported);
            }
        }
        else
        {
            mSwapChain->SetDisplay(newPrimaryDisplay, mEnableHDRWhenSupported);
        }

        mPrimaryDisplay = newPrimaryDisplay;
    }

    void DisplaySettingsController::FetchWindowRect()
    {
        RECT r{};
        GetWindowRect(mWindowHandle, &r);
        mWindowRect = Geometry::Rect2D{ glm::vec2(r.left, r.top), Geometry::Size2D(r.right - r.left, r.bottom - r.top) };
    }

}
