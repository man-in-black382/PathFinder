#include "RenderEngine.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"

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
        mRenderSurfaceDescription{ { 1920, 1080 }, HAL::ColorFormat::RGBA16_Float, HAL::DepthStencilFormat::Depth32_Float },
        mDevice{ FetchDefaultDisplayAdapter() },
        mResourceAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mCommandListAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mDescriptorAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mResourceProducer{ &mDevice, &mResourceAllocator, &mResourceStateTracker, &mDescriptorAllocator },
        mMeshStorage{ &mDevice, &mResourceProducer },
        mPipelineResourceStorage{ &mDevice, &mResourceProducer, &mDescriptorAllocator, &mResourceStateTracker, mRenderSurfaceDescription, mPassExecutionGraph },
        mResourceScheduler{ &mPipelineResourceStorage, mRenderSurfaceDescription },
        mResourceProvider{ &mPipelineResourceStorage  },
        mRootConstantsUpdater{ &mPipelineResourceStorage },
        mShaderManager{ commandLineParser },
        mPipelineStateManager{ &mDevice, &mShaderManager, mRenderSurfaceDescription },
        mPipelineStateCreator{ &mPipelineStateManager },
        mGraphicsDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mPipelineResourceStorage, &mPipelineStateManager, mRenderSurfaceDescription },
        mAsyncComputeDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mPipelineResourceStorage, &mPipelineStateManager, mRenderSurfaceDescription },
        mCommandRecorder{ &mGraphicsDevice },
        mUIStorage{ &mResourceProducer },
        mContext{ scene, &mMeshStorage, &mUIStorage, &mCommandRecorder, &mRootConstantsUpdater, &mResourceProvider, mRenderSurfaceDescription },
        mAsyncComputeFence{ mDevice },
        mGraphicsFence{ mDevice },
        mUploadFence{ mDevice },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, HAL::ColorFormat::RGBA8_Usigned_Norm, mRenderSurfaceDescription.Dimensions() },
        mScene{ scene }
    {
        mPipelineResourceStorage.CreateSwapChainBackBufferDescriptors(mSwapChain);
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());
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

    void RenderEngine::UploadProcessAndTransferAssets()
    {
        // Let resources record upload commands into graphics cmd list
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        // Run setup and asset-processing passes
        RunRenderPasses(mPassExecutionGraph->OneTimePasses());

        // Upload and process assets
        mGraphicsDevice.ExecuteCommands(nullptr, &mGraphicsFence);

        // Build BLASes once 
        for (const BottomRTAS& blas : mMeshStorage.BottomAccelerationStructures())
        {
            mAsyncComputeDevice.CommandList()->BuildRaytracingAccelerationStructure(blas.HALAccelerationStructure());
        }

        mAsyncComputeDevice.CommandList()->InsertBarriers(mMeshStorage.BottomAccelerationStructureBarriers());
        mAsyncComputeDevice.ExecuteCommands(&mGraphicsFence, &mAsyncComputeFence);

        // Execute readback commands
        mGraphicsFence.IncrementExpectedValue();

        // Let resources record readback commands into graphics cmd list
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        // Read all requested to read resources
        mAssetStorage.ReadbackAllAssets();

        // Perform readbacks
        mGraphicsDevice.ExecuteCommands(nullptr, &mGraphicsFence);

        // Wait until both devices are finished
        mGraphicsFence.StallCurrentThreadUntilCompletion();
        mAsyncComputeFence.StallCurrentThreadUntilCompletion();

        mAssetStorage.ReportAllAssetsPostprocessed();
    }

    void RenderEngine::Render()
    {
        if (mPassExecutionGraph->DefaultPasses().empty()) return;

        // Advance fences
        mAsyncComputeFence.IncrementExpectedValue();
        mGraphicsFence.IncrementExpectedValue();
        mUploadFence.IncrementExpectedValue();

        // Notify internal listeners
        NotifyStartFrame(mGraphicsFence.ExpectedValue());

        // Let resources record upload commands into graphics cmd list
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        // Notify external listeners
        mPreRenderEvent.Raise();

        // Execute any pending upload commands. Perform execution here because RT AS depends on resource uploads.
        mGraphicsDevice.ExecuteCommands(nullptr, &mUploadFence);

        //// Build AS
        //mAsyncComputeDevice.CommandList()->BuildRaytracingAccelerationStructure(mMeshStorage.TopAccelerationStructure().HALAccelerationStructure());
        //mAsyncComputeDevice.CommandList()->InsertBarrier(mMeshStorage.TopAccelerationStructure().UABarrier());
        //mAsyncComputeDevice.ExecuteCommands(&mUploadFence, &mAsyncComputeFence);

        // Wait for TLAS build then execute render passes
        HAL::Texture* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();
        HAL::ResourceTransitionBarrier preRenderBarrier{ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer };
        HAL::ResourceTransitionBarrier postRenderBarrier{ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer };

        // Let resources record transfer (upload/readback) commands into graphics cmd list
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        // Execute render passes that contain render work and may contain data transfers
        mGraphicsDevice.CommandList()->InsertBarrier(preRenderBarrier);
        //RunRenderPasses(mPassExecutionGraph->DefaultPasses());
        mGraphicsDevice.CommandList()->InsertBarrier(postRenderBarrier);

        // Execute all graphics work along with data transfers (upload/readback)
        //mGraphicsDevice.ExecuteCommands(&mAsyncComputeFence, &mGraphicsFence);

        // Put the picture on the screen
        mSwapChain.Present();

        // Issue a CPU wait if necessary
        mGraphicsFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        // Notify internal listeners
        NotifyEndFrame(mGraphicsFence.CompletedValue());

        // Notify external listeners
        mPostRenderEvent.Raise();

        // Readback data is ready
        GatherReadbackData();

        // 
        if (mPipelineStateManager.HasModifiedStates())
        {
            FlushAllQueuedFrames();
            mPipelineStateManager.RecompileModifiedStates();
        }

        MoveToNextFrame();
    }

    void RenderEngine::FlushAllQueuedFrames()
    {
        mGraphicsFence.StallCurrentThreadUntilCompletion();
    }

    HAL::DisplayAdapter RenderEngine::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch()[0];// .back();
    }

    void RenderEngine::NotifyStartFrame(uint64_t newFrameNumber)
    {
        mShaderManager.BeginFrame();
        mResourceAllocator.BeginFrame(newFrameNumber);
        mDescriptorAllocator.BeginFrame(newFrameNumber);
        mCommandListAllocator.BeginFrame(newFrameNumber);
        mResourceProducer.BeginFrame(newFrameNumber);
    }

    void RenderEngine::NotifyEndFrame(uint64_t completedFrameNumber)
    {
        mShaderManager.EndFrame();
        mResourceProducer.EndFrame(completedFrameNumber);
        mResourceAllocator.EndFrame(completedFrameNumber);
        mDescriptorAllocator.EndFrame(completedFrameNumber);
        mCommandListAllocator.EndFrame(completedFrameNumber);
    }

    void RenderEngine::MoveToNextFrame()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % (uint8_t)mSwapChain.BackBuffers().size();
        mPipelineResourceStorage.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
        mFrameNumber++;
    }

    void RenderEngine::GatherReadbackData()
    {
        // We can't touch read-back buffers until at least N first frames are complete
        if (mFrameNumber < mSimultaneousFramesInFlight)
        {
            return;
        }

       /* mPipelineResourceStorage.IterateDebugBuffers([this](PassName passName, const HAL::Buffer<float>* debugBuffer, const HAL::RingBufferResource<float>* debugReadbackBuffer) 
        {
            mUIStorage.ReadbackPassDebugBuffer(passName, *debugReadbackBuffer);
        });*/
    }

    void RenderEngine::RunRenderPasses(const std::list<RenderPass*>& passes)
    {
        for (auto passPtr : passes)
        {
            mGraphicsDevice.ResetViewportToDefault();
            mPipelineResourceStorage.SetCurrentPassName(passPtr->Name());
            mPipelineResourceStorage.TransitionResourcesToCurrentPassStates();

            HAL::ResourceBarrierCollection barriers{};
            barriers.AddBarriers(mPipelineResourceStorage.AliasingBarriersForCurrentPass());
            barriers.AddBarriers(mPipelineResourceStorage.TransitionBarriersForCurrentPass());

            mGraphicsDevice.CommandList()->InsertBarriers(barriers);

            passPtr->Render(&mContext);

            // Queue debug buffer read
          /*  auto debugBuffer = mPipelineResourceStorage.DebugBufferForCurrentPass();
            auto readackDebugBuffer = mPipelineResourceStorage.DebugReadbackBufferForCurrentPass();
            mReadbackCopyDevice.QueueBufferToBufferCopy(*debugBuffer, *readackDebugBuffer, 0, debugBuffer->Capacity(), readackDebugBuffer->CurrentFrameObjectOffset());*/
        }
    }

}
