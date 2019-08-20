#include "RenderEngine.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

namespace PathFinder
{

    RenderEngine::RenderEngine(HWND windowHandle, const std::filesystem::path& executablePath, const Scene* scene)
        : mExecutablePath{ executablePath },
        mDefaultRenderSurface{ { 1280, 720 }, HAL::ResourceFormat::Color::RGBA8_Usigned_Norm, HAL::ResourceFormat::DepthStencil::Depth24_Float_Stencil8_Unsigned },
        mDevice{ FetchDefaultDisplayAdapter() },
        mFrameFence{ mDevice },
        mVertexStorage{ &mDevice },
        mResourceStorage{ &mDevice, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mResourceScheduler{ &mResourceStorage },
        mResourceProvider{ &mResourceStorage },
        mRootConstantsUpdater{ &mResourceStorage },
        mShaderManager{ mExecutablePath / "Shaders/" },
        mPipelineStateManager{ &mDevice, mDefaultRenderSurface },
        mGraphicsDevice{ mDevice, &mResourceStorage, &mPipelineStateManager, &mVertexStorage, mSimultaneousFramesInFlight },
        mContext{ scene, &mGraphicsDevice, &mRootConstantsUpdater },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, mDefaultRenderSurface.RenderTargetFormat(), mDefaultRenderSurface.Dimensions() },
        mScene{ scene }
    {
        mResourceStorage.UseSwapChain(mSwapChain);
    }

    void RenderEngine::AddRenderPass(std::unique_ptr<RenderPass>&& pass)
    {
        mRenderPasses.emplace_back(std::move(pass));
    }

    void RenderEngine::Schedule()
    {
        for (auto& passPtr : mRenderPasses)
        {
            mResourceStorage.SetCurrentPassName(passPtr->Name());
            passPtr->SetupPipelineStates(&mShaderManager, &mPipelineStateManager);
            passPtr->ScheduleResources(&mResourceScheduler);
        }

        mResourceStorage.AllocateScheduledResources();
        mPipelineStateManager.CompileStates();
    }

    void RenderEngine::Render()
    {
        if (mRenderPasses.empty()) return;

        mFrameFence.IncreaseExpectedValue();
        mResourceStorage.BeginFrame(mFrameFence.ExpectedValue());
        mGraphicsDevice.BeginFrame(mFrameFence.ExpectedValue());

        UpdateCommonRootConstants();

        HAL::TextureResource* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();

        mGraphicsDevice.CommandList().TransitionResourceState(
            { HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer }
        );

        for (auto& passPtr : mRenderPasses)
        {
            mResourceStorage.SetCurrentPassName(passPtr->Name());
            TransitionResourceStates(passPtr->Name());
            passPtr->Render(&mContext);
        }

        mGraphicsDevice.CommandList().TransitionResourceState(
            { HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer }
        );

        mGraphicsDevice.ExecuteCommandsThenSignalFence(mFrameFence);
        mSwapChain.Present();

        mFrameFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        mResourceStorage.EndFrame(mFrameFence.CompletedValue());
        mGraphicsDevice.EndFrame(mFrameFence.CompletedValue());

        MoveToNextBackBuffer();
    }

    HAL::DisplayAdapter RenderEngine::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch()[0];// .back();
    }

    void RenderEngine::MoveToNextBackBuffer()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % mSwapChain.BackBuffers().size();
        mResourceStorage.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
    }

    void RenderEngine::TransitionResourceStates(PassName passName)
    {
        for (ResourceName resourceName : mResourceStorage.ScheduledResourceNamesForCurrentPass())
        {
            PipelineResource& pipelineResource = mResourceStorage.GetPipelineResource(resourceName);
            HAL::ResourceState currentState = pipelineResource.CurrentState;

            auto perPassData = pipelineResource.GetPerPassData(passName);

            if (!perPassData || !perPassData->OptimizedState) continue;

            HAL::ResourceState nextState = *perPassData->OptimizedState;

            if (currentState != nextState)
            {
                mGraphicsDevice.CommandList().TransitionResourceState({ currentState, nextState, pipelineResource.Resource() });
                pipelineResource.CurrentState = nextState;
            }
        }
    }

    void RenderEngine::UpdateCommonRootConstants()
    {
        GlobalRootConstants* globalConstants = mResourceStorage.GlobalRootConstantData();
        PerFrameRootConstants* perFrameConstants = mResourceStorage.PerFrameRootConstantData();

        globalConstants->PipelineRTResolution = { mDefaultRenderSurface.Dimensions().Width, mDefaultRenderSurface.Dimensions().Height };

        const Camera& camera = mScene->MainCamera();

        perFrameConstants->CameraPosition = glm::vec4{ camera.Position(), 1.0 };
        perFrameConstants->CameraInverseView = camera.View();
        perFrameConstants->CameraProjection = camera.Projection();
        perFrameConstants->CameraViewProjection = camera.ViewProjection();
        perFrameConstants->CameraInverseView = camera.InverseView();
        perFrameConstants->CameraInverseProjection = camera.InverseProjection();
        perFrameConstants->CameraInverseViewProjection = camera.InverseViewProjection();
    }

}
