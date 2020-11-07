#include "UIManager.hpp"
#include "ViewController.hpp"
#include "ViewModel.hpp"

#include <imgui/imgui.h>
#include <implot/implot.h>
#include <windows.h>

#include <Foundation/StringUtils.hpp>

namespace PathFinder
{

    UIManager::UIManager(Input* input, UIDependencies* dependencies, Memory::GPUResourceProducer* resourceProducer)
        : mInput{ input }, mGPUStorage{ resourceProducer }, mUIDependencies{ dependencies }
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
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

        io.KeyMap[ImGuiKey_Space] = VK_SPACE; // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
        io.KeyMap[ImGuiKey_Tab] = VK_TAB; 
        io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
        io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
        io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
        io.KeyMap[ImGuiKey_Home] = VK_HOME;
        io.KeyMap[ImGuiKey_End] = VK_END;
        io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
        io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
        io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
        io.KeyMap[ImGuiKey_A] = 'A';
        io.KeyMap[ImGuiKey_C] = 'C';
        io.KeyMap[ImGuiKey_V] = 'V';
        io.KeyMap[ImGuiKey_X] = 'X';
        io.KeyMap[ImGuiKey_Y] = 'Y';
        io.KeyMap[ImGuiKey_Z] = 'Z';

        UpdateCursor();

        mInput->KeyUpEvent() += { "UIManager.Key.Up", this, &UIManager::HandleKeyUp };
        mInput->KeyDownEvent() += { "UIManager.Key.Down", this, &UIManager::HandleKeyDown };
    }

    UIManager::~UIManager()
    {
        mInput->KeyUpEvent() -= "UIManager.Key.Up";
        mInput->KeyDownEvent() -= "UIManager.Key.Down";

        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    void UIManager::Draw()
    {
        // Setup time step
        //INT64 current_time;
        //::QueryPerformanceCounter((LARGE_INTEGER*)& current_time);
        //io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
        //g_Time = current_time;

        ImGuiIO& io = ImGui::GetIO();

        io.KeyCtrl = mInput->IsKeyboardKeyPressed(KeyboardKey::Ctrl);
        io.KeyShift = mInput->IsKeyboardKeyPressed(KeyboardKey::Shift);
        io.KeyAlt = mInput->IsKeyboardKeyPressed(KeyboardKey::Alt);
        io.MouseWheel = mInput->ScrollDelta().y;

        io.KeySuper =
            mInput->IsKeyboardKeyPressed(KeyboardKey::SuperLeft) ||
            mInput->IsKeyboardKeyPressed(KeyboardKey::SuperRight);

        for (auto i = 0; i < std::size(io.MouseDown); ++i)
        {
            io.MouseDown[i] = mInput->IsMouseButtonPressed(i);
        }

        io.MousePos = { mInput->MousePosition().x, mInput->MousePosition().y };

        mIsInteracting = ImGui::IsAnyItemActive() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused();
        mIsMouseOverUI = ImGui::IsAnyWindowHovered();

        UpdateCursor();

        mGPUStorage.StartNewFrame();

        std::vector<uint32_t> deallocatedVCIndices;

        for (auto vcIdx = 0u; vcIdx < mViewControllers.size(); ++vcIdx)
        {
            std::weak_ptr<ViewController>& vc = mViewControllers[vcIdx];

            if (auto strongPtr = vc.lock())
            {
                strongPtr->Draw();
                mIsInteracting = mIsInteracting || strongPtr->IsInteracting();
            }
            else
            {
                deallocatedVCIndices.push_back(vcIdx);
            }
        }

        // Remove deallocated controllers
        for (auto it = deallocatedVCIndices.rbegin(); it != deallocatedVCIndices.rend(); ++it)
        {
            mViewControllers.erase(mViewControllers.begin() + *it);
        }

        mGPUStorage.UploadUI();
    }

    void UIManager::SetViewportSize(const Geometry::Dimensions& size)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2{ (float)size.Width, (float)size.Height };
    }

    bool UIManager::IsInteracting() const
    {
        return mIsInteracting;
    }

    bool UIManager::IsMouseOverUI() const
    {
        return mIsMouseOverUI;
    }

    void UIManager::HandleKeyUp(KeyboardKey key, const KeyboardKeyInfo& info, const Input* input)
    {
        ImGui::GetIO().KeysDown[info.VirtualKey] = false;
    }

    void UIManager::HandleKeyDown(KeyboardKey key, const KeyboardKeyInfo& info, const Input* input)
    {
        ImGui::GetIO().KeysDown[info.VirtualKey] = true;
        ImGui::GetIO().AddInputCharacter(info.VirtualKey);
    }

    void UIManager::UpdateCursor()
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
            SetCursor(NULL);
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
            SetCursor(LoadCursor(NULL, win32_cursor));
        }
    }

}
