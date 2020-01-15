#pragma once

#include "../IO/Input.hpp"
#include "../Geometry/Dimensions.hpp"

#include <windef.h>

namespace PathFinder 
{

    class UIInteractor 
    {
    public:
        UIInteractor(HWND windowHandle, Input* input);

        void PollInputs();
        void SetViewportSize(const Geometry::Dimensions& size);

    private:
        void UpdateCursor();

        HWND mWindowHandle;
        Input* mInput;
    };

}
