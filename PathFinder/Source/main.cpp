#pragma once

#include <windows.h>
#include <tchar.h>

#include "Scene/Scene.hpp"
#include "Scene/MeshLoader.hpp"
#include "Scene/MaterialLoader.hpp"
#include "Scene/UIInteractor.hpp"
#include "Scene/CameraInteractor.hpp"

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

    PathFinder::Material& bambooWoodMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/BambooWood/bamboo-wood-semigloss-albedo.dds",
        "/MediaResources/Textures/BambooWood/bamboo-wood-semigloss-normal.dds",
        "/MediaResources/Textures/BambooWood/bamboo-wood-semigloss-roughness.dds"));

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

    PathFinder::Material& synthRubberMaterial = scene.AddMaterial(materialLoader.LoadMaterial(
        "/MediaResources/Textures/SynthRubber/synth-rubber-albedo.dds",
        "/MediaResources/Textures/SynthRubber/synth-rubber-normal.dds",
        "/MediaResources/Textures/SynthRubber/synth-rubber-roughness.dds"));

    PathFinder::Mesh& plane = scene.AddMesh(std::move(meshLoader.Load("plane.obj").back()));

    for (float x = -200; x < 200; x += 20)
    {
        for (float z = -200; z < 200; z += 20)
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
   



    //auto flatLight3 = scene.EmplaceRectangularLight();
    //flatLight3->SetWidth(100);
    //flatLight3->SetHeight(40);
    //flatLight3->SetPosition({ -33.1, 21, 25.6 });
    ////flatLight3->SetNormal(-glm::normalize(glm::vec3{ -30.6, 6, 23 }));
    //flatLight3->SetColor(light2Color);
    //flatLight3->SetLuminousPower(200000);

    //auto dir = glm::normalize(glm::vec3{ -23.9, 6, 15.15 } - flatLight3->Position());
    //dir.y = -0.5;
    //dir = glm::normalize(dir);
    //flatLight3->SetNormal(dir);

     /*   auto flatLight4 = scene.EmplaceRectangularLight();
        flatLight4->SetWidth(10);
        flatLight4->SetHeight(5);
        flatLight4->SetPosition({ -4.5, 3, -30.0 });
        flatLight4->SetNormal(glm::normalize(glm::vec3{ 1, -0.5, 1.0 }));
        flatLight4->SetColor({ 216.0 / 255, 247.0 / 255, 255.0 / 255 });
        flatLight4->SetLuminousPower(20000);*/

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

    choreograph::Timeline animationTimeline{};
    choreograph::Output<float> rotationOutput;

    choreograph::PhraseRef<float> rotationPhrase = choreograph::makeRamp(0.0f, 2.0f * 3.1415f, 4.f);
    animationTimeline.apply(&rotationOutput, rotationPhrase).finishFn(
        [&m = *rotationOutput.inputPtr()]
        {
            m.resetTime();
        }
    );

    float light0Power = 300000;
    float light1Power = 300000;
    float light2Power = 300000;
    float light3Power = 300000;

    // Animate lights
    choreograph::Output<float> light0PowerOutput;
    animationTimeline.apply(&light0PowerOutput).set(0.0f).holdUntil(1.0).rampTo(light0Power, 1.0f).holdUntil(28.f).rampTo(0.0f, 1.0f, choreograph::EaseOutQuad());

    choreograph::Output<float> light1PowerOutput;
    animationTimeline.apply(&light1PowerOutput).set(0.0f).holdUntil(2.0).rampTo(light1Power, 1.0f).holdUntil(29.f).rampTo(0.0f, 1.0f, choreograph::EaseOutQuad());

    choreograph::Output<float> light2PowerOutput;
    animationTimeline.apply(&light2PowerOutput).set(0.0f).holdUntil(3.0).rampTo(light2Power, 1.0f).holdUntil(30.f).rampTo(0.0f, 1.0f, choreograph::EaseOutQuad());

    choreograph::Output<float> light3PowerOutput;
    animationTimeline.apply(&light3PowerOutput).set(0.0f).holdUntil(4.0).rampTo(light3Power, 2.0f).holdUntil(31.f).rampTo(0.0f, 1.0f, choreograph::EaseOutQuad());

    //choreograph::Output<float> lightRotationOutput;
    //animationTimeline.apply(&lightRotationOutput).set(0.0f).rampTo(glm::pi<float>() * 2.0f, 2.0f).finishFn([&] {lightRotationOutput.inputPtr()->resetTime(); });

    choreograph::Output<float> lightSizeOutput;
    animationTimeline.apply(&lightSizeOutput).set(0.2f).holdUntil(10).rampTo(1.0f, 12.0f, choreograph::EaseOutQuad());

    // Light positions
    choreograph::Output<glm::vec3> light0PositionOutput;
    animationTimeline.apply(&light0PositionOutput).set({ -57, 10, 2 }).rampTo({ 7, 15, 36 }, 1.5f, choreograph::EaseInOutSine()).finishFn([&] {
        light0PositionOutput.inputPtr()->setPlaybackSpeed(light0PositionOutput.inputPtr()->getPlaybackSpeed() * -1);
        light0PositionOutput.inputPtr()->resetTime(); 
        });

    choreograph::Output<glm::vec3> light1PositionOutput;
    animationTimeline.apply(&light1PositionOutput).set({ 20,23,37 }).rampTo({ -68, 9, -13 }, 2.0f, choreograph::EaseInOutSine()).finishFn([&] {
        light1PositionOutput.inputPtr()->setPlaybackSpeed(light1PositionOutput.inputPtr()->getPlaybackSpeed() * -1);
        light1PositionOutput.inputPtr()->resetTime(); 
        });

    choreograph::Output<glm::vec3> light2PositionOutput;
    animationTimeline.apply(&light2PositionOutput).set({ 33,2.5,8 }).rampTo({ -35, 17, 31 }, 2.0f, choreograph::EaseInOutSine()).finishFn([&] {
        light2PositionOutput.inputPtr()->setPlaybackSpeed(light2PositionOutput.inputPtr()->getPlaybackSpeed() * -1);
        light2PositionOutput.inputPtr()->resetTime();
        });

    choreograph::Output<glm::vec3> light3PositionOutput;
    animationTimeline.apply(&light3PositionOutput).set({ -49, 7, -2.6 }).rampTo({ 30, 5, 30 }, 2.0f, choreograph::EaseInOutSine()).finishFn([&] {
        light3PositionOutput.inputPtr()->setPlaybackSpeed(light3PositionOutput.inputPtr()->getPlaybackSpeed() * -1);
        light3PositionOutput.inputPtr()->resetTime();
        });

    glm::vec3 cameraStartPos{ 44.8, 8.6, -106.0 };
    glm::vec3 cameraEndPos{ 3.83, 6.7, -23 };
    choreograph::Output<glm::vec3> cameraPosOutput;
    choreograph::PhraseRef<glm::vec3> cameraPosPhrase = choreograph::makeRamp(cameraStartPos, cameraEndPos, 34.f);
    animationTimeline.apply(&cameraPosOutput, cameraPosPhrase).finishFn(
        [&m = *cameraPosOutput.inputPtr()]
        {
            //m.setPlaybackSpeed(m.getPlaybackSpeed() * -1);
            m.resetTime();
        }
        );

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
            /*camera.MoveTo(cameraPosOutput.value());
            camera.LookAt(glm::vec3{ -6.17, 4.58, -4.66 });*/
            //sphereLight0->SetLuminousPower(light0PowerOutput.value());
            //sphereLight1->SetLuminousPower(light1PowerOutput.value());
            //sphereLight2->SetLuminousPower(light2PowerOutput.value());
            //sphereLight3->SetLuminousPower(light3PowerOutput.value());
            //flatLight3->SetLuminousPower(light3PowerOutput.value());
            //flatLight4->SetLuminousPower(light3PowerOutput.value());

        /*    sphereLight0->SetPosition(light0PositionOutput.value());
            sphereLight1->SetPosition(light1PositionOutput.value());
            sphereLight2->SetPosition(light2PositionOutput.value());
            sphereLight3->SetPosition(light3PositionOutput.value());*/

          /*  flatLight3->SetWidth(40.0 * lightSizeOutput.value());
            flatLight3->SetHeight(20.0 * lightSizeOutput.value());*/

        /*t = cubeInstance.Transformation();
        t.Rotation = glm::angleAxis(rotationOutput.value(), glm::normalize(glm::vec3{ 1.f, 0.f, 1.f }));
        cubeInstance.SetTransformation(t);*/

            t = sphereType1Instance0.Transformation();
            t.Rotation = glm::angleAxis(rotationOutput.value(), glm::normalize(glm::vec3{ 0.f, 1.f, 0.f }));
            sphereType1Instance0.SetTransformation(t);

       /* glm::vec3 rotCenter{ -5.65, 9.0, -4.6 };
        glm::vec3 rotated = glm::rotate(glm::vec3{ 1.0, 0.0, 0.0 }, lightRotationOutput.value(), glm::vec3(0.0, 1.0, 0.0));*/

        //sphereLight1->SetPosition(rotCenter + rotated * 4.0f);

       /* t = sphereType2Instance0.Transformation();
        t.Rotation = glm::angleAxis(rotationOutput.value(), glm::normalize(glm::vec3{ 0.f, 1.f, 0.f }));
        sphereType2Instance0.SetTransformation(t);*/
    /*    flatLight3->SetWidth(lightSizeOutput.value());
        flatLight3->SetHeight(lightSizeOutput.value());*/

        settingsContainer.ApplyVolatileSettings();
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

    ::ShowCursor(false);

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
        animationTimeline.step(1.0 / 60.0);

        IC(camera.Position().x, camera.Position().y, camera.Position().z);
    }

    engine.FlushAllQueuedFrames();

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}