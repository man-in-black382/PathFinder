#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"

#include <pix.h>

namespace PathFinder
{

    template <class ContentMediator>
    RenderEngine<ContentMediator>::RenderEngine(HWND windowHandle, const CommandLineParser& commandLineParser)
        : mRenderSurfaceDescription{ { 1920, 1080 }, HAL::ColorFormat::RGBA16_Float, HAL::DepthStencilFormat::Depth32_Float },
        mPassUtilityProvider{ 0, mRenderSurfaceDescription },
        mDevice{ FetchDefaultDisplayAdapter(), commandLineParser.ShouldEnableDebugLayer() },
        mResourceAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mCommandListAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mDescriptorAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mResourceProducer{ &mDevice, &mResourceAllocator, &mResourceStateTracker, &mDescriptorAllocator },
        mPipelineResourceStorage{ &mDevice, &mResourceProducer, &mDescriptorAllocator, &mResourceStateTracker, mRenderSurfaceDescription, &mRenderPassGraph },
        mResourceScheduler{ &mPipelineResourceStorage, &mPassUtilityProvider, &mRenderPassGraph },
        mShaderManager{ commandLineParser },
        mPipelineStateManager{ &mDevice, &mShaderManager, &mResourceProducer, mRenderSurfaceDescription },
        mPipelineStateCreator{ &mPipelineStateManager },
        mRootSignatureCreator{ &mPipelineStateManager },
        mRenderDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mResourceStateTracker, &mPipelineResourceStorage, &mPipelineStateManager, &mRenderPassGraph, mRenderSurfaceDescription },
        mSwapChain{ mRenderDevice.GraphicsCommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, HAL::ColorFormat::RGBA8_Usigned_Norm, mRenderSurfaceDescription.Dimensions() },
        mFrameFence{ mDevice }
    {
        for (auto& backBufferPtr : mSwapChain.BackBuffers())
        {
            mBackBuffers.emplace_back(mResourceProducer.NewTexture(backBufferPtr.get()));
        }

        // Prepare memory to be immediately used after engine construction
        mFrameFence.IncrementExpectedValue();
        NotifyStartFrame(mFrameFence.ExpectedValue());
        mRenderDevice.AllocateUploadCommandList();
        mResourceProducer.SetCommandList(mRenderDevice.PreRenderUploadsCommandList());
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::AddRenderPass(RenderPass<ContentMediator>* pass)
    {
        mRenderPasses.emplace(pass->Metadata().Name, std::pair<RenderPass<ContentMediator>*, uint64_t>{ pass, 0 });
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
        mContentMediator = mediator;
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::CommitRenderPasses()
    {
        mRenderContexts.clear();
        mCommandRecorders.clear();
        mResourceProviders.clear();
        mRootConstantUpdaters.clear();

        mRenderContexts.reserve(mRenderPasses.size());
        mCommandRecorders.reserve(mRenderPasses.size());
        mResourceProviders.reserve(mRenderPasses.size());
        mRootConstantUpdaters.reserve(mRenderPasses.size());

        for (auto& [passName, passPtrAndContextIdx] : mRenderPasses)
        {
            auto& [passPtr, contextIdx] = passPtrAndContextIdx;

            // Populate graph with nodes first
            mRenderPassGraph.AddPass(passPtr->Metadata());
            // Run PSO setup
            passPtr->SetupPipelineStates(&mPipelineStateCreator, &mRootSignatureCreator);
        }

        auto nodeIdx = 0;

        for (auto& [passName, passPtrAndContextIdx] : mRenderPasses)
        {
            auto& [passPtr, contextIdx] = passPtrAndContextIdx;

            // Create a separate command recorder for each pass
            CommandRecorder& commandRecorder = mCommandRecorders.emplace_back(&mRenderDevice, &mRenderPassGraph.Nodes()[nodeIdx]);
            ResourceProvider& resourceProvider = mResourceProviders.emplace_back(&mPipelineResourceStorage, &mRenderPassGraph.Nodes()[nodeIdx]);
            RootConstantsUpdater& constantsUpdater = mRootConstantUpdaters.emplace_back(&mPipelineResourceStorage, &mRenderPassGraph.Nodes()[nodeIdx]);
            RenderContext<ContentMediator> context{ &commandRecorder, &constantsUpdater, &resourceProvider, &mPassUtilityProvider };
            context.SetContent(mContentMediator);
            mRenderContexts.push_back(context);
            contextIdx = mRenderContexts.size() - 1;

            ++nodeIdx;
        }

        // Make resource storage allocate necessary info using graph nodes
        mPipelineResourceStorage.CreatePerPassData();

        // Compile states
        mPipelineStateManager.CompileSignaturesAndStates();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::UploadProcessAndTransferAssets()
    {
        //// Let resources record upload commands into graphics cmd list
        ////mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        //// Run resource scheduling
        //ScheduleResources();

        //// Run asset-processing passes
        //RunAssetProcessingPasses();

        //// Upload and process assets
        //mGraphicsDevice.ExecuteCommands(nullptr, &mGraphicsFence);

        //// Execute readback commands
        //mGraphicsFence.IncrementExpectedValue();

        //// Let resources record readback commands into graphics cmd list
        ////mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

        //// Read all requested to read resources
        //mAssetStorage.ReadbackAllAssets();

        //// Perform readbacks
        //mGraphicsDevice.ExecuteCommands(nullptr, &mGraphicsFence);

        //// Wait until both devices are finished
        //mGraphicsFence.StallCurrentThreadUntilCompletion();
        //mAsyncComputeFence.StallCurrentThreadUntilCompletion();

        //mAssetStorage.ReportAllAssetsPostprocessed();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::Render()
    {
        if (mRenderPassGraph.Nodes().empty()) return;

        // First frame starts in engine constructor
        if (mFrameNumber > 0)
        {
            mFrameFence.IncrementExpectedValue();
            // Notify internal listeners
            NotifyStartFrame(mFrameFence.ExpectedValue());
            // For first frame use upload cmd list allocated in constructor
            mRenderDevice.AllocateUploadCommandList();
            mResourceProducer.SetCommandList(mRenderDevice.PreRenderUploadsCommandList());
        }

        // Recompile states that were modified
        if (mPipelineStateManager.HasModifiedStates())
        {
            mPipelineStateManager.RecompileModifiedStates();
        }

        // Scheduler resources, build graph
        ScheduleResources();

        mRenderDevice.AllocateRTASBuildsCommandList();
        mRenderDevice.AllocateWorkerCommandLists();

        // Update render device with current frame back buffer
        mRenderDevice.SetBackBuffer(mBackBuffers[mCurrentBackBufferIndex].get());

        // Notify external listeners
        mPreRenderEvent.Raise();

        // Build AS
        BuildAccelerationStructures();

        // Render
        RecordCommandLists();
        mRenderDevice.BatchCommandLists();
        mRenderDevice.UploadPassConstants();
        mRenderDevice.ExetuteCommandLists();

        // Put the picture on the screen
        mSwapChain.Present();

        // Issue a CPU wait if necessary
        mRenderDevice.GraphicsCommandQueue().SignalFence(mFrameFence);
        mFrameFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        // Notify internal listeners
        NotifyEndFrame(mFrameFence.CompletedValue());

        // Notify external listeners
        mPostRenderEvent.Raise();

        MoveToNextFrame();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::FlushAllQueuedFrames()
    {
        mFrameFence.StallCurrentThreadUntilCompletion();
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
        mPassUtilityProvider.FrameNumber = mFrameNumber;

        mBottomRTASes.clear();
        mTopRTASes.clear();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::BuildAccelerationStructures()
    {
        if (!mRenderPassGraph.FirstNodeThatUsesRayTracing())
        {
            // Skip building ray tracing acceleration structure
            // if no render passes consume them
            return;
        }

        HAL::ResourceBarrierCollection bottomRTASUABarriers{};

        for (const BottomRTAS* blas : mBottomRTASes)
        {
            bottomRTASUABarriers.AddBarrier(blas->UABarrier());
            mRenderDevice.RTASBuildsCommandList()->BuildRaytracingAccelerationStructure(blas->HALAccelerationStructure());
        }

        // Top RTAS needs to wait for Bottom RTAS
        mRenderDevice.RTASBuildsCommandList()->InsertBarriers(bottomRTASUABarriers);

        HAL::ResourceBarrierCollection topRTASUABarriers{};
        for (const TopRTAS* tlas : mTopRTASes)
        {
            topRTASUABarriers.AddBarrier(tlas->UABarrier());
            mRenderDevice.RTASBuildsCommandList()->BuildRaytracingAccelerationStructure(tlas->HALAccelerationStructure());
        }

        mRenderDevice.RTASBuildsCommandList()->InsertBarriers(topRTASUABarriers);
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::RunAssetProcessingPasses()
    {
        /*auto& nodes = mPassExecutionGraph.AssetProcessingPasses();

        for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            mGraphicsDevice.ResetViewportToDefault();
            mPipelineResourceStorage.SetCurrentRenderPassGraphNode(*nodeIt);
            mRenderPasses[nodeIt->PassMetadata.Name]->Render(&mContext);
        }*/
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::RecordCommandLists()
    {
        Memory::Texture* currentBackBuffer = mBackBuffers[mCurrentBackBufferIndex].get();
        mRenderDevice.SetBackBuffer(currentBackBuffer);
        auto& nodes = mRenderPassGraph.Nodes();

        for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            auto& [passPtr, passContextIdx] = mRenderPasses[nodeIt->PassMetadata().Name];
            RenderContext<ContentMediator>& context = mRenderContexts[passContextIdx];
            passPtr->Render(&context);
        }

        // TODO: When multi threading is implemented, insert a sync here
        // because render passes request transitions in the state tracker
        // and we treat those as prerender transitions alongside external resource 
        // upload transitions
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::ScheduleResources()
    {
        mRenderPassGraph.Clear();
        mPipelineResourceStorage.StartResourceScheduling();

        // Schedule resources and states
        for (RenderPassGraph::Node& passNode : mRenderPassGraph.Nodes())
        {
            auto& [passPtr, passContextIdx] = mRenderPasses[passNode.PassMetadata().Name];
            mResourceScheduler.SetCurrentlySchedulingPassNode(&passNode);
            passPtr->ScheduleResources(&mResourceScheduler);
        }

        mRenderPassGraph.Build();
        mPipelineResourceStorage.EndResourceScheduling();
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

