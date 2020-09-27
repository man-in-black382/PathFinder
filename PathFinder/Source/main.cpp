#pragma once

#include <windows.h>
#include <tchar.h>

#include "Scene/Scene.hpp"
#include "Scene/MeshLoader.hpp"
#include "Scene/MaterialLoader.hpp"
#include "Scene/UIInteractor.hpp"
#include "Scene/CameraInteractor.hpp"

#include "UI/SceneManipulatorViewController.hpp"

#include "RenderPipeline/RenderEngine.hpp"
#include "RenderPipeline/RenderSettings.hpp"
#include "RenderPipeline/RenderPasses/GBufferRenderPass.hpp"
#include "RenderPipeline/RenderPasses/BackBufferOutputPass.hpp"
#include "RenderPipeline/RenderPasses/RngSeedGenerationRenderPass.hpp"
#include "RenderPipeline/RenderPasses/ShadingRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserPreBlurRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserMipGenerationRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserReprojectionRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserForwardProjectionRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserGradientConstructionRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserGradientFilteringRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserHistoryFixRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserPostStabilizationRenderPass.hpp"
#include "RenderPipeline/RenderPasses/DenoiserPostBlurRenderPass.hpp"
#include "RenderPipeline/RenderPasses/SpecularDenoiserRenderPass.hpp"
#include "RenderPipeline/RenderPasses/SMAAEdgeDetectionRenderPass.hpp"
#include "RenderPipeline/RenderPasses/SMAABlendingWeightCalculationRenderPass.hpp"
#include "RenderPipeline/RenderPasses/SMAANeighborhoodBlendingRenderPass.hpp"
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

#include "choreograph/Choreograph.h"

#include "../resource.h"

#include "../Foundation/Halton.hpp"
#include "../Foundation/StringUtils.hpp"

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <glm/common.hpp>
#include <glm/gtx/rotate_vector.hpp>

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
    ::SetWindowPos(hwnd, 0, 0, 0, 1920, 1080, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    PathFinder::CommandLineParser cmdLineParser{ argc, argv };
    PathFinder::RenderEngine<PathFinder::RenderPassContentMediator> engine{ hwnd, cmdLineParser };
    PathFinder::Scene scene{ cmdLineParser.ExecutableFolderPath(), engine.ResourceProducer() };
    PathFinder::SceneGPUStorage sceneStorage{ &scene, engine.Device(), engine.ResourceProducer() };
    PathFinder::UIGPUStorage uiStorage{ engine.ResourceProducer() };
    PathFinder::Input input{};
    PathFinder::RenderSettingsController settingsContainer{ &input };
    PathFinder::InputHandlerWindows windowsInputHandler{ &input, hwnd };
    PathFinder::UIInteractor uiInteractor{ hwnd, &input };
    PathFinder::CameraInteractor cameraInteractor{ &scene.MainCamera(), &input };
    PathFinder::MeshLoader meshLoader{ cmdLineParser.ExecutableFolderPath() / "MediaResources/Models/" };
    PathFinder::MaterialLoader materialLoader{ cmdLineParser.ExecutableFolderPath(), engine.AssetStorage(), engine.ResourceProducer() };
    PathFinder::RenderPassContentMediator contentMediator{ &uiStorage, &sceneStorage, &scene, &settingsContainer };

    auto commonSetupPass = std::make_unique<PathFinder::CommonSetupRenderPass>();
    auto GBufferPass = std::make_unique<PathFinder::GBufferRenderPass>();
    auto rgnSeedGenerationPass = std::make_unique<PathFinder::RngSeedGenerationRenderPass>();
    auto shadingPass = std::make_unique<PathFinder::ShadingRenderPass>();
    auto denoiserPreBlurPass = std::make_unique<PathFinder::DenoiserPreBlurRenderPass>();
    auto denoiserMipGenerationPass = std::make_unique<PathFinder::DenoiserMipGenerationRenderPass>();
    auto denoiserReprojectionPass = std::make_unique<PathFinder::DenoiserReprojectionRenderPass>();
    auto denoiserGradientConstructionPass = std::make_unique<PathFinder::DenoiserGradientConstructionRenderPass>();
    auto denoiserGradientFilteringPass = std::make_unique<PathFinder::DenoiserGradientFilteringRenderPass>();
    auto denoiserForwardProjectionPass = std::make_unique<PathFinder::DenoiserForwardProjectionRenderPass>();
    auto denoiserHistoryFixPass = std::make_unique<PathFinder::DenoiserHistoryFixRenderPass>();
    auto denoiserPostStabilizationPass = std::make_unique<PathFinder::DenoiserPostStabilizationRenderPass>();
    auto denoiserPostBlurPass = std::make_unique<PathFinder::DenoiserPostBlurRenderPass>();
    auto specularDenoiserPass = std::make_unique<PathFinder::SpecularDenoiserRenderPass>();
    auto bloomBlurPass = std::make_unique<PathFinder::BloomBlurRenderPass>();
    auto bloomCompositionPass = std::make_unique<PathFinder::BloomCompositionRenderPass>();
    auto toneMappingPass = std::make_unique<PathFinder::ToneMappingRenderPass>();
    auto SMAAEdgeDetectionPass = std::make_unique<PathFinder::SMAAEdgeDetectionRenderPass>();
    auto SMAABlendingWeightCalculationPass = std::make_unique<PathFinder::SMAABlendingWeightCalculationRenderPass>();
    auto SMAANeighborhoodBlendingPass = std::make_unique<PathFinder::SMAANeighborhoodBlendingRenderPass>();
    auto backBufferOutputPass = std::make_unique<PathFinder::BackBufferOutputPass>();
    auto uiPass = std::make_unique<PathFinder::UIRenderPass>();

    engine.SetContentMediator(&contentMediator);
    engine.AddRenderPass(commonSetupPass.get());
    engine.AddRenderPass(GBufferPass.get());
    engine.AddRenderPass(rgnSeedGenerationPass.get());
    engine.AddRenderPass(shadingPass.get());
    engine.AddRenderPass(denoiserPreBlurPass.get());
    engine.AddRenderPass(denoiserMipGenerationPass.get());
    engine.AddRenderPass(denoiserReprojectionPass.get());
    engine.AddRenderPass(denoiserGradientConstructionPass.get());
    engine.AddRenderPass(denoiserGradientFilteringPass.get());
    engine.AddRenderPass(denoiserForwardProjectionPass.get());
    engine.AddRenderPass(denoiserHistoryFixPass.get());
    engine.AddRenderPass(specularDenoiserPass.get());
    engine.AddRenderPass(denoiserPostStabilizationPass.get());
    engine.AddRenderPass(denoiserPostBlurPass.get());
    engine.AddRenderPass(bloomBlurPass.get());
    engine.AddRenderPass(bloomCompositionPass.get());
    engine.AddRenderPass(SMAAEdgeDetectionPass.get());
    engine.AddRenderPass(SMAABlendingWeightCalculationPass.get());
    engine.AddRenderPass(SMAANeighborhoodBlendingPass.get());
    engine.AddRenderPass(toneMappingPass.get());
    engine.AddRenderPass(backBufferOutputPass.get());
    engine.AddRenderPass(uiPass.get());

    PathFinder::Material& metalMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/Metal07/Metal07_col.dds",
        "/MediaResources/Textures/Metal07/Metal07_nrm.dds",
        "/MediaResources/Textures/Metal07/Metal07_rgh.dds",
        "/MediaResources/Textures/Metal07/Metal07_met.dds"));

    PathFinder::Material& concrete19Material = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/Concrete19/Concrete19_col.dds",
        "/MediaResources/Textures/Concrete19/Concrete19_nrm.dds",
        "/MediaResources/Textures/Concrete19/Concrete19_rgh.dds"));

    PathFinder::Material& charcoalMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/Charcoal/charcoal-albedo2.dds",
        "/MediaResources/Textures/Charcoal/charcoal-normal.dds",
        "/MediaResources/Textures/Charcoal/charcoal-roughness.dds"));

    PathFinder::Material& grimyMetalMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/GrimyMetal/grimy-metal-albedo.dds",
        "/MediaResources/Textures/GrimyMetal/grimy-metal-normal-dx.dds",
        "/MediaResources/Textures/GrimyMetal/grimy-metal-roughness.dds",
        "/MediaResources/Textures/GrimyMetal/grimy-metal-metalness.dds"));

    PathFinder::Material& marble006Material = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/Marble006/Marble006_4K_Color.dds",
        "/MediaResources/Textures/Marble006/Marble006_4K_Normal.dds",
        "/MediaResources/Textures/Marble006/Marble006_4K_Roughness.dds"));

    PathFinder::Material& marbleTilesMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/MarbleTiles/Marble_tiles_02_4K_Base_Color.dds",
        "/MediaResources/Textures/MarbleTiles/Marble_tiles_02_4K_Normal.dds",
        "/MediaResources/Textures/MarbleTiles/Marble_tiles_02_4K_Roughness.dds"));

    PathFinder::Material& redPlasticMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/RedPlastic/plasticpattern1-albedo.dds",
        "/MediaResources/Textures/RedPlastic/plasticpattern1-normal2b.dds",
        "/MediaResources/Textures/RedPlastic/plasticpattern1-roughness2.dds"));

    PathFinder::Material& rustedIronMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/RustedIron/rustediron2_basecolor.dds",
        "/MediaResources/Textures/RustedIron/rustediron2_normal.dds",
        "/MediaResources/Textures/RustedIron/rustediron2_roughness.dds",
        "/MediaResources/Textures/RustedIron/rustediron2_metallic.dds"));

    PathFinder::Material& scuffedTitamiumMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/ScuffedTitanium/Titanium-Scuffed_basecolor.dds",
        "/MediaResources/Textures/ScuffedTitanium/Titanium-Scuffed_normal.dds",
        "/MediaResources/Textures/ScuffedTitanium/Titanium-Scuffed_roughness.dds",
        "/MediaResources/Textures/ScuffedTitanium/Titanium-Scuffed_metallic.dds"));

    PathFinder::Mesh& plane = scene.AddMesh(std::move(meshLoader.Load("plane.obj").back()));

    for (float x = -100; x < 100; x += 20)
    {
        for (float z = -100; z < 100; z += 20)
        {
            PathFinder::MeshInstance& planeInstance = scene.AddMeshInstance({ &plane, &scuffedTitamiumMaterial });
            Geometry::Transformation t;
            t.Translation = glm::vec3{ x, -3.50207, z };
            planeInstance.SetTransformation(t);
        }
    }

    PathFinder::Mesh& cube = scene.AddMesh(std::move(meshLoader.Load("cube.obj").back()));
    PathFinder::MeshInstance& cubeInstance = scene.AddMeshInstance({ &cube, &marbleTilesMaterial });

    PathFinder::Mesh& sphereType1 = scene.AddMesh(std::move(meshLoader.Load("sphere1.obj").back()));
    PathFinder::MeshInstance& sphereType1Instance0 = scene.AddMeshInstance({ &sphereType1, &marbleTilesMaterial });

    PathFinder::Mesh& sphereType2 = scene.AddMesh(std::move(meshLoader.Load("sphere2.obj").back()));
    PathFinder::MeshInstance& sphereType2Instance0 = scene.AddMeshInstance({ &sphereType2, &marbleTilesMaterial });

    PathFinder::Mesh& sphereType3 = scene.AddMesh(std::move(meshLoader.Load("sphere3.obj").back()));
    PathFinder::MeshInstance& sphereType3Instance0 = scene.AddMeshInstance({ &sphereType3, &grimyMetalMaterial });
    PathFinder::MeshInstance& sphereType3Instance1 = scene.AddMeshInstance({ &sphereType3, &redPlasticMaterial });
    PathFinder::MeshInstance& sphereType3Instance2 = scene.AddMeshInstance({ &sphereType3, &marble006Material });
    PathFinder::MeshInstance& sphereType3Instance3 = scene.AddMeshInstance({ &sphereType3, &charcoalMaterial });
    PathFinder::MeshInstance& sphereType3Instance4 = scene.AddMeshInstance({ &sphereType3, &concrete19Material });
    PathFinder::MeshInstance& sphereType3Instance5 = scene.AddMeshInstance({ &sphereType3, &metalMaterial });

    Geometry::Transformation t = cubeInstance.Transformation();
    t.Rotation = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    t.Translation = glm::vec3{ -4.88, 3.25, -3.42 };
    t.Scale = glm::vec3{ 0/*2.0f*/ };
    cubeInstance.SetTransformation(t);

    //t.Scale = glm::vec3{ 0.1f }; // Large version
    //t.Translation = glm::vec3{ 0.0, -2, -4.0 }; // Large version
    t.Scale = glm::vec3{ 0.11f };
    t.Translation = glm::vec3{ -6.0, 8.0, -4.0 };
    sphereType1Instance0.SetTransformation(t);

    // Small sphere
    t.Scale = glm::vec3{ 0/*0.21f*/ };
    t.Translation = glm::vec3{ 6.0, -14.0, -9.0 };
    sphereType2Instance0.SetTransformation(t);

    // Normal spheres
    t.Scale = glm::vec3{ 0.12 };
    t.Translation = glm::vec3{ 9.0, 2.0, -17.5 };
    sphereType3Instance0.SetTransformation(t);

    t.Scale = glm::vec3{ 0.085 };
    t.Translation = glm::vec3{ 8.88, 1.2, 0.1 };
    sphereType3Instance1.SetTransformation(t);

    t.Scale = glm::vec3{ 0.15 };
    t.Translation = glm::vec3{ 3.88, 3, -23.77 };
    sphereType3Instance2.SetTransformation(t);

    t.Scale = glm::vec3{ 0.1 };
    t.Translation = glm::vec3{ -13.2, 1.5, -18.6 };
    sphereType3Instance3.SetTransformation(t);

    t.Scale = glm::vec3{ 0.09 };
    t.Translation = glm::vec3{ -4.07, 1.25, -19.25 };
    sphereType3Instance4.SetTransformation(t);

    t.Scale = glm::vec3{ 0.1 };
    t.Translation = glm::vec3{ 12.47, 1.7, -9.26 };
    sphereType3Instance5.SetTransformation(t);

    Foundation::Color light0Color{ 255.0 / 255, 241.0 / 255, 224.1 / 255 };
    Foundation::Color light1Color{ 64.0 / 255, 156.0 / 255, 255.0 / 255 };
    Foundation::Color light2Color{ 255.0 / 255, 147.0 / 255, 41.0 / 255 };
    Foundation::Color light3Color{ 250.0 / 255, 110.0 / 255, 100.0 / 255 };

    auto sphereLight0 = scene.EmplaceSphericalLight();
    sphereLight0->SetRadius(7);
    sphereLight0->SetPosition({ -5.65, 12.0, -4.6 });
    sphereLight0->SetColor(light0Color);
    sphereLight0->SetLuminousPower(300000);

    auto sphereLight1 = scene.EmplaceSphericalLight();
    sphereLight1->SetRadius(7.5);
    sphereLight1->SetPosition({ -5.65, 12.0, -4.6 });
    sphereLight1->SetColor(light1Color);
    sphereLight1->SetLuminousPower(300000);

    auto sphereLight2 = scene.EmplaceSphericalLight();
    sphereLight2->SetRadius(6);
    sphereLight2->SetPosition({ -5.3, 4.43, -4.76 });
    sphereLight2->SetColor(light2Color);
    sphereLight2->SetLuminousPower(300000);

    auto sphereLight3 = scene.EmplaceSphericalLight();
    sphereLight3->SetRadius(8);
    sphereLight3->SetPosition({ -5.3, 4.43, -4.76 });
    sphereLight3->SetColor(light3Color);
    sphereLight3->SetLuminousPower(300000);

    PathFinder::Camera& camera = scene.MainCamera();
    camera.SetFarPlane(500);
    camera.SetNearPlane(1);
    camera.MoveTo({ 63.65, 6.41, -32.7 });
    camera.LookAt({ 0.f, 0.0f, 0.f });
    camera.SetViewportAspectRatio(16.0f / 9.0f);
    camera.SetAperture(1.2);
    camera.SetFilmSpeed(800);
    camera.SetShutterTime(1.0 / 125.0);
    camera.SetFieldOfView(90);

    input.SetInvertVerticalDelta(true);

    UI::SceneManipulatorViewController sceneManipulatorVC{};
    sceneManipulatorVC.SetCamera(&scene.MainCamera());

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
        settingsContainer.ApplyVolatileSettings();
        uiStorage.StartNewFrame();
        sceneManipulatorVC.Draw();
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
        globalConstants.AnisotropicClampSamplerIdx = engine.ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::AnisotropicClamp)->IndexInHeapRange();
        globalConstants.LinearClampSamplerIdx = engine.ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::LinearClamp)->IndexInHeapRange();
        globalConstants.PointClampSamplerIdx = engine.ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::PointClamp)->IndexInHeapRange();
        globalConstants.MinSamplerIdx = engine.ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::Minimim)->IndexInHeapRange();
        globalConstants.MaxSamplerIdx = engine.ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::Maximum)->IndexInHeapRange();

        perFrameConstants.PreviousFrameCamera = perFrameConstants.CurrentFrameCamera;
        perFrameConstants.CurrentFrameCamera = sceneStorage.CameraGPURepresentation();

        const PathFinder::RenderSettings& settings = settingsContainer.AppliedSettings();

        perFrameConstants.IsDenoiserEnabled = settings.IsDenoiserEnabled;
        perFrameConstants.IsReprojectionHistoryDebugEnabled = settings.IsReprojectionHistoryDebugRenderingEnabled;
        perFrameConstants.IsGradientDebugEnabled = settings.IsDenoiserGradientDebugRenderingEnabled;
        perFrameConstants.IsMotionDebugEnabled = settings.IsDenoiserMotionDebugRenderingEnabled;
        perFrameConstants.IsDenoiserAntilagEnabled = settings.IsDenoiserAntilagEnabled;

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