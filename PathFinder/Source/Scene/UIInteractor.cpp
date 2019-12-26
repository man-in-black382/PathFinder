#include "UIInteractor.hpp"

#include <imgui/imgui.h>
#include <windows.h>

namespace PathFinder 
{

    UIInteractor::UIInteractor(HWND windowHandle, Input* input)
        : mWindowHandle{ windowHandle }, mInput{ input }
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'misc/fonts/README.txt' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        UpdateUIInputs();
    }

    void UIInteractor::UpdateUIInputs()
    {
        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built!");

        // Setup display size (every frame to accommodate for window resizing)
        RECT rect;
        ::GetClientRect(mWindowHandle, &rect);
        io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

        io.KeyMap[ImGuiKey_Space] = VK_SPACE;

        // Setup time step
        //INT64 current_time;
        //::QueryPerformanceCounter((LARGE_INTEGER*)& current_time);
        //io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
        //g_Time = current_time;

        // Read keyboard modifiers inputs
       /* io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
        io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
        io.KeySuper = false;*/


        io.MousePos = ImVec2(mInput->MousePosition().x, mInput->MousePosition().y);

        UpdateCursor();
    }

    void UIInteractor::UpdateCursor()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        {
            return;
        }
            
        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            ::SetCursor(NULL);
        }
        else
        {
            // Show OS mouse cursor
            LPTSTR win32_cursor = IDC_ARROW;
            switch (imgui_cursor)
            {
            case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
            case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
            case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
            case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
            case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
            case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
            case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
            case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
            }
            ::SetCursor(::LoadCursor(NULL, win32_cursor));
        }
    }

}
