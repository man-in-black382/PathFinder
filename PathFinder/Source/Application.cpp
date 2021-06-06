#include "Application.hpp"

#include "../resource.h"
#include <Foundation/Filesystem.hpp>
#include <choreograph/Choreograph.h>
#include <windows.h>
#include <tchar.h>

namespace PathFinder
{

    static choreograph::Output<float> intensity1;
    static choreograph::Output<float> intensity2;
    static choreograph::Output<float> intensity3;
    static choreograph::Output<glm::vec3> cameraPosition;
    static choreograph::Output<glm::vec3> lightPosition;

    static choreograph::PhraseRef<float> intensity1Phrase = choreograph::makeRamp(0.0f, 400000.f, 2.f);
    static choreograph::PhraseRef<float> intensity2Phrase = choreograph::makeRamp(0.0f, 400000.0f, 2.f);
    static choreograph::PhraseRef<float> intensity3Phrase = choreograph::makeRamp(0.0f, 25000.0f, 1.0f);
    static choreograph::PhraseRef<glm::vec3> cameraPositionPhrase = choreograph::makeRamp(glm::vec3{ -18.77, -0.63, 0.095 }, glm::vec3{ 9.18, -7.48, -0.04 }, 1200.f);
    static choreograph::PhraseRef<glm::vec3> lightPositionPhrase = choreograph::makeRamp(glm::vec3{ 7.660, 8.417, 0.060 }, glm::vec3{ 7.660, - 12.7, 0.060 }, 4.f);

    static Light* gLight1 = nullptr;
    static Light* gLight2 = nullptr;
    static Light* gLight3 = nullptr;

    Application::Application(int argc, char** argv)
    {
        CreateEngineWindow();

        mCmdLineParser = std::make_unique<CommandLineParser>(argc, argv);
        mSettingsController = std::make_unique<RenderSettingsController>();
        mRenderEngine = std::make_unique<RenderEngine<RenderPassContentMediator>>(mWindowHandle, *mCmdLineParser);

        mScene = std::make_unique<Scene>(
            mCmdLineParser->ExecutableFolderPath(),
            mRenderEngine->Device(), 
            mRenderEngine->ResourceProducer(),
            mRenderEngine->ResourceStorage(),
            &mRenderEngine->RenderSurface(),
            mSettingsController->GetAppliedSettings());

        mInput = std::make_unique<Input>();
        mWindowsInputHandler = std::make_unique<InputHandlerWindows>(mInput.get(), mWindowHandle);
        mCameraInteractor = std::make_unique<CameraInteractor>(&mScene->GetMainCamera(), mInput.get());
        mDisplaySettingsController = std::make_unique<DisplaySettingsController>(mRenderEngine->SelectedAdapter(), mRenderEngine->SwapChain(), mWindowHandle);

        mUIDependencies = std::make_unique<UIDependencies>(
            mRenderEngine->ResourceStorage(),
            mRenderEngine.get(),
            &mSettingsController->VolatileSettings,
            mRenderEngine->RendererDevice(),
            mScene.get());

        mUIManager = std::make_unique<UIManager>(mInput.get(), mUIDependencies.get(), mRenderEngine->ResourceProducer());
        mUIEntryPoint = std::make_unique<UIEntryPoint>(mUIManager.get());
        mContentMediator = std::make_unique<RenderPassContentMediator>(&mUIManager->GPUStorage(), &mScene->GetGPUStorage(), mScene.get(), mInput.get(), mDisplaySettingsController.get(), mSettingsController.get());

        mRenderEngine->SetContentMediator(mContentMediator.get());

        mRenderEngine->PreRenderEvent() += { "Engine.Pre.Render", this, &Application::PerformPreRenderActions };
        mRenderEngine->PostRenderEvent() += { "Engine.Post.Render", this, & Application::PerformPostRenderActions };

        mInput->SetInvertVerticalDelta(true);

        InjectRenderPasses();

        mUIEntryPoint->CreateMandatoryViewControllers();

        // Temporary to load demo Scene until proper UI is implemented
        //mThirdPartySceneLoader = std::make_unique<ThirdPartySceneLoader>(mCmdLineParser->ExecutableFolderPath() / "MediaResources/Models/");
        //mMaterialLoader = std::make_unique<MaterialLoader>(mCmdLineParser->ExecutableFolderPath(), mRenderEngine->ResourceProducer());
       
        LoadDemoScene();
        //mScene->LoadThirdPartyScene(mCmdLineParser->ExecutableFolderPath() / "SanMiguel" / "san-miguel-low-poly.obj");
        //mScene->LoadThirdPartyScene(mCmdLineParser->ExecutableFolderPath() / "MediaResources" / "sibenik" / "sibenik.obj");
        ThirdPartySceneLoader::Settings loadSettings{};
        loadSettings.InitialScale = 0.02;
        mScene->LoadThirdPartyScene(mCmdLineParser->ExecutableFolderPath() / "MediaResources" / "sponza" / "sponza.obj", loadSettings);
        //mScene->Deserialize(mCmdLineParser->ExecutableFolderPath() / "DebugSceneSerialization" / "Scene.pfscene");

        Foundation::Color light0Color{ 255.0 / 255, 241.0 / 255, 224.1 / 255 };
        Foundation::Color light1Color{ 64.0 / 255, 156.0 / 255, 255.0 / 255 };
        Foundation::Color light2Color{ 255.0 / 255, 147.0 / 255, 41.0 / 255 };
        Foundation::Color light3Color{ 250.0 / 255, 110.0 / 255, 100.0 / 255 };

        //auto light0 = mScene->EmplaceRectangularLight();
        //light0->SetWidth(20);
        //light0->SetHeight(4);
        ////light0->SetRotation(glm::angleAxis(3.1415f / 2.0f, glm::vec3(1.0, 0.0, 0.0)));
        //light0->SetPosition({ 0.7, 14.0, -0.12 });
        //light0->SetColor(light0Color);
        //light0->SetLuminousPower(200000);

        /*  auto light1 = mScene->EmplaceRectangularLight();
          light1->SetWidth(20);
          light1->SetHeight(1.5);
          light1->SetNormal(glm::normalize(glm::vec3{ 0.0, -1.0, 0.0 }));
          light1->SetPosition({ -8.731, -8.386, -5.786 });
          light1->SetColor(light1Color);
          light1->SetLuminousPower(200000);

          auto light2 = mScene->EmplaceRectangularLight();
          light2->SetWidth(4);
          light2->SetHeight(4);
          light2->SetNormal(glm::normalize(glm::vec3{ 0.0, -1.0, 0.0 }));
          light2->SetPosition({ 7.66, 0, 0.06 });
          light2->SetColor(light2Color);
          light2->SetLuminousPower(100000);*/

     /*   auto sphereLight1 = mScene->EmplaceSphericalLight();
        sphereLight1->SetRadius(2);
        sphereLight1->SetPosition({ 7.66, 3, 0.06 });
        sphereLight1->SetColor(light1Color);
        sphereLight1->SetLuminousPower(100000);

        auto sphereLight2 = mScene->EmplaceSphericalLight();
        sphereLight2->SetRadius(2);
        sphereLight2->SetPosition({ 7.66, 0, 0.06 });
        sphereLight2->SetColor(light2Color);
        sphereLight2->SetLuminousPower(300000);*/

    /*    auto sphereLight3 = mScene->EmplaceSphericalLight();
        sphereLight3->SetRadius(2);
        sphereLight3->SetPosition({ 7.66, -3, 0.06 });
        sphereLight3->SetColor(light3Color);
        sphereLight3->SetLuminousPower(100000);*/

        PathFinder::Camera& camera = mScene->GetMainCamera();
        camera.SetFarPlane(5000);
        camera.SetNearPlane(1);
        camera.MoveTo({ 4.7, 11.0, -0.12 });
        camera.LookAt({ 0.0, 11.0, .0 });
        camera.SetViewportAspectRatio(16.0f / 9.0f);
        camera.SetAperture(1.2);
        camera.SetFilmSpeed(800);
        camera.SetShutterTime(1.0 / 125.0);
        camera.SetFieldOfView(90);

        //mScene->Serialize(mCmdLineParser->ExecutableFolderPath() / "DebugSceneSerialization" / "Scene.pfscene");

        /*gLight1 = &(*light0);
        gLight2 = &(*light1);
        gLight3 = &(*light2);*/

        /*  mAnimationTimeline.apply(&intensity1, intensity1Phrase).setStartTime(25.0f).finishFn(
              [&m = *intensity1.inputPtr(), &light0]
              {
                  if (m.getPlaybackSpeed() > 0.0)
                  {
                      m.setPlaybackSpeed(m.getPlaybackSpeed() * -1);
                      m.resetTime();
                  }

              }
          );

          mAnimationTimeline.apply(&intensity2, intensity2Phrase).setStartTime(3.0f + 25).finishFn(
              [&m = *intensity2.inputPtr(), &light1]
              {
                     if (m.getPlaybackSpeed() > 0.0)
                  {
                      m.setPlaybackSpeed(m.getPlaybackSpeed() * -1);
                      m.resetTime();
                  }
              }
          );

          mAnimationTimeline.apply(&intensity3, intensity3Phrase).setStartTime(6.0f + 25);
          mAnimationTimeline.apply(&lightPosition, lightPositionPhrase).setStartTime(6.0f + 25);

          mAnimationTimeline.apply(&cameraPosition, cameraPositionPhrase);*/
    }

    void Application::RunMessageLoop()
    {
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        bool shouldQuit = false;
        while (!shouldQuit)
        {
            while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    shouldQuit = true;

                TranslateMessage(&msg);
                DispatchMessage(&msg);

                mWindowsInputHandler->HandleMessage(msg);
                mDisplaySettingsController->HandleMessage(msg);
            }

            //mAnimationTimeline.step(1.0f / 60.0);

        /*    gLight1->SetLuminousPower(intensity1.value());
            gLight2->SetLuminousPower(intensity2.value());
            gLight3->SetLuminousPower(intensity3.value());
            gLight3->SetPosition(lightPosition.value());
            mScene->GetMainCamera().MoveTo(cameraPosition.value());
            mScene->GetMainCamera().LookAt(glm::vec3{ 9.18, -7.48, -0.04 });*/

            mInput->FinalizeInput();
            mSettingsController->ApplyVolatileSettings(*mScene); // Settings must be applied at the very beginning so that render passes stay coherent
            mRenderEngine->Render();
            mInput->Clear();
        }

        mRenderEngine->FlushAllQueuedFrames();
    }

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // https://docs.microsoft.com/en-us/windows/win32/learnwin32/closing-the-window
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
        mRenderEngine->AddRenderPass(&mDeferredLightingPass);
        mRenderEngine->AddRenderPass(&mDeferredShadowsPass);
        mRenderEngine->AddRenderPass(&mDenoiserPreBlurPass);
        mRenderEngine->AddRenderPass(&mDenoiserMipGenerationPass);
        mRenderEngine->AddRenderPass(&mReprojectionPass);
        mRenderEngine->AddRenderPass(&mDenoiserGradientConstructionPass);
        mRenderEngine->AddRenderPass(&mDenoiserGradientFilteringPass);
        mRenderEngine->AddRenderPass(&mDenoiserForwardProjectionPass);
        mRenderEngine->AddRenderPass(&mDenoiserHistoryFixPass);
        mRenderEngine->AddRenderPass(&mDenoiserMainPass);
        mRenderEngine->AddRenderPass(&mTAARenderPass);
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
        mRenderEngine->AddRenderPass(&mGIRayTracingPass);
        mRenderEngine->AddRenderPass(&mGIProbeUpdatePass);
        mRenderEngine->AddRenderPass(&mGIDebugPass);
    }

    void Application::PerformPreRenderActions()
    {
        static bool IsInitialSceneUploaded = false; // Dirty hack until dynamic mesh and material buffers are implemented

        const Geometry::Dimensions& viewportSize = mRenderEngine->RenderSurface().Dimensions();

        mScene->UpdatePreviousFrameValues();

        // 'Top' is screen bottom
        mUIManager->SetViewportSize(viewportSize);
        mCameraInteractor->SetViewportSize(viewportSize);
        mUIManager->Draw();

        bool interactingWithUI = mUIManager->IsInteracting();

        mCameraInteractor->SetKeyboardControlsEnabled(!interactingWithUI);
        mCameraInteractor->SetMouseControlsEnabled(!mUIManager->IsMouseOverUI());
        mCameraInteractor->PollInputs(mRenderEngine->FrameDurationUS());

        mSettingsController->SetEnabled(!interactingWithUI);

        if (!IsInitialSceneUploaded)
        {
            mScene->GetGPUStorage().UploadMeshes();
            mScene->GetGPUStorage().UploadMaterials();

            for (const PathFinder::BottomRTAS& bottomRTAS : mScene->GetGPUStorage().BottomAccelerationStructures())
                mRenderEngine->AddBottomRayTracingAccelerationStructure(&bottomRTAS);

            IsInitialSceneUploaded = true;
        }
        mScene->GetGIManager().Update();
        mScene->GetGPUStorage().UploadInstances();

        mRenderEngine->AddTopRayTracingAccelerationStructure(&mScene->GetGPUStorage().TopAccelerationStructure());

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

        mPerFrameConstants.PreviousFrameCamera = mRenderEngine->FrameNumber() > 1 ? 
            mPerFrameConstants.CurrentFrameCamera : mScene->GetGPUStorage().GetCameraGPURepresentation();

        mPerFrameConstants.CurrentFrameCamera = mScene->GetGPUStorage().GetCameraGPURepresentation();

        const PathFinder::RenderSettings& settings = *mSettingsController->GetAppliedSettings();

        mPerFrameConstants.MousePosition = mInput->MousePosition();
        mPerFrameConstants.IsDenoiserEnabled = settings.IsDenoiserEnabled;
        mPerFrameConstants.IsReprojectionHistoryDebugEnabled = settings.IsReprojectionHistoryDebugRenderingEnabled;
        mPerFrameConstants.IsGradientDebugEnabled = settings.IsDenoiserGradientDebugRenderingEnabled;
        mPerFrameConstants.IsMotionDebugEnabled = settings.IsDenoiserMotionDebugRenderingEnabled;
        mPerFrameConstants.IsDenoiserAntilagEnabled = settings.IsDenoiserAntilagEnabled;
        mPerFrameConstants.IsSMAAEnabled = settings.IsAntialiasingEnabled;
        mPerFrameConstants.IsSMAAEdgeDetectionEnabled = settings.IsAntialiasingEdgeDetectionEnabled;
        mPerFrameConstants.IsSMAABlendingWeightCalculationEnabled = settings.IsAntialiasingBlendingWeightCalculationEnabled;
        mPerFrameConstants.IsSMAANeighborhoodBlendingEnabled = settings.IsAntialiasingNeighborhoodBlendingEnabled;
        mPerFrameConstants.IsTAAEnabled = settings.IsTAAEnabled;
        mPerFrameConstants.IsGIEnabled = settings.IsGIEnabled;
        mPerFrameConstants.IsGIRecursionEnabled = settings.IsGIRecursionEnabled;
        mPerFrameConstants.IsGIIrradianceDebugEnabled = settings.IsGIShowIrradianceOnly;

        mRenderEngine->SetGlobalRootConstants(mGlobalConstants);
        mRenderEngine->SetFrameRootConstants(mPerFrameConstants);
    }

    void Application::PerformPostRenderActions()
    {
    }

    void Application::LoadDemoScene()
    {
        /* mScene->LoadThirdPartyScene(mCmdLineParser->ExecutableFolderPath() / "MediaResources" / "Models" / "sphere3.obj");

         MeshInstance& sphereInstance = mScene->GetMeshInstances().back();
         Mesh* sphereMesh = sphereInstance.GetAssociatedMesh();
         Material* sphereMaterial = sphereInstance.GetAssociatedMaterial();

         Geometry::Transformation t;
         glm::vec3 translation{ -12.5, -6.0, 0.0 };
         t.SetTranslation(translation);
         t.SetScale(glm::vec3{ 0.05f });

         sphereInstance.SetTransformation(t);

         sphereMaterial->DiffuseAlbedoOverride = glm::vec3{ 1.0f };
         sphereMaterial->MetalnessOverride = 0.0f;
         sphereMaterial->RoughnessOverride = 0.1f;

         float roughness = 0.1;

         for (auto i = 0; i < 9; ++i)
         {
             roughness += 0.1;

             Material& nextMaterial = mScene->GetMaterials().emplace_back();
             nextMaterial.DiffuseAlbedoOverride = glm::vec3{ 0.0f, 1.0f, 0.0f };
             nextMaterial.MetalnessOverride = 0.0f;
             nextMaterial.RoughnessOverride = roughness;
             nextMaterial.DiffuseAlbedoMap.Texture = Memory::GPUResourceProducer::TexturePtr{ sphereMaterial->DiffuseAlbedoMap.Texture.get(), [](auto* a) {} };
             nextMaterial.NormalMap.Texture = Memory::GPUResourceProducer::TexturePtr{ sphereMaterial->NormalMap.Texture.get(), [](auto* a) {} };
             nextMaterial.RoughnessMap.Texture = Memory::GPUResourceProducer::TexturePtr{ sphereMaterial->RoughnessMap.Texture.get(), [](auto* a) {} };
             nextMaterial.MetalnessMap.Texture = Memory::GPUResourceProducer::TexturePtr{ sphereMaterial->MetalnessMap.Texture.get(), [](auto* a) {} };
             nextMaterial.DisplacementMap.Texture = Memory::GPUResourceProducer::TexturePtr{ sphereMaterial->DisplacementMap.Texture.get(), [](auto* a) {} };
             nextMaterial.DistanceField.Texture = Memory::GPUResourceProducer::TexturePtr{ sphereMaterial->DistanceField.Texture.get(), [](auto* a) {} };
             nextMaterial.LTC_LUT_MatrixInverse_Specular = sphereMaterial->LTC_LUT_MatrixInverse_Specular;
             nextMaterial.LTC_LUT_Matrix_Specular = sphereMaterial->LTC_LUT_Matrix_Specular;
             nextMaterial.LTC_LUT_Terms_Specular = sphereMaterial->LTC_LUT_Terms_Specular;
             nextMaterial.LTC_LUT_MatrixInverse_Diffuse = sphereMaterial->LTC_LUT_MatrixInverse_Diffuse;
             nextMaterial.LTC_LUT_Matrix_Diffuse = sphereMaterial->LTC_LUT_Matrix_Diffuse;
             nextMaterial.LTC_LUT_Terms_Diffuse = sphereMaterial->LTC_LUT_Terms_Diffuse;

             MeshInstance& nextInstance = mScene->GetMeshInstances().emplace_back(sphereMesh, &nextMaterial);
             translation.x += 2.5;
             t.SetTranslation(translation);

             nextInstance.SetTransformation(t);
         }*/
    }

}
