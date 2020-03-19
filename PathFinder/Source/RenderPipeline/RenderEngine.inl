#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"

#include <pix.h>

namespace PathFinder
{

    template <class ContentMediator>
    RenderEngine<ContentMediator>::RenderEngine(HWND windowHandle, const CommandLineParser& commandLineParser)
        : mRenderSurfaceDescription{ { 1920, 1080 }, HAL::ColorFormat::RGBA16_Float, HAL::DepthStencilFormat::Depth32_Float },
        mDevice{ FetchDefaultDisplayAdapter() },
        mResourceAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mCommandListAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mDescriptorAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mResourceProducer{ &mDevice, &mResourceAllocator, &mResourceStateTracker, &mDescriptorAllocator },
        mPipelineResourceStorage{ &mDevice, &mResourceProducer, &mDescriptorAllocator, &mResourceStateTracker, mRenderSurfaceDescription, &mPassExecutionGraph },
        mResourceScheduler{ &mPipelineResourceStorage, mRenderSurfaceDescription },
        mResourceProvider{ &mPipelineResourceStorage },
        mRootConstantsUpdater{ &mPipelineResourceStorage },
        mShaderManager{ commandLineParser },
        mPipelineStateManager{ &mDevice, &mShaderManager, &mResourceProducer, mRenderSurfaceDescription },
        mPipelineStateCreator{ &mPipelineStateManager },
        mRootSignatureCreator{ &mPipelineStateManager },
        mGraphicsDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mResourceStateTracker, &mPipelineResourceStorage, &mPipelineStateManager, mRenderSurfaceDescription },
        mAsyncComputeDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mResourceStateTracker, &mPipelineResourceStorage, &mPipelineStateManager, mRenderSurfaceDescription },
        mCommandRecorder{ &mGraphicsDevice },
        mContext{ &mCommandRecorder, &mRootConstantsUpdater, &mResourceProvider, mRenderSurfaceDescription },
        mAsyncComputeFence{ mDevice },
        mGraphicsFence{ mDevice },
        mUploadFence{ mDevice },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, HAL::ColorFormat::RGBA8_Usigned_Norm, mRenderSurfaceDescription.Dimensions() }
    {
        for (auto& backBufferPtr : mSwapChain.BackBuffers())
        {
            mBackBuffers.emplace_back(mResourceProducer.NewTexture(backBufferPtr.get()));
        }

        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::AddRenderPass(RenderPass<ContentMediator>* pass)
    {
        mPassExecutionGraph.AddPass(pass->Metadata());
        mRenderPasses.emplace(pass->Metadata().Name, pass);
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::AddTopRayTracingAccelerationStructure(const TopRTAS* topRTAS)
    {
        mTopRTASes.push_back(topRTAS);
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::AddBottomRayTracingAccelerationStructure(const BottomRTAS* bottomRTAS)
    {
        mBottomRTASes.push_back(bottomRTAS);
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::SetContentMediator(ContentMediator* mediator)
    {
        mContext.SetContentMediator(mediator);
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::ScheduleAndAllocatePipelineResources()
    {
        // Schedule resources and states
        for (auto passNode : mPassExecutionGraph.AllPasses())
        {
            auto pass = mRenderPasses[passNode.PassMetadata.Name];
            mPipelineResourceStorage.SetCurrentRenderPassGraphNode(passNode);
            pass->SetupPipelineStates(&mPipelineStateCreator, &mRootSignatureCreator);
            pass->ScheduleResources(&mResourceScheduler);
        }

        mPipelineResourceStorage.AllocateScheduledResources();
        mPipelineStateManager.CompileSignaturesAndStates();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::UploadProcessAndTransferAssets()
    {
        // Let resources record upload commands into graphics cmd list
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        // Run asset-processing passes
        RunAssetProcessingPasses();

        // Upload and process assets
        mGraphicsDevice.ExecuteCommands(nullptr, &mGraphicsFence);

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

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::Render()
    {
        if (mPassExecutionGraph.DefaultPasses().empty()) return;

        // Advance fences
        mAsyncComputeFence.IncrementExpectedValue();
        mGraphicsFence.IncrementExpectedValue();
        mUploadFence.IncrementExpectedValue();

        // Update Graphics device with current frame back buffer
        mGraphicsDevice.SetBackBuffer(mBackBuffers[mCurrentBackBufferIndex].get());

        // Notify internal listeners
        NotifyStartFrame(mGraphicsFence.ExpectedValue());

        // Let resources record upload commands into graphics cmd list
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        // Notify external listeners
        mPreRenderEvent.Raise();

        // Execute any pending upload commands. Perform execution here because RT AS depends on resource uploads.
        mGraphicsDevice.ExecuteCommands(nullptr, &mUploadFence);

        // Build AS
        BuildAccelerationStructures(mUploadFence, mAsyncComputeFence);

        // Let resources record transfer (upload/readback) commands into graphics cmd list
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        // Execute render passes that contain render work and may contain data transfers
        RunDefaultPasses();

        // Execute all graphics work along with data transfers (upload/readback)
        // But wait for TLAS build first
        mGraphicsDevice.ExecuteCommands(&mAsyncComputeFence, &mGraphicsFence);

        // Put the picture on the screen
        mSwapChain.Present();

        // Issue a CPU wait if necessary
        mGraphicsFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        // Notify internal listeners
        NotifyEndFrame(mGraphicsFence.CompletedValue());

        // Notify external listeners
        mPostRenderEvent.Raise();

        // Recompile states that were modified, but after a pipeline flush
        if (mPipelineStateManager.HasModifiedStates())
        {
            FlushAllQueuedFrames();
            // Since recompilation of ray tracing involves shader table upload
            // we need to set correct command list to resource producer before recompilations
            mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());
            mPipelineStateManager.RecompileModifiedStates();
        }

        MoveToNextFrame();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::FlushAllQueuedFrames()
    {
        mGraphicsFence.StallCurrentThreadUntilCompletion();
    }

    template <class ContentMediator>
    HAL::DisplayAdapter RenderEngine<ContentMediator>::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch()[0];// .back();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::NotifyStartFrame(uint64_t newFrameNumber)
    {
        mShaderManager.BeginFrame();
        mResourceAllocator.BeginFrame(newFrameNumber);
        mDescriptorAllocator.BeginFrame(newFrameNumber);
        mCommandListAllocator.BeginFrame(newFrameNumber);
        mResourceProducer.BeginFrame(newFrameNumber);

        mFrameStartTimestamp = std::chrono::steady_clock::now();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::NotifyEndFrame(uint64_t completedFrameNumber)
    {
        mShaderManager.EndFrame();
        mResourceProducer.EndFrame(completedFrameNumber);
        mResourceAllocator.EndFrame(completedFrameNumber);
        mDescriptorAllocator.EndFrame(completedFrameNumber);
        mCommandListAllocator.EndFrame(completedFrameNumber);

        using namespace std::chrono;
        mFrameDuration = duration_cast<microseconds>(steady_clock::now() - mFrameStartTimestamp);
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::MoveToNextFrame()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % (uint8_t)mBackBuffers.size();
        mFrameNumber++;

        mBottomRTASes.clear();
        mTopRTASes.clear();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::BuildAccelerationStructures(HAL::Fence& fenceToWaitFor, HAL::Fence& fenceToSignal)
    {
        HAL::ResourceBarrierCollection bottomRTASUABarriers{};

        for (const BottomRTAS* blas : mBottomRTASes)
        {
            bottomRTASUABarriers.AddBarrier(blas->UABarrier());
            mAsyncComputeDevice.CommandList()->BuildRaytracingAccelerationStructure(blas->HALAccelerationStructure());
        }

        // Top RTAS needs to wait for Bottom RTAS
        mAsyncComputeDevice.CommandList()->InsertBarriers(bottomRTASUABarriers);

        HAL::ResourceBarrierCollection topRTASUABarriers{};

        for (const TopRTAS* tlas : mTopRTASes)
        {
            topRTASUABarriers.AddBarrier(tlas->UABarrier());
            mAsyncComputeDevice.CommandList()->BuildRaytracingAccelerationStructure(tlas->HALAccelerationStructure());
        }

        // Make the rest of the pipeline wait for Top RTAS build
        mAsyncComputeDevice.CommandList()->InsertBarriers(topRTASUABarriers);

        mAsyncComputeDevice.ExecuteCommands(&fenceToWaitFor, &fenceToSignal);
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::RunAssetProcessingPasses()
    {
        auto& nodes = mPassExecutionGraph.AssetProcessingPasses();

        for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            mGraphicsDevice.ResetViewportToDefault();
            mPipelineResourceStorage.SetCurrentRenderPassGraphNode(*nodeIt);
            mRenderPasses[nodeIt->PassMetadata.Name]->Render(&mContext);
        }
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::RunDefaultPasses()
    {
        Memory::Texture* currentBackBuffer = mBackBuffers[mCurrentBackBufferIndex].get();
        auto& nodes = mPassExecutionGraph.DefaultPasses();

        // This transition will be picked up automatically
        // by the first draw/dispatch in any of the render passes
        currentBackBuffer->RequestNewState(HAL::ResourceState::RenderTarget);

        for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            mGraphicsDevice.ResetViewportToDefault();
            mPipelineResourceStorage.SetCurrentRenderPassGraphNode(*nodeIt);
            mRenderPasses[nodeIt->PassMetadata.Name]->Render(&mContext); 
            mPipelineResourceStorage.RequestCurrentPassDebugReadback();
        }

        // This is a special case when a transition needs to be manually extracted from state tracker
        // and then executed, because no more dispatches/draws will be executed in this frame,
        // therefore no transitions will be performed automatically, but Swap Chain Present requires 
        // back buffer to be in Present state
        currentBackBuffer->RequestNewState(HAL::ResourceState::Present);
        mGraphicsDevice.CommandList()->InsertBarriers(mResourceStateTracker.ApplyRequestedTransitions());
    }

    template <class ContentMediator> 
    template <class Constants>
    void RenderEngine<ContentMediator>::SetFrameRootConstants(const Constants& constants)
    {
        mPipelineResourceStorage.UpdateFrameRootConstants(constants);
    }

    template <class ContentMediator>
    template <class Constants>
    void RenderEngine<ContentMediator>::SetGlobalRootConstants(const Constants& constants)
    {
        mPipelineResourceStorage.UpdateGlobalRootConstants(constants);
    }

}

