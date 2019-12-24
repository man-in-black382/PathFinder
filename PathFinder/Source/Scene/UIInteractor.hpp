#pragma once

#include "../IO/Input.hpp"

#include <windef.h>

namespace PathFinder 
{

    class UIInteractor 
    {
    public:
        UIInteractor(HWND windowHandle, Input* input);

        void UpdateUIInputs();

    private:
        void UpdateCursor();

        HWND mWindowHandle;
        Input* mInput;
    };

}
