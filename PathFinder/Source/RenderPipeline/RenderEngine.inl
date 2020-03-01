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
        mGraphicsDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mPipelineResourceStorage, &mPipelineStateManager, mRenderSurfaceDescription },
        mAsyncComputeDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mPipelineResourceStorage, &mPipelineStateManager, mRenderSurfaceDescription },
        mCommandRecorder{ &mGraphicsDevice },
        mContext{ &mCommandRecorder, &mRootConstantsUpdater, &mResourceProvider, mRenderSurfaceDescription },
        mAsyncComputeFence{ mDevice },
        mGraphicsFence{ mDevice },
        mUploadFence{ mDevice },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, HAL::ColorFormat::RGBA8_Usigned_Norm, mRenderSurfaceDescription.Dimensions() }
    {
        mPipelineResourceStorage.CreateSwapChainBackBufferDescriptors(mSwapChain);
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::AddRenderPass(RenderPass<ContentMediator>* pass)
    {
        mPassExecutionGraph.AddPass(pass->Metadata());
        mRenderPasses.emplace(pass->Metadata().Name, pass);
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
            pass->SetupPipelineStates(&mPipelineStateCreator);
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

        // Build BLASes once 
    /*    for (const BottomRTAS& blas : mMeshStorage.BottomAccelerationStructures())
        {
            mAsyncComputeDevice.CommandList()->BuildRaytracingAccelerationStructure(blas.HALAccelerationStructure());
        }

        mAsyncComputeDevice.CommandList()->InsertBarriers(mMeshStorage.BottomAccelerationStructureBarriers());*/
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

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::Render()
    {
        if (mPassExecutionGraph.DefaultPasses().empty()) return;

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

        // Build AS
        /*mAsyncComputeDevice.CommandList()->BuildRaytracingAccelerationStructure(mMeshStorage.TopAccelerationStructure().HALAccelerationStructure());
        mAsyncComputeDevice.CommandList()->InsertBarrier(mMeshStorage.TopAccelerationStructure().UABarrier());*/
        mAsyncComputeDevice.ExecuteCommands(&mUploadFence, &mAsyncComputeFence);

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

        // 
        if (mPipelineStateManager.HasModifiedStates())
        {
            FlushAllQueuedFrames();
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
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % (uint8_t)mSwapChain.BackBuffers().size();
        mPipelineResourceStorage.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
        mFrameNumber++;
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::RunAssetProcessingPasses()
    {
        auto& nodes = mPassExecutionGraph.AssetProcessingPasses();

        for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            mGraphicsDevice.ResetViewportToDefault();
            mPipelineResourceStorage.SetCurrentRenderPassGraphNode(*nodeIt);
            mPipelineResourceStorage.TransitionResourcesToCurrentPassStates();
    
            HAL::ResourceBarrierCollection barriers{};
            barriers.AddBarriers(mPipelineResourceStorage.AliasingBarriersForCurrentPass());
            barriers.AddBarriers(mPipelineResourceStorage.TransitionBarriersForCurrentPass());

            mGraphicsDevice.CommandList()->InsertBarriers(barriers);

            mRenderPasses[nodeIt->PassMetadata.Name]->Render(&mContext);
        }
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::RunDefaultPasses()
    {
        HAL::Texture* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();
        auto& nodes = mPassExecutionGraph.DefaultPasses();

        for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            bool firstPass = nodeIt == nodes.begin();

            mGraphicsDevice.ResetViewportToDefault();
            mPipelineResourceStorage.SetCurrentRenderPassGraphNode(*nodeIt);
            mPipelineResourceStorage.TransitionResourcesToCurrentPassStates();

            HAL::ResourceBarrierCollection barriers{};
            barriers.AddBarriers(mPipelineResourceStorage.AliasingBarriersForCurrentPass());
            barriers.AddBarriers(mPipelineResourceStorage.TransitionBarriersForCurrentPass());

            if (firstPass)
            {
                barriers.AddBarrier(HAL::ResourceTransitionBarrier{
                    HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer });
            }

            mGraphicsDevice.CommandList()->InsertBarriers(barriers);
            mRenderPasses[nodeIt->PassMetadata.Name]->Render(&mContext); 
            mPipelineResourceStorage.RequestCurrentPassDebugReadback();
        }

        mGraphicsDevice.CommandList()->InsertBarrier(HAL::ResourceTransitionBarrier{
            HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer });
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

