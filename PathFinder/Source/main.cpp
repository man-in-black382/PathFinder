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
#include "RenderPipeline/RenderPasses/BackBufferOutputPass.hpp"
#include "RenderPipeline/RenderPasses/ShadingRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserPreBlurRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserMipGenerationRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserReprojectionRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserGradientConstructionRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserHistoryFixRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserPostStabilizationRenderPass.hpp"
#include "RenderPipeline/RenderPasses/SpecularDenoiserRenderPass.hpp"
#include "RenderPipeline/RenderPasses/ToneMappingRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DisplacementDistanceMapRenderPass.hpp"
#include "RenderPipeline/RenderPasses/UIRenderPass.hpp"
#include "RenderPipeline/RenderPasses/CommonSetupRenderPass.hpp"
#include "RenderPipeline/RenderPasses/BloomBlurRenderPass.hpp"
#include "RenderPipeline/RenderPasses/BloomCompositionRenderPass.hpp"
#include "RenderPipeline/GlobalRootConstants.hpp"
#include "RenderPipeline/PerFrameRootConstants.hpp"
#include "RenderPipeline/RenderPassContentMediator.hpp"

#include "IO/CommandLineParser.hpp"
#include "IO/Input.hpp"
#include "IO/InputHandlerWindows.hpp"

#include "../resource.h"

#include "../Foundation/Halton.hpp"
#include "../Foundation/StringUtils.hpp"

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
    PathFinder::Scene scene{ cmdLineParser.ExecutableFolderPath(), engine.ResourceProducer() };
    PathFinder::SceneGPUStorage sceneStorage{ &scene, engine.Device(), engine.ResourceProducer() };
    PathFinder::UIGPUStorage uiStorage{ engine.ResourceProducer() };
    PathFinder::Input input{};
    PathFinder::InputHandlerWindows windowsInputHandler{ &input, hwnd };
    PathFinder::UIInteractor uiInteractor{ hwnd, &input };
    PathFinder::CameraInteractor cameraInteractor{ &scene.MainCamera(), &input };
    PathFinder::MeshLoader meshLoader{ cmdLineParser.ExecutableFolderPath() / "MediaResources/Models/" };
    PathFinder::MaterialLoader materialLoader{ cmdLineParser.ExecutableFolderPath(), engine.AssetStorage(), engine.ResourceProducer() };
    PathFinder::RenderPassContentMediator contentMediator{ &uiStorage, &sceneStorage, &scene };

    auto commonSetupPass = std::make_unique<PathFinder::CommonSetupRenderPass>();
    auto GBufferPass = std::make_unique<PathFinder::GBufferRenderPass>();
    auto shadingPass = std::make_unique<PathFinder::ShadingRenderPass>();
    auto denoiserPreBlurPass = std::make_unique<PathFinder::DenoiserPreBlurRenderPass>();
    auto denoiserMipGenerationPass = std::make_unique<PathFinder::DenoiserMipGenerationRenderPass>();
    auto denoiserReprojectionPass = std::make_unique<PathFinder::DenoiserReprojectionRenderPass>();
    auto denoiserGradientConstructionPass = std::make_unique<PathFinder::DenoiserGradientConstructionRenderPass>();
    auto denoiserHistoryFixPass = std::make_unique<PathFinder::DenoiserHistoryFixRenderPass>();
    auto denoiserPostStabilizationPass = std::make_unique<PathFinder::DenoiserPostStabilizationRenderPass>();
    auto specularDenoiserPass = std::make_unique<PathFinder::SpecularDenoiserRenderPass>();
    auto bloomBlurPass = std::make_unique<PathFinder::BloomBlurRenderPass>();
    auto bloomCompositionPass = std::make_unique<PathFinder::BloomCompositionRenderPass>();
    auto toneMappingPass = std::make_unique<PathFinder::ToneMappingRenderPass>();
    auto backBufferOutputPass = std::make_unique<PathFinder::BackBufferOutputPass>();
    auto uiPass = std::make_unique<PathFinder::UIRenderPass>();

    engine.SetContentMediator(&contentMediator);
    engine.AddRenderPass(commonSetupPass.get());
    engine.AddRenderPass(GBufferPass.get());
    engine.AddRenderPass(shadingPass.get());
    engine.AddRenderPass(denoiserPreBlurPass.get());
    engine.AddRenderPass(denoiserMipGenerationPass.get());
    engine.AddRenderPass(denoiserReprojectionPass.get());
    engine.AddRenderPass(denoiserGradientConstructionPass.get());
    engine.AddRenderPass(denoiserHistoryFixPass.get());
    engine.AddRenderPass(specularDenoiserPass.get());
    engine.AddRenderPass(denoiserPostStabilizationPass.get());
    //engine.AddRenderPass(bloomBlurPass.get());
    //engine.AddRenderPass(bloomCompositionPass.get());
    engine.AddRenderPass(toneMappingPass.get());
    engine.AddRenderPass(backBufferOutputPass.get());
    engine.AddRenderPass(uiPass.get());

   /* auto sphereLight0 = scene.EmplaceSphericalLight();
    sphereLight0->SetRadius(7);
    sphereLight0->SetPosition({ 2.5, 0.0, 0.0 });
    sphereLight0->SetColor({ 201.0 / 255, 226.0 / 255, 255.0 / 255 });
    sphereLight0->SetLuminousPower(50000);*/

    //auto sphereLight1 = scene.EmplaceSphericalLight();
    //sphereLight1->SetRadius(2);
    //sphereLight1->SetPosition({ -2.5, 0.0, 0.0 });
    //sphereLight1->SetColor({ 201.0 / 255, 226.0 / 255, 255.0 / 255 });
    //sphereLight1->SetLuminousPower(50000);

    /*auto sphereLight2 = scene.EmplaceSphericalLight();
    sphereLight2->SetRadius(2);
    sphereLight2->SetPosition({ 7.5, 0.0, 0.0 });
    sphereLight2->SetColor({ 50.0 / 255, 50.0 / 255, 255.0 / 255 });
    sphereLight2->SetLuminousPower(50000);

    auto sphereLight3 = scene.EmplaceSphericalLight();
    sphereLight3->SetRadius(2);
    sphereLight3->SetPosition({ -7.5, 0.0, 0.0 });
    sphereLight3->SetColor({ 201.0 / 255, 226.0 / 255, 50.0 / 255 });
    sphereLight3->SetLuminousPower(50000);*/

   /* auto flatLight0 = scene.EmplaceRectangularLight();
    flatLight0->SetWidth(2);
    flatLight0->SetHeight(2);
    flatLight0->SetPosition({ 7.5, 0.0, 0.0 });
    flatLight0->SetNormal({ 0.0, .0, 1.0 });
    flatLight0->SetColor({ 249.0 / 255, 215.0 / 255, 28.0 / 255 });
    flatLight0->SetLuminousPower(5000);

    auto flatLight2 = scene.EmplaceRectangularLight();
    flatLight2->SetWidth(2);
    flatLight2->SetHeight(2);
    flatLight2->SetPosition({ -7.5, 0.0, 0.0 });
    flatLight2->SetNormal({ 0.0, .0, 1.0 });
    flatLight2->SetColor({ 201.0 / 255, 125.0 / 255, 255.0 / 255 });
    flatLight2->SetLuminousPower(5000);*/

  /*  auto flatLight1 = scene.EmplaceRectangularLight();
    flatLight1->SetWidth(2);
    flatLight1->SetHeight(2);
    flatLight1->SetPosition({ -2.5, 0.0, 0.0 });
    flatLight1->SetNormal({ 0.0, .0, -1.0 });
    flatLight1->SetColor({ 200.0 / 255, 50.0 / 255, 50.0 / 255 });
    flatLight1->SetLuminousPower(10000);*/

    auto flatLight3 = scene.EmplaceRectangularLight();
    flatLight3->SetWidth(2);
    flatLight3->SetHeight(2);
    flatLight3->SetPosition({ 0, 2, 0.0 });
    flatLight3->SetNormal(glm::normalize(glm::vec3{ 0.0, -1.0, -1.0 }));
    flatLight3->SetColor({ 255 / 255, 255 / 255, 255 / 255 });
    flatLight3->SetLuminousPower(10000);

    PathFinder::Material& metalMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/Metal07/Metal07_col.dds",
        "/MediaResources/Textures/Metal07/Metal07_nrm.dds", 
        "/MediaResources/Textures/Metal07/Metal07_rgh.dds",
        "/MediaResources/Textures/Metal07/Metal07_met.dds",
        "/MediaResources/Textures/Metal07/Metal07_disp.dds"));

    //PathFinder::Material& harshBricksMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
    //    "/MediaResources/Textures/HarshBricks/harshbricks-albedo.dds",
    //    "/MediaResources/Textures/HarshBricks/harshbricks-normal.dds", 
    //    "/MediaResources/Textures/HarshBricks/harshbricks-roughness.dds",
    //    "/MediaResources/Textures/HarshBricks/harshbricks-metalness.dds", 
    //    "/MediaResources/Textures/HarshBricks/harshbricks-height.dds"));

    PathFinder::Material& concrete19Material = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/Concrete19/Concrete19_col.dds",
        "/MediaResources/Textures/Concrete19/Concrete19_nrm.dds",
        "/MediaResources/Textures/Concrete19/Concrete19_rgh.dds",
        std::nullopt,
        "/MediaResources/Textures/Concrete19/Concrete19_disp.dds"));

    PathFinder::Mesh& plane = scene.AddMesh(std::move(meshLoader.Load("plane.obj").back()));
    PathFinder::MeshInstance& planeInstance = scene.AddMeshInstance({ &plane, &metalMaterial });

    PathFinder::Mesh& cube = scene.AddMesh(std::move(meshLoader.Load("cube.obj").back()));
    PathFinder::MeshInstance& cubeInstance = scene.AddMeshInstance({ &cube, &metalMaterial });

    auto t = planeInstance.Transformation();
    //t.Rotation = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0)));
    t.Translation = glm::vec3{ 0.0, -2.50207 - 1, 0.0 };
    planeInstance.SetTransformation(t);

    t = cubeInstance.Transformation();
    t.Rotation = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    t.Translation = glm::vec3{ 0.0, 1.0, -4.0 };
    cubeInstance.SetTransformation(t);

    PathFinder::Camera& camera = scene.MainCamera();
    camera.SetFarPlane(80);
    camera.SetNearPlane(0.1);
    camera.MoveTo({ 0.0, 2.0f, -25.f });
    camera.LookAt({ 0.f, 4.0f, 0.f });
    camera.SetViewportAspectRatio(16.0f / 9.0f);
    camera.SetAperture(0.8);
    camera.SetFilmSpeed(400);
    camera.SetShutterTime(1.0 / 125.0);

    input.SetInvertVerticalDelta(true);

    // ---------------------------------------------------------------------------- //

    sceneStorage.UploadMeshes();
    sceneStorage.UploadMaterials();

    engine.UploadProcessAndTransferAssets();

    // Build bottom AS only once. A production-level application would build every frame
    for (const PathFinder::BottomRTAS& bottomRTAS : sceneStorage.BottomAccelerationStructures())
    {
        engine.AddBottomRayTracingAccelerationStructure(&bottomRTAS);
    }

    PathFinder::GlobalRootConstants globalConstants;
    PathFinder::PerFrameRootConstants perFrameConstants;

    engine.PreRenderEvent() += { "Engine.Pre.Render", [&]()
    {
        uiStorage.StartNewFrame();
        //ImGui::ShowDemoWindow();
        uiStorage.UploadUI();

        sceneStorage.UploadMeshInstances();
        sceneStorage.UploadLights();

        // Top RT needs to be rebuilt every frame
        engine.AddTopRayTracingAccelerationStructure(&sceneStorage.TopAccelerationStructure());

        globalConstants.PipelineRTResolution = { 
            engine.RenderSurface().Dimensions().Width, 
            engine.RenderSurface().Dimensions().Height 
        };

        globalConstants.PipelineRTResolutionInverse = 1.0f / globalConstants.PipelineRTResolution;

        perFrameConstants.PreviousFrameCamera = perFrameConstants.CurrentFrameCamera;
        perFrameConstants.CurrentFrameCamera = sceneStorage.CameraGPURepresentation();

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
        cameraInteractor.PollInputs(engine.FrameDurationMicroseconds());
        engine.Render();
        windowsInputHandler.EndFrame();
    }

    engine.FlushAllQueuedFrames();

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}