#include "Application.hpp"

#include "../resource.h"

#include <choreograph/Choreograph.h>
#include <windows.h>
#include <tchar.h>

namespace PathFinder
{

    Application::Application(int argc, char** argv)
    {
        CreateEngineWindow();

        mCmdLineParser = std::make_unique<CommandLineParser>(argc, argv);
        mRenderEngine = std::make_unique<RenderEngine<RenderPassContentMediator>>(mWindowHandle, *mCmdLineParser);
        mScene = std::make_unique<Scene>(mCmdLineParser->ExecutableFolderPath(), mRenderEngine->Device(), mRenderEngine->ResourceProducer());
        mInput = std::make_unique<Input>();
        mSettingsController = std::make_unique<RenderSettingsController>(mInput.get());
        mWindowsInputHandler = std::make_unique<InputHandlerWindows>(mInput.get(), mWindowHandle);
        mCameraInteractor = std::make_unique<CameraInteractor>(&mScene->MainCamera(), mInput.get());
        mDisplaySettingsController = std::make_unique<DisplaySettingsController>(mRenderEngine->SelectedAdapter(), mRenderEngine->SwapChain(), mWindowHandle);
        mUIDependencies = std::make_unique<UIDependencies>(mRenderEngine->ResourceStorage(), &mRenderEngine->PreRenderEvent(), &mRenderEngine->PostRenderEvent(), mRenderEngine->RendererDevice(), mScene.get());
        mUIManager = std::make_unique<UIManager>(mInput.get(), mUIDependencies.get(), mRenderEngine->ResourceProducer());
        mUIEntryPoint = std::make_unique<UIEntryPoint>(mUIManager.get());
        mContentMediator = std::make_unique<RenderPassContentMediator>(&mUIManager->GPUStorage(), &mScene->GPUStorage(), mScene.get(), mInput.get(), mDisplaySettingsController.get(), mSettingsController.get());

        mRenderEngine->SetContentMediator(mContentMediator.get());

        mRenderEngine->PreRenderEvent() += { "Engine.Pre.Render", this, &Application::PerformPreRenderActions };
        mRenderEngine->PostRenderEvent() += { "Engine.Post.Render", this, & Application::PerformPostRenderActions };

        mInput->SetInvertVerticalDelta(true);

        InjectRenderPasses();

        mUIEntryPoint->CreateMandatoryViewControllers();

        // Temporary to load demo Scene until proper UI is implemented
        mMeshLoader = std::make_unique<MeshLoader>(mCmdLineParser->ExecutableFolderPath() / "MediaResources/Models/");
        mMaterialLoader = std::make_unique<MaterialLoader>(mCmdLineParser->ExecutableFolderPath(), mRenderEngine->AssetStorage(), mRenderEngine->ResourceProducer());
        LoadDemoScene();
    }

    void Application::RunMessageLoop()
    {
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        while (msg.message != WM_QUIT)
        {
            while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                mWindowsInputHandler->HandleMessage(msg);
                mDisplaySettingsController->HandleMessage(msg);
            }

            mInput->FinalizeInput();
            mRenderEngine->Render();
            mInput->Clear();
        }
    }

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void Application::CreateEngineWindow()
    {
        HICON iconHandle = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

        // Create application window
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), iconHandle, NULL, NULL, NULL, _T("PathFinder"), NULL };
        RegisterClassEx(&wc);
        auto windowStyle = WS_OVERLAPPEDWINDOW;
        HWND hwnd = CreateWindow(wc.lpszClassName, _T("PathFinder"), windowStyle, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

        // Show the window
        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);
        SetWindowPos(hwnd, 0, 0, 0, 1920, 1080, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        mWindowHandle = hwnd;
        mWindowClass = wc;
    }
    
    void Application::DestroyEngineWindow()
    {
        mRenderEngine->FlushAllQueuedFrames();

        DestroyWindow(mWindowHandle);
        UnregisterClass(mWindowClass.lpszClassName, mWindowClass.hInstance);
    }

    void Application::InjectRenderPasses()
    {
        mRenderEngine->SetContentMediator(mContentMediator.get());
        mRenderEngine->AddRenderPass(&mCommonSetupPass);
        mRenderEngine->AddRenderPass(&mGBufferPass);
        mRenderEngine->AddRenderPass(&mRgnSeedGenerationPass);
        mRenderEngine->AddRenderPass(&mShadingPass);
        mRenderEngine->AddRenderPass(&mDenoiserPreBlurPass);
        mRenderEngine->AddRenderPass(&mDenoiserMipGenerationPass);
        mRenderEngine->AddRenderPass(&mDenoiserReprojectionPass);
        mRenderEngine->AddRenderPass(&mDenoiserGradientConstructionPass);
        mRenderEngine->AddRenderPass(&mDenoiserGradientFilteringPass);
        mRenderEngine->AddRenderPass(&mDenoiserForwardProjectionPass);
        mRenderEngine->AddRenderPass(&mDenoiserHistoryFixPass);
        mRenderEngine->AddRenderPass(&mSpecularDenoiserPass);
        mRenderEngine->AddRenderPass(&mDenoiserPostStabilizationPass);
        mRenderEngine->AddRenderPass(&mDenoiserPostBlurPass);
        mRenderEngine->AddRenderPass(&mBloomBlurPass);
        mRenderEngine->AddRenderPass(&mBloomCompositionPass);
        mRenderEngine->AddRenderPass(&mSMAAEdgeDetectionPass);
        mRenderEngine->AddRenderPass(&mSMAABlendingWeightCalculationPass);
        mRenderEngine->AddRenderPass(&mSMAANeighborhoodBlendingPass);
        mRenderEngine->AddRenderPass(&mToneMappingPass);
        mRenderEngine->AddRenderPass(&mBackBufferOutputPass);
        mRenderEngine->AddRenderPass(&mUIPass);
        mRenderEngine->AddRenderPass(&mGeometryPickingPass);
    }

    void Application::PerformPreRenderActions()
    {
        const Geometry::Dimensions& viewportSize = mRenderEngine->RenderSurface().Dimensions();

        // 'Top' is screen bottom
        mUIManager->SetViewportSize(viewportSize);
        mCameraInteractor->SetViewportSize(viewportSize);
        mUIManager->Draw();

        bool interactingWithUI = mUIManager->IsInteracting();

        mCameraInteractor->SetKeyboardControlsEnabled(!interactingWithUI);
        mCameraInteractor->SetMouseControlsEnabled(!mUIManager->IsMouseOverUI());
        mCameraInteractor->PollInputs(mRenderEngine->FrameDurationUS());

        mSettingsController->SetEnabled(!interactingWithUI);
        mSettingsController->ApplyVolatileSettings();

        mScene->GPUStorage().UploadInstances();
        mScene->RemapEntityIDs();

        // Top RT needs to be rebuilt every frame
        mRenderEngine->AddTopRayTracingAccelerationStructure(&mScene->GPUStorage().TopAccelerationStructure());

        mGlobalConstants.PipelineRTResolution = {
            mRenderEngine->RenderSurface().Dimensions().Width,
            mRenderEngine->RenderSurface().Dimensions().Height
        };

        mGlobalConstants.PipelineRTResolutionInverse = 1.0f / mGlobalConstants.PipelineRTResolution;
        mGlobalConstants.AnisotropicClampSamplerIdx = mRenderEngine->ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::AnisotropicClamp)->IndexInHeapRange();
        mGlobalConstants.LinearClampSamplerIdx = mRenderEngine->ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::LinearClamp)->IndexInHeapRange();
        mGlobalConstants.PointClampSamplerIdx = mRenderEngine->ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::PointClamp)->IndexInHeapRange();
        mGlobalConstants.MinSamplerIdx = mRenderEngine->ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::Minimim)->IndexInHeapRange();
        mGlobalConstants.MaxSamplerIdx = mRenderEngine->ResourceStorage()->GetSamplerDescriptor(PathFinder::SamplerNames::Maximum)->IndexInHeapRange();

        mPerFrameConstants.PreviousFrameCamera = mPerFrameConstants.CurrentFrameCamera;
        mPerFrameConstants.CurrentFrameCamera = mScene->GPUStorage().CameraGPURepresentation();

        const PathFinder::RenderSettings& settings = mSettingsController->AppliedSettings();

        mPerFrameConstants.IsDenoiserEnabled = settings.IsDenoiserEnabled;
        mPerFrameConstants.IsReprojectionHistoryDebugEnabled = settings.IsReprojectionHistoryDebugRenderingEnabled;
        mPerFrameConstants.IsGradientDebugEnabled = settings.IsDenoiserGradientDebugRenderingEnabled;
        mPerFrameConstants.IsMotionDebugEnabled = settings.IsDenoiserMotionDebugRenderingEnabled;
        mPerFrameConstants.IsDenoiserAntilagEnabled = settings.IsDenoiserAntilagEnabled;

        mRenderEngine->SetGlobalRootConstants(mGlobalConstants);
        mRenderEngine->SetFrameRootConstants(mPerFrameConstants);
    }

    void Application::PerformPostRenderActions()
    {
    }

    void Application::LoadDemoScene()
    {
        // This function is temporary until proper scene UI and serialization is implemented 
        //
        

        PathFinder::Material& metalMaterial = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/Metal07/Metal07_col.dds",
            "/MediaResources/Textures/Metal07/Metal07_nrm.dds",
            "/MediaResources/Textures/Metal07/Metal07_rgh.dds",
            "/MediaResources/Textures/Metal07/Metal07_met.dds"));

        PathFinder::Material& concrete19Material = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/Concrete19/Concrete19_col.dds",
            "/MediaResources/Textures/Concrete19/Concrete19_nrm.dds",
            "/MediaResources/Textures/Concrete19/Concrete19_rgh.dds"));

        PathFinder::Material& charcoalMaterial = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/Charcoal/charcoal-albedo2.dds",
            "/MediaResources/Textures/Charcoal/charcoal-normal.dds",
            "/MediaResources/Textures/Charcoal/charcoal-roughness.dds"));

        PathFinder::Material& grimyMetalMaterial = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/GrimyMetal/grimy-metal-albedo.dds",
            "/MediaResources/Textures/GrimyMetal/grimy-metal-normal-dx.dds",
            "/MediaResources/Textures/GrimyMetal/grimy-metal-roughness.dds",
            "/MediaResources/Textures/GrimyMetal/grimy-metal-metalness.dds"));

        PathFinder::Material& marble006Material = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/Marble006/Marble006_4K_Color.dds",
            "/MediaResources/Textures/Marble006/Marble006_4K_Normal.dds",
            "/MediaResources/Textures/Marble006/Marble006_4K_Roughness.dds"));

        PathFinder::Material& marbleTilesMaterial = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/MarbleTiles/Marble_tiles_02_4K_Base_Color.dds",
            "/MediaResources/Textures/MarbleTiles/Marble_tiles_02_4K_Normal.dds",
            "/MediaResources/Textures/MarbleTiles/Marble_tiles_02_4K_Roughness.dds"));

        PathFinder::Material& redPlasticMaterial = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/RedPlastic/plasticpattern1-albedo.dds",
            "/MediaResources/Textures/RedPlastic/plasticpattern1-normal2b.dds",
            "/MediaResources/Textures/RedPlastic/plasticpattern1-roughness2.dds"));

        PathFinder::Material& rustedIronMaterial = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/RustedIron/rustediron2_basecolor.dds",
            "/MediaResources/Textures/RustedIron/rustediron2_normal.dds",
            "/MediaResources/Textures/RustedIron/rustediron2_roughness.dds",
            "/MediaResources/Textures/RustedIron/rustediron2_metallic.dds"));

        PathFinder::Material& scuffedTitamiumMaterial = mScene->AddMaterial(mMaterialLoader->LoadMaterial(
            "/MediaResources/Textures/ScuffedTitanium/Titanium-Scuffed_basecolor.dds",
            "/MediaResources/Textures/ScuffedTitanium/Titanium-Scuffed_normal.dds",
            "/MediaResources/Textures/ScuffedTitanium/Titanium-Scuffed_roughness.dds",
            "/MediaResources/Textures/ScuffedTitanium/Titanium-Scuffed_metallic.dds"));

        PathFinder::Mesh& plane = mScene->AddMesh(std::move(mMeshLoader->Load("plane.obj").back()));

        for (float x = -100; x < 100; x += 20)
        {
            for (float z = -100; z < 100; z += 20)
            {
                PathFinder::MeshInstance& planeInstance = mScene->AddMeshInstance({ &plane, &concrete19Material });
                Geometry::Transformation t;
                t.Translation = glm::vec3{ x, -3.50207, z };
                planeInstance.SetTransformation(t);
            }
        }

        PathFinder::Mesh& cube = mScene->AddMesh(std::move(mMeshLoader->Load("cube.obj").back()));
        PathFinder::MeshInstance& cubeInstance = mScene->AddMeshInstance({ &cube, &metalMaterial });

        PathFinder::Mesh& sphereType1 = mScene->AddMesh(std::move(mMeshLoader->Load("sphere1.obj").back()));
        PathFinder::MeshInstance& sphereType1Instance0 = mScene->AddMeshInstance({ &sphereType1, &marbleTilesMaterial });

        PathFinder::Mesh& sphereType2 = mScene->AddMesh(std::move(mMeshLoader->Load("sphere2.obj").back()));
        PathFinder::MeshInstance& sphereType2Instance0 = mScene->AddMeshInstance({ &sphereType2, &marbleTilesMaterial });

        PathFinder::Mesh& sphereType3 = mScene->AddMesh(std::move(mMeshLoader->Load("sphere3.obj").back()));
        PathFinder::MeshInstance& sphereType3Instance0 = mScene->AddMeshInstance({ &sphereType3, &grimyMetalMaterial });
        PathFinder::MeshInstance& sphereType3Instance1 = mScene->AddMeshInstance({ &sphereType3, &redPlasticMaterial });
        PathFinder::MeshInstance& sphereType3Instance2 = mScene->AddMeshInstance({ &sphereType3, &marble006Material });
        PathFinder::MeshInstance& sphereType3Instance3 = mScene->AddMeshInstance({ &sphereType3, &charcoalMaterial });
        PathFinder::MeshInstance& sphereType3Instance4 = mScene->AddMeshInstance({ &sphereType3, &concrete19Material });
        PathFinder::MeshInstance& sphereType3Instance5 = mScene->AddMeshInstance({ &sphereType3, &metalMaterial });

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

        auto light0 = mScene->EmplaceRectangularLight();
        light0->SetWidth(7);
        light0->SetHeight(4);
        light0->SetNormal(glm::vec3{ 0.0, -1.0, 0.0 });
        light0->SetPosition({ 10.65, 15.0, -4.6 });
        light0->SetColor(light0Color);
        light0->SetLuminousPower(40000);

       /* auto sphereLight1 = mScene->EmplaceSphericalLight();
        sphereLight1->SetRadius(7.5);
        sphereLight1->SetPosition({ -10.65, 12.0, -4.6 });
        sphereLight1->SetColor(light1Color);
        sphereLight1->SetLuminousPower(100000);*/

        //auto sphereLight2 = mScene->EmplaceSphericalLight();
        //sphereLight2->SetRadius(6);
        //sphereLight2->SetPosition({ -5.3, 4.43, -4.76 });
        //sphereLight2->SetColor(light2Color);
        //sphereLight2->SetLuminousPower(300000);

        //auto sphereLight3 = mScene->EmplaceSphericalLight();
        //sphereLight3->SetRadius(8);
        //sphereLight3->SetPosition({ -5.3, 4.43, -4.76 });
        //sphereLight3->SetColor(light3Color);
        //sphereLight3->SetLuminousPower(300000);

        PathFinder::Camera& camera = mScene->MainCamera();
        camera.SetFarPlane(500);
        camera.SetNearPlane(1);
        camera.MoveTo({ 63.65, 6.41, -32.7 });
        camera.LookAt({ 0.f, 0.0f, 0.f });
        camera.SetViewportAspectRatio(16.0f / 9.0f);
        camera.SetAperture(1.2);
        camera.SetFilmSpeed(800);
        camera.SetShutterTime(1.0 / 125.0);
        camera.SetFieldOfView(90);

        //sceneManipulatorVC->CameraVM.SetCamera(&mScene->MainCamera());
        //sceneManipulatorVC->EntityVM.SetScene(&scene);

        mScene->GPUStorage().UploadMeshes();
        mScene->GPUStorage().UploadMaterials();

        // Build bottom AS only once. A production-level application would build every frame
        for (const PathFinder::BottomRTAS& bottomRTAS : mScene->GPUStorage().BottomAccelerationStructures())
        {
            mRenderEngine->AddBottomRayTracingAccelerationStructure(&bottomRTAS);
        }
    }

}
