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
#include "RenderPipeline/RenderPasses/UIRenderPass.hpp"
#include "RenderPipeline/RenderPasses/CommonSetupRenderPass.hpp"
#include "RenderPipeline/GlobalRootConstants.hpp"
#include "RenderPipeline/PerFrameRootConstants.hpp"
#include "RenderPipeline/RenderPassContentMediator.hpp"

#include "IO/CommandLineParser.hpp"
#include "IO/Input.hpp"
#include "IO/InputHandlerWindows.hpp"

#include "../resource.h"

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <glm/common.hpp>

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

    PathFinder::RenderEngine<PathFinder::RenderPassContentMediator> engine{ hwnd, cmdLineParser };
    PathFinder::SceneGPUStorage sceneStorage{ &engine.Device(), &engine.ResourceProducer() };
    PathFinder::UIGPUStorage uiStorage{ &engine.ResourceProducer() };
    PathFinder::Scene scene{ cmdLineParser.ExecutableFolderPath(), &engine.ResourceProducer() };
    PathFinder::Input input{};
    PathFinder::InputHandlerWindows windowsInputHandler{ &input, hwnd };
    PathFinder::UIInteractor uiInteractor{ hwnd, &input };
    PathFinder::CameraInteractor cameraInteractor{ &scene.MainCamera(), &input };
    PathFinder::MeshLoader meshLoader{ cmdLineParser.ExecutableFolderPath() / "MediaResources/Models/" };
    PathFinder::MaterialLoader materialLoader{ cmdLineParser.ExecutableFolderPath() / "MediaResources/Textures/", &engine.AssetStorage(), &engine.ResourceProducer() };
    PathFinder::RenderPassContentMediator contentMediator{ &uiStorage, &sceneStorage, &scene };

    auto commonSetupPass = std::make_unique<PathFinder::CommonSetupRenderPass>();
    auto distanceFieldGenerationPass = std::make_unique<PathFinder::DisplacementDistanceMapRenderPass>();
    auto GBufferPass = std::make_unique<PathFinder::GBufferRenderPass>();
    auto deferredLightingPass = std::make_unique<PathFinder::DeferredLightingRenderPass>();
    auto toneMappingPass = std::make_unique<PathFinder::ToneMappingRenderPass>();
    auto backBufferOutputPass = std::make_unique<PathFinder::BackBufferOutputPass>();
    auto uiPass = std::make_unique<PathFinder::UIRenderPass>();

    engine.SetContentMediator(&contentMediator);
    engine.AddRenderPass(commonSetupPass.get());
    engine.AddRenderPass(distanceFieldGenerationPass.get());
    engine.AddRenderPass(GBufferPass.get());
    engine.AddRenderPass(deferredLightingPass.get());
    engine.AddRenderPass(toneMappingPass.get());
    engine.AddRenderPass(backBufferOutputPass.get());
    engine.AddRenderPass(uiPass.get());

    //renderPassGraph.AddPass(PathFinder::ShadowsRenderPass{});
    //renderPassGraph.AddPass(blurPass.get());

    PathFinder::Scene::FlatLightIt diskLight0 = scene.EmplaceFlatLight();
    diskLight0->SetWidth(4);
    diskLight0->SetHeight(4);
    diskLight0->SetPosition({ 0.0, 12.0, 0.0 });
    diskLight0->SetNormal({ 0.0, -1.0, 0.0 });
    diskLight0->SetColor(Foundation::Color::White());
    diskLight0->SetLuminousPower(50000);

    PathFinder::Material& metalMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/Metal07/Metal07_col.dds", "/Metal07/Metal07_nrm.dds", "/Metal07/Metal07_rgh.dds",
        "/Metal07/Metal07_met.dds", "/Metal07/Metal07_disp.dds"));

    PathFinder::Material& harshBricksMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/HarshBricks/harshbricks-albedo.dds", "/HarshBricks/harshbricks-normal.dds", "/HarshBricks/harshbricks-roughness.dds",
        "/HarshBricks/harshbricks-metalness.dds", "/HarshBricks/harshbricks-height.dds"));

    PathFinder::Material& concrete19Material = scene.AddMaterial(materialLoader.LoadMaterial(
        "/Concrete19/Concrete19_col.dds", "/Concrete19/Concrete19_nrm.dds", "/Concrete19/Concrete19_rgh.dds",
        std::nullopt, "/Concrete19/Concrete19_disp.dds"));

    PathFinder::Mesh& sphere = scene.AddMesh(std::move(meshLoader.Load("plane.obj").back()));
    PathFinder::MeshInstance& sphereInstance = scene.AddMeshInstance({ &sphere, &metalMaterial });

    auto t = sphereInstance.Transformation();
    t.Rotation = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0)));
    sphereInstance.SetTransformation(t);

    PathFinder::Camera& camera = scene.MainCamera();
    camera.SetFarPlane(1000);
    camera.SetNearPlane(1);
    camera.MoveTo({ 0.0, 0.0f, 25.f });
    camera.LookAt({ 0.f, 0.0f, 0.f });
    camera.SetViewportAspectRatio(16.0f / 9.0f);

    input.SetInvertVerticalDelta(true);

    // ---------------------------------------------------------------------------- //

    sceneStorage.UploadMeshes(scene.Meshes());

    engine.ScheduleAndAllocatePipelineResources();
    engine.UploadProcessAndTransferAssets();

    engine.PreRenderEvent() += { "Engine.Pre.Render", [&]()
    {
        uiStorage.StartNewFrame();
        ImGui::ShowDemoWindow();
        uiStorage.UploadUI();

        sceneStorage.ClearMeshInstanceTable();
        sceneStorage.ClearLightInstanceTable();
        sceneStorage.UploadMeshInstances(scene.MeshInstances());
        sceneStorage.UploadLights(scene.FlatLights());

        PathFinder::GlobalRootConstants globalConstants;
        PathFinder::PerFrameRootConstants perFrameConstants;

        globalConstants.PipelineRTResolution = { 
            engine.RenderSurface().Dimensions().Width, 
            engine.RenderSurface().Dimensions().Height 
        };

        const PathFinder::Camera& camera = scene.MainCamera();

        perFrameConstants.CameraPosition = glm::vec4{ camera.Position(), 1.0 };
        perFrameConstants.CameraView = camera.View();
        perFrameConstants.CameraProjection = camera.Projection();
        perFrameConstants.CameraViewProjection = camera.ViewProjection();
        perFrameConstants.CameraInverseView = camera.InverseView();
        perFrameConstants.CameraInverseProjection = camera.InverseProjection();
        perFrameConstants.CameraInverseViewProjection = camera.InverseViewProjection();

        engine.SetGlobalRootConstants(globalConstants);
        engine.SetFrameRootConstants(perFrameConstants);
    }};

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);

            windowsInputHandler.HandleMessage(msg);
            continue;
        }

        RECT clientRect{};
        if (::GetClientRect(hwnd, &clientRect))
        {
            // 'Top' is screen bottom
            Geometry::Dimensions viewportSize(
                clientRect.right - clientRect.left,
                clientRect.bottom - clientRect.top);

            uiInteractor.SetViewportSize(viewportSize);
            cameraInteractor.SetViewportSize(viewportSize);
        }

        uiInteractor.PollInputs();
        cameraInteractor.PollInputs();
        engine.Render();
        windowsInputHandler.EndFrame();
    }

    engine.FlushAllQueuedFrames();

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}