#include "RenderEngine.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

namespace PathFinder
{

    RenderEngine::RenderEngine(HWND windowHandle, const std::filesystem::path& executablePath, const Scene* scene)
        : mExecutablePath{ executablePath },
        mDefaultRenderSurface{ { 1280, 720 }, HAL::ResourceFormat::Color::RGBA8_Usigned_Norm, HAL::ResourceFormat::DepthStencil::Depth24_Float_Stencil8_Unsigned },
        mDevice{ FetchDefaultDisplayAdapter() },
        mCopyDevice{ &mDevice },
        mFrameFence{ mDevice },
        mVertexStorage{ &mDevice, &mCopyDevice },
        mResourceStorage{ &mDevice, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mResourceScheduler{ &mResourceStorage },
        mResourceProvider{ &mResourceStorage },
        mRootConstantsUpdater{ &mResourceStorage },
        mShaderManager{ mExecutablePath / "Shaders/" },
        mPipelineStateManager{ &mDevice, &mShaderManager, mDefaultRenderSurface },
        mPipelineStateCreator{ &mPipelineStateManager },
        mGraphicsDevice{ mDevice, &mResourceStorage, &mPipelineStateManager, &mVertexStorage, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mContext{ scene, &mGraphicsDevice, &mRootConstantsUpdater, &mResourceProvider },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, mDefaultRenderSurface.RenderTargetFormat(), mDefaultRenderSurface.Dimensions() },
        mScene{ scene }
    {
        mResourceStorage.UseSwapChain(mSwapChain);
    }

    void RenderEngine::AddRenderPass(std::unique_ptr<RenderPass>&& pass)
    {
        mRenderPasses.push_back(std::move(pass));
        mPassExecutionGraph.AddPass(mRenderPasses.back().get());
    }

    void RenderEngine::PreRender()
    {
        for (auto& passPtr : mRenderPasses)
        {
            mResourceStorage.SetCurrentPassName(passPtr->Name());
            passPtr->SetupPipelineStates(&mPipelineStateCreator);
            passPtr->ScheduleResources(&mResourceScheduler);
        }

        mResourceStorage.AllocateScheduledResources(mPassExecutionGraph);
        mPipelineStateManager.CompileStates();
        mGraphicsDevice.CommandList().InsertBarriers(mResourceStorage.OneTimeResourceBarriers());
        mCopyDevice.CopyResources();
    }

    void RenderEngine::Render()
    {
        if (mRenderPasses.empty()) return;

        mFrameFence.IncreaseExpectedValue();
        mResourceStorage.BeginFrame(mFrameFence.ExpectedValue());
        mGraphicsDevice.BeginFrame(mFrameFence.ExpectedValue());

        UpdateCommonRootConstants();

        HAL::TextureResource* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();

        mGraphicsDevice.CommandList().InsertBarrier(
            HAL::ResourceTransitionBarrier{ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer }
        );

        for (auto& passPtr : mRenderPasses)
        {
            mGraphicsDevice.SetCurrentRenderPass(passPtr.get());
            mResourceStorage.SetCurrentPassName(passPtr->Name());
            mGraphicsDevice.CommandList().InsertBarriers(mResourceStorage.ResourceBarriersForCurrentPass());
            passPtr->Render(&mContext);
        }

        mGraphicsDevice.CommandList().InsertBarrier(
            HAL::ResourceTransitionBarrier{ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer }
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
