#include "RenderEngine.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

namespace PathFinder
{

    RenderEngine::RenderEngine(
        HWND windowHandle, const std::filesystem::path& executablePath, 
        Scene* scene, const RenderPassExecutionGraph* passExecutionGraph)
        : 
        mPassExecutionGraph{ passExecutionGraph },
        mExecutablePath{ executablePath },
        mDefaultRenderSurface{ { 1280, 720 }, HAL::ResourceFormat::Color::RGBA8_Usigned_Norm, HAL::ResourceFormat::DepthStencil::Depth24_Float_Stencil8_Unsigned },
        mDevice{ FetchDefaultDisplayAdapter() },
        mFrameFence{ mDevice },
        mCopyDevice{ &mDevice },
        mVertexStorage{ &mDevice, &mCopyDevice },
        mDescriptorStorage{ &mDevice },
        mPipelineResourceStorage{ &mDevice, &mDescriptorStorage, mDefaultRenderSurface, mSimultaneousFramesInFlight, mPassExecutionGraph },
        mAssetResourceStorage{ mDevice, &mDescriptorStorage, mSimultaneousFramesInFlight },
        mResourceScheduler{ &mPipelineResourceStorage, mDefaultRenderSurface },
        mResourceProvider{ &mPipelineResourceStorage },
        mRootConstantsUpdater{ &mPipelineResourceStorage },
        mShaderManager{ mExecutablePath / "Shaders/" },
        mPipelineStateManager{ &mDevice, &mShaderManager, mDefaultRenderSurface },
        mPipelineStateCreator{ &mPipelineStateManager, mDefaultRenderSurface },
        mGraphicsDevice{ mDevice, &mDescriptorStorage.CBSRUADescriptorHeap(), &mPipelineResourceStorage, &mPipelineStateManager, &mVertexStorage, &mAssetResourceStorage, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mContext{ scene, &mGraphicsDevice, &mRootConstantsUpdater, &mResourceProvider },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, mDefaultRenderSurface.RenderTargetFormat(), mDefaultRenderSurface.Dimensions() },
        mScene{ scene }
    {
        mPipelineResourceStorage.CreateSwapChainBackBufferDescriptors(mSwapChain);
    }

    void RenderEngine::PreRender()
    {
        for (auto passPtr : mPassExecutionGraph->ExecutionOrder())
        {
            mPipelineResourceStorage.SetCurrentPassName(passPtr->Name());
            passPtr->SetupPipelineStates(&mPipelineStateCreator);
            passPtr->ScheduleResources(&mResourceScheduler);
        }

        mPipelineResourceStorage.AllocateScheduledResources();
        mPipelineStateManager.CompileStates();
        mGraphicsDevice.CommandList().InsertBarriers(mPipelineResourceStorage.OneTimeResourceBarriers());
        mCopyDevice.CopyResources();
    }

    void RenderEngine::Render()
    {
        if (mPassExecutionGraph->ExecutionOrder().empty()) return;

        mFrameFence.IncreaseExpectedValue();
        mPipelineResourceStorage.BeginFrame(mFrameFence.ExpectedValue());
        mGraphicsDevice.BeginFrame(mFrameFence.ExpectedValue());
        mAssetResourceStorage.BeginFrame(mFrameFence.ExpectedValue());

        UpdateCommonRootConstants();
        UpdateTransientResources();

        HAL::TextureResource* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();

        mGraphicsDevice.CommandList().InsertBarrier(
            HAL::ResourceTransitionBarrier{ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer }
        );

        for (auto passPtr : mPassExecutionGraph->ExecutionOrder())
        {
            mGraphicsDevice.SetCurrentRenderPass(passPtr);
            mPipelineResourceStorage.SetCurrentPassName(passPtr->Name());
            mGraphicsDevice.CommandList().InsertBarriers(mPipelineResourceStorage.ResourceBarriersForCurrentPass());
            passPtr->Render(&mContext);
        }

        mGraphicsDevice.CommandList().InsertBarrier(
            HAL::ResourceTransitionBarrier{ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer }
        );

        mGraphicsDevice.ExecuteCommandsThenSignalFence(mFrameFence);
        mSwapChain.Present();

        mFrameFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        mPipelineResourceStorage.EndFrame(mFrameFence.CompletedValue());
        mGraphicsDevice.EndFrame(mFrameFence.CompletedValue());
        mAssetResourceStorage.EndFrame(mFrameFence.CompletedValue());

        MoveToNextBackBuffer();
    }

    HAL::DisplayAdapter RenderEngine::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch()[0];// .back();
    }

    void RenderEngine::MoveToNextBackBuffer()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % (uint8_t)mSwapChain.BackBuffers().size();
        mPipelineResourceStorage.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
    }

    void RenderEngine::UpdateCommonRootConstants()
    {
        GlobalRootConstants* globalConstants = mPipelineResourceStorage.GlobalRootConstantData();
        PerFrameRootConstants* perFrameConstants = mPipelineResourceStorage.PerFrameRootConstantData();

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

    void RenderEngine::UpdateTransientResources()
    {
        mScene->IterateMeshInstances([&](MeshInstance& instance)
        {
             auto indexInTable = mAssetResourceStorage.UpdateInstanceTable(instance.CreateGPUInstanceTableEntry());
             instance.SetIndexInGPUInstanceTable(indexInTable);
        });
    }

}
