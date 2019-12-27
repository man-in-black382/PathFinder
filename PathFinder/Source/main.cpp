#pragma once

#include <windows.h>
#include <tchar.h>

#include "Scene/Scene.hpp"
#include "Scene/MeshLoader.hpp"
#include "Scene/MaterialLoader.hpp"
#include "Scene/UIInteractor.hpp"
#include "Scene/CameraInteractor.hpp"

#include "RenderPipeline/RenderEngine.hpp"
#include "RenderPipeline/RenderPasses/GBufferRenderPass.hpp"
#include "RenderPipeline/RenderPasses/BlurRenderPass.hpp"
#include "RenderPipeline/RenderPasses/BackBufferOutputPass.hpp"
#include "RenderPipeline/RenderPasses/ShadowsRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DeferredLightingRenderPass.hpp"
#include "RenderPipeline/RenderPasses/ToneMappingRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DisplacementDistanceMapRenderPass.hpp"

#include "IO/CommandLineParser.hpp"
#include "IO/Input.hpp"

#include "../resource.h"

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

//struct FrameContext
//{
//    ID3D12CommandAllocator* CommandAllocator;
//    UINT64                  FenceValue;
//};
//
//// Data
//static int const                    NUM_FRAMES_IN_FLIGHT = 3;
//static FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
//static UINT                         g_frameIndex = 0;
//
//static int const                    NUM_BACK_BUFFERS = 3;
//static ID3D12Device*                g_pd3dDevice = NULL;
//static ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = NULL;
//static ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = NULL;
//static ID3D12CommandQueue*          g_pd3dCommandQueue = NULL;
//static ID3D12GraphicsCommandList*   g_pd3dCommandList = NULL;
//static ID3D12Fence*                 g_fence = NULL;
//static HANDLE                       g_fenceEvent = NULL;
//static UINT64                       g_fenceLastSignaledValue = 0;
//static IDXGISwapChain3*             g_pSwapChain = NULL;
//static HANDLE                       g_hSwapChainWaitableObject = NULL;
//static ID3D12Resource*              g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
//static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
//
//// Forward declarations of helper functions
//bool CreateDeviceD3D(HWND hWnd);
//void CleanupDeviceD3D();
//void CreateRenderTarget();
//void CleanupRenderTarget();
//void WaitForLastSubmittedFrame();
//FrameContext*   WaitForNextFrameResources();
//void ResizeSwapChain(HWND hWnd, int width, int height);
//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//
//// Main code
//int main(int, char**)
//{
//    // Create application window
//    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
//    ::RegisterClassEx(&wc);
//    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX12 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
//
//    // Initialize Direct3D
//    if (!CreateDeviceD3D(hwnd))
//    {
//        CleanupDeviceD3D();
//        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
//        return 1;
//    }
//
//    // Show the window
//    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
//    ::UpdateWindow(hwnd);
//
//    // Setup Dear ImGui context
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO(); (void)io;
//    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
//    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
//
//    // Setup Dear ImGui style
//    ImGui::StyleColorsDark();
//    //ImGui::StyleColorsClassic();
//
//    // Setup Platform/Renderer bindings
//    ImGui_ImplWin32_Init(hwnd);
//    ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
//        DXGI_FORMAT_R8G8B8A8_UNORM,
//        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
//        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
//
//    // Load Fonts
//    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
//    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
//    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
//    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
//    // - Read 'misc/fonts/README.txt' for more instructions and details.
//    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
//    //io.Fonts->AddFontDefault();
//    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
//    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
//    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
//    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
//    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
//    //IM_ASSERT(font != NULL);
//
//    // Our state
//    bool show_demo_window = true;
//    bool show_another_window = false;
//    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
//
//    // Main loop
//    MSG msg;
//    ZeroMemory(&msg, sizeof(msg));
//    while (msg.message != WM_QUIT)
//    {
//        // Poll and handle messages (inputs, window resize, etc.)
//        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
//        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
//        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
//        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
//        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
//        {
//            ::TranslateMessage(&msg);
//            ::DispatchMessage(&msg);
//            continue;
//        }
//
//        // Start the Dear ImGui frame
//        ImGui_ImplDX12_NewFrame();
//        ImGui_ImplWin32_NewFrame();
//        ImGui::NewFrame();
//
//        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
//        if (show_demo_window)
//            ImGui::ShowDemoWindow(&show_demo_window);
//
//        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
//        {
//            static float f = 0.0f;
//            static int counter = 0;
//
//            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
//
//            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
//            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
//            ImGui::Checkbox("Another Window", &show_another_window);
//
//            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
//            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
//
//            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
//                counter++;
//            ImGui::SameLine();
//            ImGui::Text("counter = %d", counter);
//
//            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//            ImGui::End();
//        }
//
//        // 3. Show another simple window.
//        if (show_another_window)
//        {
//            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
//            ImGui::Text("Hello from another window!");
//            if (ImGui::Button("Close Me"))
//                show_another_window = false;
//            ImGui::End();
//        }
//
//        // Rendering
//        FrameContext* frameCtxt = WaitForNextFrameResources();
//        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
//        frameCtxt->CommandAllocator->Reset();
//
//        D3D12_RESOURCE_BARRIER barrier = {};
//        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//        barrier.Transition.pResource   = g_mainRenderTargetResource[backBufferIdx];
//        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
//        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
//        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
//
//        g_pd3dCommandList->Reset(frameCtxt->CommandAllocator, NULL);
//        g_pd3dCommandList->ResourceBarrier(1, &barrier);
//        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], (float*)&clear_color, 0, NULL);
//        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
//        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
//        ImGui::Render();
//        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
//        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
//        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
//        g_pd3dCommandList->ResourceBarrier(1, &barrier);
//        g_pd3dCommandList->Close();
//
//        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);
//
//        g_pSwapChain->Present(1, 0); // Present with vsync
//        //g_pSwapChain->Present(0, 0); // Present without vsync
//
//        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
//        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
//        g_fenceLastSignaledValue = fenceValue;
//        frameCtxt->FenceValue = fenceValue;
//    }
//
//    WaitForLastSubmittedFrame();
//    ImGui_ImplDX12_Shutdown();
//    ImGui_ImplWin32_Shutdown();
//    ImGui::DestroyContext();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

int main(int argc, char** argv)
{
    using namespace HAL;
    
    HICON iconHandle = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), iconHandle, NULL, NULL, NULL, _T("PathFinder"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("PathFinder"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    PathFinder::CommandLineParser cmdLineParser{ argc, argv };

    auto displacementDistanceMapGenerationPass = std::make_unique<PathFinder::DisplacementDistanceMapRenderPass>();
    auto gBufferPass = std::make_unique<PathFinder::GBufferRenderPass>();
    auto deferredLightingPass = std::make_unique<PathFinder::DeferredLightingRenderPass>();
    auto shadowsPass = std::make_unique<PathFinder::ShadowsRenderPass>();
    auto toneMappingPass = std::make_unique<PathFinder::ToneMappingRenderPass>();
    auto blurPass = std::make_unique<PathFinder::BlurRenderPass>();
    auto backBufferOutputPass = std::make_unique<PathFinder::BackBufferOutputPass>();

    PathFinder::RenderPassExecutionGraph renderPassGraph;
    renderPassGraph.AddPass(displacementDistanceMapGenerationPass.get());
    renderPassGraph.AddPass(gBufferPass.get());
    renderPassGraph.AddPass(deferredLightingPass.get());
    renderPassGraph.AddPass(toneMappingPass.get());
    //renderPassGraph.AddPass(shadowsPass.get());
    //renderPassGraph.AddPass(blurPass.get());
    renderPassGraph.AddPass(backBufferOutputPass.get());

    PathFinder::Scene scene{};
    PathFinder::Input input{};
    PathFinder::UIInteractor uiInteractor{ hwnd, &input };
    PathFinder::CameraInteractor cameraInteractor{ &scene.MainCamera(), &input };
    PathFinder::RenderEngine engine{ hwnd, cmdLineParser, &scene, &renderPassGraph };
    PathFinder::MeshLoader meshLoader{ cmdLineParser.ExecutableFolderPath() / "MediaResources/Models/", &engine.VertexGPUStorage() };
    PathFinder::MaterialLoader materialLoader{ cmdLineParser.ExecutableFolderPath() / "MediaResources/Textures/", &engine.Device(), &engine.AssetGPUStorage(), &engine.StandardCopyDevice() };

    PathFinder::Material& metalMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/Metal07/Metal07_col.dds", "/Metal07/Metal07_nrm.dds", "/Metal07/Metal07_rgh.dds",
        "/Metal07/Metal07_met.dds", "/Metal07/Metal07_disp.dds"));

    PathFinder::Material& harshBricksMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/HarshBricks/harshbricks-albedo.dds", "/HarshBricks/harshbricks-normal.dds", "/HarshBricks/harshbricks-roughness.dds",
        "/HarshBricks/harshbricks-metalness.dds", "/HarshBricks/harshbricks-height.dds"));

    PathFinder::Mesh& sphere = scene.AddMesh(std::move(meshLoader.Load("plane.obj").back()));
    PathFinder::MeshInstance& sphereInstance = scene.AddMeshInstance({ &sphere, &harshBricksMaterial });

    auto t = sphereInstance.Transformation();
    t.Rotation = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0)));
    sphereInstance.SetTransformation(t);    
     
    PathFinder::Camera& camera = scene.MainCamera();
    camera.SetFarPlane(1000);
    camera.SetNearPlane(1);
    camera.MoveTo({ 0.0, 0.0f, 25.f });
    camera.LookAt({ 0.f, 0.0f, 0.f });
    camera.SetViewportAspectRatio(16.0f / 9.0f);

    engine.ScheduleAndAllocatePipelineResources();
    engine.ProcessAndTransferAssets();

    materialLoader.SerializePostprocessedTextures();

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        engine.Render();
    }

    engine.FlushAllQueuedFrames();

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}