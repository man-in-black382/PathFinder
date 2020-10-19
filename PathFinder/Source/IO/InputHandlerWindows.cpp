#include "InputHandlerWindows.hpp"


#include <Foundation/StringUtils.hpp>

namespace PathFinder
{

    InputHandlerWindows::InputHandlerWindows(Input* input, HWND hwnd)
        : mInput{ input }, mHWND{ hwnd }
    {
        UINT inputDeviceCount = 0;
        UINT err = GetRawInputDeviceList(NULL, &inputDeviceCount, sizeof(RAWINPUTDEVICELIST));
        assert_format(err != (UINT)-1, "Failed to retrieve RAWINPUTDEVICELIST(0). Error code: ", GetLastError());

        RAWINPUTDEVICELIST* list = nullptr;
        if (inputDeviceCount > 0) 
        {
            list = (RAWINPUTDEVICELIST*)malloc(sizeof(RAWINPUTDEVICELIST) * inputDeviceCount);
            err = GetRawInputDeviceList(list, &inputDeviceCount, sizeof(RAWINPUTDEVICELIST));
            assert_format(err != (UINT)-1, "Failed to retrieve RAWINPUTDEVICELIST(1). Error code: ", GetLastError());
        }

        bool keyboardConnected = false;
        bool mouseConnected = false;

        for (auto i = 0u; i < inputDeviceCount; ++i) 
        {
            if (list[i].dwType == RIM_TYPEMOUSE) 
            {
                mouseConnected = true;
            }
            else if (list[i].dwType == RIM_TYPEKEYBOARD) 
            {
                keyboardConnected = true;
            }
        }

        UINT rawDevicesCount = 0;
        RAWINPUTDEVICE rawDevices[2];

        if (mouseConnected)
        {
            rawDevices[rawDevicesCount].usUsagePage = 1;
            rawDevices[rawDevicesCount].usUsage = 2;
            rawDevices[rawDevicesCount].dwFlags = 0;
            rawDevices[rawDevicesCount].hwndTarget = hwnd;

            rawDevicesCount += 1;
        }

        if (keyboardConnected)
        {
            rawDevices[rawDevicesCount].usUsagePage = 1;
            rawDevices[rawDevicesCount].usUsage = 6;
            rawDevices[rawDevicesCount].dwFlags = RIDEV_NOLEGACY;
            rawDevices[rawDevicesCount].hwndTarget = hwnd;

            rawDevicesCount += 1;
        }

        assert_format(RegisterRawInputDevices(rawDevices, rawDevicesCount, sizeof(RAWINPUTDEVICE)),
            "Failed to register raw input devices. Error code: ", GetLastError());

        if (list) std::free(list);
    }

    void InputHandlerWindows::HandleMessage(const MSG& winMsg)
    {
        // Only process input when in focus
        if (GetForegroundWindow() != mHWND)
        {
            return;
        }

        if (winMsg.message == WM_INPUT)
        {
            UINT dataSize = 0;

            assert_format(GetRawInputData((HRAWINPUT)winMsg.lParam, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER)) == 0,
                "GetRawInputData failed. Can't retrieve system input. Error code: ", GetLastError());

            mRawInputBytes.resize(dataSize);

            auto result = GetRawInputData((HRAWINPUT)winMsg.lParam, RID_INPUT, mRawInputBytes.data(), &dataSize, sizeof(RAWINPUTHEADER));
            assert_format(result >= 0 && result == dataSize, "GetRawInputData failed. Can't retrieve system input. Error code: ", GetLastError());

            RAWINPUT* rawInput = (RAWINPUT*)mRawInputBytes.data();
            RAWINPUTHEADER& header = rawInput->header;

            if (header.dwType == RIM_TYPEMOUSE)
            {
                RAWMOUSE& mouse = rawInput->data.mouse;

                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) mInput->MouseDown(0);
                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) mInput->MouseDown(1);
                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) mInput->MouseDown(2);
                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) mInput->MouseDown(3);
                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) mInput->MouseDown(4);

                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) mInput->MouseUp(0);
                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) mInput->MouseUp(1);
                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP) mInput->MouseUp(2);
                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) mInput->MouseUp(3);
                if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) mInput->MouseUp(4);

                if (mouse.usButtonFlags & RI_MOUSE_WHEEL) mInput->SetScrollDelta({ 0, (SHORT)mouse.usButtonData });

                mInput->SetMouseDelta({ mouse.lLastX, mouse.lLastY });
            }
            else if (header.dwType == RIM_TYPEKEYBOARD)
            {
                RAWKEYBOARD& keyboard = rawInput->data.keyboard;

                if (keyboard.Message == WM_KEYUP || keyboard.Message == WM_SYSKEYUP)
                {
                    mInput->KeyboardKeyUp(VKeyToKey(keyboard.VKey), (KeyboardScanCode)keyboard.MakeCode, keyboard.VKey);
                }
                if (keyboard.Message == WM_KEYDOWN || keyboard.Message == WM_SYSKEYDOWN)
                {
                    mInput->KeyboardKeyDown(VKeyToKey(keyboard.VKey), (KeyboardScanCode)keyboard.MakeCode, keyboard.VKey);
                }
            }
        }
        else if (winMsg.message == WM_MOUSEMOVE)
        {
            // Use GetCursorPos to account for mouse ballistics
            POINT point{};
            if (::GetCursorPos(&point) && ::ScreenToClient(winMsg.hwnd, &point))
            {
                mInput->SetMouseAbsolutePosition({ point.x, point.y }, true);
            }
        }
    }

    KeyboardKey InputHandlerWindows::VKeyToKey(uint32_t vKey)
    {
        switch (vKey)
        {
        case 0x30:
            return KeyboardKey::Num0;
        case 0x31:
            return KeyboardKey::Num1;
        case 0x32:
            return KeyboardKey::Num2;
        case 0x33:
            return KeyboardKey::Num3;
        case 0x34:
            return KeyboardKey::Num4;
        case 0x35:
            return KeyboardKey::Num5;
        case 0x36:
            return KeyboardKey::Num6;
        case 0x37:
            return KeyboardKey::Num7;
        case 0x38:
            return KeyboardKey::Num8;
        case 0x39:
            return KeyboardKey::Num9;

        case VK_NUMPAD0:
            return KeyboardKey::NumPad0;
        case VK_NUMPAD1:
            return KeyboardKey::NumPad1;
        case VK_NUMPAD2:
            return KeyboardKey::NumPad2;
        case VK_NUMPAD3:
            return KeyboardKey::NumPad3;
        case VK_NUMPAD4:
            return KeyboardKey::NumPad4;
        case VK_NUMPAD5:
            return KeyboardKey::NumPad5;
        case VK_NUMPAD6:
            return KeyboardKey::NumPad6;
        case VK_NUMPAD7:
            return KeyboardKey::NumPad7;
        case VK_NUMPAD8:
            return KeyboardKey::NumPad8;
        case VK_NUMPAD9:
            return KeyboardKey::NumPad9;


        case 0x41:
            return KeyboardKey::A;
        case 0x42:
            return KeyboardKey::B;
        case 0x43:
            return KeyboardKey::C;
        case 0x44:
            return KeyboardKey::D;
        case 0x45:
            return KeyboardKey::E;
        case 0x46:
            return KeyboardKey::F;
        case 0x47:
            return KeyboardKey::G;
        case 0x48:
            return KeyboardKey::H;
        case 0x49:
            return KeyboardKey::I;
        case 0x4A:
            return KeyboardKey::J;
        case 0x4B:
            return KeyboardKey::K;
        case 0x4C:
            return KeyboardKey::L;
        case 0x4D:
            return KeyboardKey::M;
        case 0x4E:
            return KeyboardKey::N;
        case 0x4F:
            return KeyboardKey::O;
        case 0x50:
            return KeyboardKey::P;
        case 0x51:
            return KeyboardKey::Q;
        case 0x52:
            return KeyboardKey::R;
        case 0x53:
            return KeyboardKey::S;
        case 0x54:
            return KeyboardKey::T;
        case 0x55:
            return KeyboardKey::U;
        case 0x56:
            return KeyboardKey::V;
        case 0x57:
            return KeyboardKey::W;
        case 0x58:
            return KeyboardKey::X;
        case 0x59:
            return KeyboardKey::Y;
        case 0x5A:
            return KeyboardKey::Z;
        case VK_SPACE:
            return KeyboardKey::Space;

        case VK_F1:
            return KeyboardKey::F1;
        case VK_F2:
            return KeyboardKey::F2;
        case VK_F3:
            return KeyboardKey::F3;
        case VK_F4:
            return KeyboardKey::F4;
        case VK_F5:
            return KeyboardKey::F5;
        case VK_F6:
            return KeyboardKey::F6;
        case VK_F7:
            return KeyboardKey::F7;
        case VK_F8:
            return KeyboardKey::F8;
        case VK_F9:
            return KeyboardKey::F9;
        case VK_F10:
            return KeyboardKey::F10;
        case VK_F11:
            return KeyboardKey::F11;
        case VK_F12:
            return KeyboardKey::F12;

        case VK_ESCAPE:
            return KeyboardKey::Escape;
        case VK_OEM_3:
            return KeyboardKey::Tilde;

        case VK_OEM_MINUS:
            return KeyboardKey::MinusUnderscore;
        case VK_OEM_PLUS:
            return KeyboardKey::PlusEquals;
        case VK_BACK:
            return KeyboardKey::Backspace;
        case VK_OEM_1:
            return KeyboardKey::Semicolon;
        case VK_TAB:
            return KeyboardKey::Tab;
        case VK_OEM_4:
            return KeyboardKey::BracketSquaredLeft;
        case VK_OEM_6:
            return KeyboardKey::BracketSquaredRight;
        case VK_RETURN:
            return KeyboardKey::Enter;
        case VK_CAPITAL:
            return KeyboardKey::CapsLock;
        case VK_OEM_2:
            return KeyboardKey::Slash;
        case VK_OEM_5:
            return KeyboardKey::Backslash;
        case VK_SHIFT:
            return KeyboardKey::Shift;
        case VK_CONTROL:
            return KeyboardKey::Ctrl;
        case VK_MENU:
            return KeyboardKey::Alt;
        case VK_LWIN:
            return KeyboardKey::SuperLeft;
        case VK_RWIN:
            return KeyboardKey::SuperRight;
        case VK_LEFT:
            return KeyboardKey::Left;
        case VK_RIGHT:
            return KeyboardKey::Right;
        case VK_UP:
            return KeyboardKey::Up;
        case VK_DOWN:
            return KeyboardKey::Down;
        case VK_INSERT:
            return KeyboardKey::Insert;
        case VK_HOME:
            return KeyboardKey::Home;
        case VK_END:
            return KeyboardKey::End;
        case VK_DELETE:
            return KeyboardKey::Delete;
        case VK_PRIOR:
            return KeyboardKey::PageUp;
        case VK_NEXT:
            return KeyboardKey::PageDown;
        default:
            return KeyboardKey::END;
        }
    }

    uint32_t InputHandlerWindows::KeyToVKey(KeyboardKey key)
    {
        switch (key)
        {
        case KeyboardKey::Num0:
            return 0x30;
        case KeyboardKey::Num1:
            return 0x31;
        case KeyboardKey::Num2:
            return 0x32;
        case KeyboardKey::Num3:
            return 0x33;
        case KeyboardKey::Num4:
            return 0x34;
        case KeyboardKey::Num5:
            return 0x35;
        case KeyboardKey::Num6:
            return 0x36;
        case KeyboardKey::Num7:
            return 0x37;
        case KeyboardKey::Num8:
            return 0x38;
        case KeyboardKey::Num9:
            return 0x39;

        case KeyboardKey::NumPad0:
            return VK_NUMPAD0;
        case KeyboardKey::NumPad1:
            return VK_NUMPAD1;
        case KeyboardKey::NumPad2:
            return VK_NUMPAD2;
        case KeyboardKey::NumPad3:
            return VK_NUMPAD3;
        case KeyboardKey::NumPad4:
            return VK_NUMPAD4;
        case KeyboardKey::NumPad5:
            return VK_NUMPAD5;
        case KeyboardKey::NumPad6:
            return VK_NUMPAD6;
        case KeyboardKey::NumPad7:
            return VK_NUMPAD7;
        case KeyboardKey::NumPad8:
            return VK_NUMPAD8;
        case KeyboardKey::NumPad9:
            return VK_NUMPAD9;


        case KeyboardKey::A:
            return 0x41;
        case KeyboardKey::B:
            return 0x42;
        case KeyboardKey::C:
            return 0x43;
        case KeyboardKey::D:
            return 0x44;
        case KeyboardKey::E:
            return 0x45;
        case KeyboardKey::F:
            return 0x46;
        case KeyboardKey::G:
            return 0x47;
        case KeyboardKey::H:
            return 0x48;
        case KeyboardKey::I:
            return 0x49;
        case KeyboardKey::J:
            return 0x4A;
        case KeyboardKey::K:
            return 0x4B;
        case KeyboardKey::L:
            return 0x4C;
        case KeyboardKey::M:
            return 0x4D;
        case KeyboardKey::N:
            return 0x4E;
        case KeyboardKey::O:
            return 0x4F;
        case KeyboardKey::P:
            return 0x50;
        case KeyboardKey::Q:
            return 0x51;
        case KeyboardKey::R:
            return 0x52;
        case KeyboardKey::S:
            return 0x53;
        case KeyboardKey::T:
            return 0x54;
        case KeyboardKey::U:
            return 0x55;
        case KeyboardKey::V:
            return 0x56;
        case KeyboardKey::W:
            return 0x57;
        case KeyboardKey::X:
            return 0x58;
        case KeyboardKey::Y:
            return 0x59;
        case KeyboardKey::Z:
            return 0x5A;
        case KeyboardKey::Space:
            return VK_SPACE;

        case KeyboardKey::F1:
            return VK_F1;
        case KeyboardKey::F2:
            return VK_F2;
        case KeyboardKey::F3:
            return VK_F3;
        case KeyboardKey::F4:
            return VK_F4;
        case KeyboardKey::F5:
            return VK_F5;
        case KeyboardKey::F6:
            return VK_F6;
        case KeyboardKey::F7:
            return VK_F7;
        case KeyboardKey::F8:
            return VK_F8;
        case KeyboardKey::F9:
            return VK_F9;
        case KeyboardKey::F10:
            return VK_F10;
        case KeyboardKey::F11:
            return VK_F11;
        case KeyboardKey::F12:
            return VK_F12;

        case KeyboardKey::Escape:
            return VK_ESCAPE;
        case KeyboardKey::Tilde:
            return VK_OEM_3;

        case KeyboardKey::MinusUnderscore:
            return VK_OEM_MINUS;
        case KeyboardKey::PlusEquals:
            return VK_OEM_PLUS;
        case KeyboardKey::Backspace:
            return VK_BACK;
        case KeyboardKey::Semicolon:
            return VK_OEM_1;
        case KeyboardKey::Tab:
            return VK_TAB;
        case KeyboardKey::BracketSquaredLeft:
            return VK_OEM_4;
        case KeyboardKey::BracketSquaredRight:
            return VK_OEM_6;
        case KeyboardKey::Enter:
            return VK_RETURN;
        case KeyboardKey::CapsLock:
            return VK_CAPITAL;
        case KeyboardKey::Slash:
            return VK_OEM_2;
        case KeyboardKey::Backslash:
            return VK_OEM_5;
        case KeyboardKey::Shift:
            return VK_SHIFT;
        case KeyboardKey::Ctrl:
            return VK_CONTROL;
        case KeyboardKey::Alt:
            return VK_MENU;
        case KeyboardKey::SuperLeft:
            return VK_LWIN;
        case KeyboardKey::SuperRight:
            return VK_RWIN;
        case KeyboardKey::Left:
            return VK_LEFT;
        case KeyboardKey::Right:
            return VK_RIGHT;
        case KeyboardKey::Up:
            return VK_UP;
        case KeyboardKey::Down:
            return VK_DOWN;
        case KeyboardKey::Insert:
            return VK_INSERT;
        case KeyboardKey::Home:
            return VK_HOME;
        case KeyboardKey::End:
            return VK_END;
        case KeyboardKey::Delete:
            return VK_DELETE;
        case KeyboardKey::PageUp:
            return VK_PRIOR;
        case KeyboardKey::PageDown:
            return VK_NEXT;
        default:
            return 0;
        }
    }

}
