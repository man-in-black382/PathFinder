#pragma once

#include "Input.hpp"

#include <windows.h>
#include <vector>

namespace PathFinder 
{

    class InputHandlerWindows
    {
    public:
        InputHandlerWindows(Input* input, HWND hwnd);

        void HandleMessage(const MSG& winMsg);

    private:
        KeyboardKey VKeyToKey(uint32_t vKey);
        uint32_t KeyToVKey(KeyboardKey key);

        Input* mInput;
        HWND mHWND;
        std::vector<uint8_t> mRawInputBytes;
    };

}
