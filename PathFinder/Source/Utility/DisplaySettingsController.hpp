#pragma once

#include <HardwareAbstractionLayer/DisplayAdapter.hpp>
#include <HardwareAbstractionLayer/SwapChain.hpp>
#include <Geometry/Rect2D.hpp>
#include <windows.h>

namespace PathFinder
{
    
    class DisplaySettingsController
    {
    public:
        DisplaySettingsController(HAL::DisplayAdapter* adapter, HAL::SwapChain* swapChain, HWND windowHandle);

        void HandleMessage(const MSG& winMsg);
        void SetEnableHDRWhenSupported(bool enabled);

    private:
        void HandleSizeChange();
        void HandleMove();

        void FindPrimaryDisplay();
        void FetchWindowRect();

        HAL::DisplayAdapter* mDisplayAdapter = nullptr;
        HAL::SwapChain* mSwapChain = nullptr;
        const HAL::Display* mPrimaryDisplay = nullptr;
        HWND mWindowHandle;
        Geometry::Rect2D mWindowRect;
        bool mEnableHDRWhenSupported = true;

    public:
        inline bool IsHDREnabled() const { return mEnableHDRWhenSupported && mPrimaryDisplay->IsHDRSupported(); }
        inline const HAL::Display* PrimaryDisplay() const { return mPrimaryDisplay; }
    };

}
