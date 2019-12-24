#include "RenderEngine.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

#include <pix.h>

namespace PathFinder
{

    RenderEngine::RenderEngine(
        HWND windowHandle, 
        const CommandLineParser& commandLineParser,
        Scene* scene, 
        const RenderPassExecutionGraph* passExecutionGraph)
        : 
        mPassExecutionGraph{ passExecutionGraph },
        mDefaultRenderSurface{ { 1920, 1080 }, HAL::ResourceFormat::Color::RGBA16_Float, HAL::ResourceFormat::DepthStencil::Depth32_Float },
        mDevice{ FetchDefaultDisplayAdapter() },
        mFrameFence{ mDevice },
        mAccelerationStructureFence{ mDevice },
        mStandardCopyDevice{ &mDevice },
        mAssetPostprocessCopyDevice{ &mDevice },
        mVertexStorage{ &mDevice, &mStandardCopyDevice },
        mDescriptorStorage{ &mDevice },
        mPipelineResourceStorage{ &mDevice, &mDescriptorStorage, mDefaultRenderSurface, mSimultaneousFramesInFlight, mPassExecutionGraph },
        mAssetResourceStorage{ &mDevice, &mAssetPostprocessCopyDevice, &mDescriptorStorage, mSimultaneousFramesInFlight },
        mResourceScheduler{ &mPipelineResourceStorage, mDefaultRenderSurface },
        mResourceProvider{ &mPipelineResourceStorage, &mDescriptorStorage },
        mRootConstantsUpdater{ &mPipelineResourceStorage },
        mShaderManager{ commandLineParser },
        mPipelineStateManager{ &mDevice, &mShaderManager, mDefaultRenderSurface },
        mPipelineStateCreator{ &mPipelineStateManager },
        mGraphicsDevice{ mDevice, &mDescriptorStorage.CBSRUADescriptorHeap(), &mPipelineResourceStorage, &mPipelineStateManager, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mAsyncComputeDevice{ mDevice, &mDescriptorStorage.CBSRUADescriptorHeap(), &mPipelineResourceStorage, &mPipelineStateManager, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mCommandRecorder{ &mGraphicsDevice },
        mUIStorage{ &mDevice, mSimultaneousFramesInFlight },
        mContext{ scene, &mAssetResourceStorage, &mVertexStorage, &mCommandRecorder, &mRootConstantsUpdater, &mResourceProvider, mDefaultRenderSurface },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, HAL::ResourceFormat::Color::RGBA8_Usigned_Norm, mDefaultRenderSurface.Dimensions() },
        mScene{ scene }
    {
        mPipelineResourceStorage.CreateSwapChainBackBufferDescriptors(mSwapChain);
    }

    void RenderEngine::ScheduleAndAllocatePipelineResources()
    {
        // Schedule resources and states
        for (auto passPtr : mPassExecutionGraph->AllPasses())
        {
            mPipelineResourceStorage.SetCurrentPassName(passPtr->Name());
            passPtr->SetupPipelineStates(&mPipelineStateCreator);
            passPtr->ScheduleResources(&mResourceScheduler);
        }
 
        mPipelineResourceStorage.AllocateScheduledResources();
        mPipelineStateManager.CompileStates();
    }

    void RenderEngine::ProcessAndTransferAssets()
    {
        // Allocate and transfer GPU memory, compile states, perform initial resource transitions
        mVertexStorage.AllocateAndQueueBuffersForCopy();
        mStandardCopyDevice.CopyResources();

        // Run setup and asset-processing passes
        RunRenderPasses(mPassExecutionGraph->OneTimePasses());

        mFrameFence.IncreaseExpectedValue();

        // Transition resources after asset-processing passes completed their work
        mGraphicsDevice.CommandList().InsertBarriers(mAssetResourceStorage.AssetPostProcessingBarriers());
        mGraphicsDevice.ExecuteCommands();
        mGraphicsDevice.SignalFence(mFrameFence);

        // Copy processed assets if needed
        mAssetPostprocessCopyDevice.WaitFence(mFrameFence);
        mAssetPostprocessCopyDevice.CopyResources();

        // Unlock device before rendering starts
        mGraphicsDevice.ResetCommandList();

        BuildBottomAccelerationStructures();
    }

    void RenderEngine::Render()
    {
        if (mPassExecutionGraph->DefaultPasses().empty()) return;

        mFrameFence.IncreaseExpectedValue();
        NotifyStartFrame();
        mPreRenderEvent.Raise();

        UploadUI();
        UploadCommonRootConstants();
        UploadMeshInstanceData();
        BuildTopAccelerationStructures();

        HAL::TextureResource* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();
        HAL::ResourceTransitionBarrier preRenderBarrier{ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer };
        HAL::ResourceTransitionBarrier postRenderBarrier{ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer };

        mGraphicsDevice.CommandList().InsertBarrier(preRenderBarrier);
        RunRenderPasses(mPassExecutionGraph->DefaultPasses());
        mGraphicsDevice.CommandList().InsertBarrier(postRenderBarrier);
        mGraphicsDevice.WaitFence(mAccelerationStructureFence);
        mGraphicsDevice.ExecuteCommands();
        mGraphicsDevice.SignalFence(mFrameFence);
        mSwapChain.Present();
        mFrameFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);
        mPostRenderEvent.Raise();

        NotifyEndFrame();

        if (mPipelineStateManager.HasModifiedStates())
        {
            FlushAllQueuedFrames();
            mPipelineStateManager.RecompileModifiedStates();
        }

        MoveToNextBackBuffer();
    }

    void RenderEngine::FlushAllQueuedFrames()
    {
        mFrameFence.StallCurrentThreadUntilCompletion();
    }

    HAL::DisplayAdapter RenderEngine::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch()[0];// .back();
    }

    void RenderEngine::NotifyStartFrame()
    {
        mShaderManager.BeginFrame();
        mPipelineResourceStorage.BeginFrame(mFrameFence.ExpectedValue());
        mGraphicsDevice.BeginFrame(mFrameFence.ExpectedValue());
        mAsyncComputeDevice.BeginFrame(mFrameFence.ExpectedValue());
        mAssetResourceStorage.BeginFrame(mFrameFence.ExpectedValue());
        mUIStorage.BeginFrame(mFrameFence.ExpectedValue());
    }

    void RenderEngine::NotifyEndFrame()
    {
        mShaderManager.EndFrame();
        mPipelineResourceStorage.EndFrame(mFrameFence.CompletedValue());
        mGraphicsDevice.EndFrame(mFrameFence.CompletedValue());
        mAsyncComputeDevice.EndFrame(mFrameFence.CompletedValue());
        mAssetResourceStorage.EndFrame(mFrameFence.CompletedValue());
        mUIStorage.EndFrame(mFrameFence.CompletedValue());
    }

    void RenderEngine::MoveToNextBackBuffer()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % (uint8_t)mSwapChain.BackBuffers().size();
        mPipelineResourceStorage.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
    }

    void RenderEngine::UploadCommonRootConstants()
    {
        GlobalRootConstants* globalConstants = mPipelineResourceStorage.GlobalRootConstantData();
        PerFrameRootConstants* perFrameConstants = mPipelineResourceStorage.PerFrameRootConstantData();

        globalConstants->PipelineRTResolution = { mDefaultRenderSurface.Dimensions().Width, mDefaultRenderSurface.Dimensions().Height };

        const Camera& camera = mScene->MainCamera();

        perFrameConstants->CameraPosition = glm::vec4{ camera.Position(), 1.0 };
        perFrameConstants->CameraView = camera.View();
        perFrameConstants->CameraProjection = camera.Projection();
        perFrameConstants->CameraViewProjection = camera.ViewProjection();
        perFrameConstants->CameraInverseView = camera.InverseView();
        perFrameConstants->CameraInverseProjection = camera.InverseProjection();
        perFrameConstants->CameraInverseViewProjection = camera.InverseViewProjection();
    }

    void RenderEngine::BuildBottomAccelerationStructures()
    {
        mAccelerationStructureFence.IncreaseExpectedValue();

        // Build BLASes once 
        for (const HAL::RayTracingBottomAccelerationStructure& blas : mVertexStorage.BottomAccelerationStructures())
        {
            mAsyncComputeDevice.CommandList().BuildRaytracingAccelerationStructure(blas);
        }

        mAsyncComputeDevice.CommandList().InsertBarriers(mVertexStorage.AccelerationStructureBarriers()); 
        mAsyncComputeDevice.ExecuteCommands();
        mAsyncComputeDevice.SignalFence(mAccelerationStructureFence);
        mAccelerationStructureFence.StallCurrentThreadUntilCompletion();
        mAsyncComputeDevice.ResetCommandList();
    }

    void RenderEngine::BuildTopAccelerationStructures()
    {
        mAccelerationStructureFence.IncreaseExpectedValue();
        mAssetResourceStorage.AllocateTopAccelerationStructureIfNeeded();
        mAsyncComputeDevice.CommandList().BuildRaytracingAccelerationStructure(mAssetResourceStorage.TopAccelerationStructure());
        mAsyncComputeDevice.CommandList().InsertBarriers(mAssetResourceStorage.TopAccelerationStructureBarriers());
        mAsyncComputeDevice.ExecuteCommands();
        mAsyncComputeDevice.SignalFence(mAccelerationStructureFence);
    }

    void RenderEngine::UploadMeshInstanceData()
    {
        mAssetResourceStorage.ResetInstanceStorages();

        auto iterator = [&](MeshInstance& instance)
        {
            auto blasIndex = instance.AssosiatedMesh()->LocationInVertexStorage().BottomAccelerationStructureIndex;
            auto& blas = mVertexStorage.BottomAccelerationStructures()[blasIndex];

            auto indexInTable = mAssetResourceStorage.StoreMeshInstance(instance, blas);

            instance.SetGPUInstanceIndex(indexInTable);
        };

        mScene->IterateMeshInstances(iterator);

        mAssetResourceStorage.AllocateTopAccelerationStructureIfNeeded();
    }

    void RenderEngine::UploadUI()
    {
        mUIStorage.UploadUIVertices();
    }

    void RenderEngine::RunRenderPasses(const std::list<RenderPass*>& passes)
    {
        for (auto passPtr : passes)
        {
            mGraphicsDevice.ResetViewportToDefault();
            mPipelineResourceStorage.SetCurrentPassName(passPtr->Name());
            mGraphicsDevice.CommandList().InsertBarriers(mPipelineResourceStorage.TransitionAndAliasingBarriersForCurrentPass());

            passPtr->Render(&mContext);
        }
    }

}
