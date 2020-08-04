#include "../HardwareAbstractionLayer/DebugLayer.hpp"

#include <pix.h>

namespace PathFinder
{

    template <class ContentMediator>
    RenderEngine<ContentMediator>::RenderEngine(HWND windowHandle, const CommandLineParser& commandLineParser)
        : mRenderSurfaceDescription{ { 1920, 1080 }, HAL::ColorFormat::RGBA16_Float, HAL::DepthStencilFormat::Depth32_Float }
    {
        if (commandLineParser.ShouldEnableDebugLayer() &&
            !commandLineParser.ShouldEnableAftermath())
        {
            HAL::EnableDebugLayer();
        }

        mAftermathCrashTracker = std::make_unique<AftermathCrashTracker>(commandLineParser.ExecutableFolderPath());

        if (commandLineParser.ShouldEnableAftermath())
        {
            mAftermathCrashTracker->Initialize();
        }

        mDevice = std::make_unique<HAL::Device>(mAdapterFetcher.HardwareAdapters().front(), commandLineParser.ShouldEnableAftermath());

        if (commandLineParser.ShouldEnableAftermath())
        {
            mAftermathCrashTracker->RegisterDevice(*mDevice);
        }
        
        mPassUtilityProvider = std::make_unique<RenderPassUtilityProvider>(RenderPassUtilityProvider{ 0, mRenderSurfaceDescription });
        mResourceStateTracker = std::make_unique<Memory::ResourceStateTracker>();
        mResourceAllocator = std::make_unique<Memory::SegregatedPoolsResourceAllocator>(mDevice.get(), mSimultaneousFramesInFlight);
        mCommandListAllocator = std::make_unique<Memory::PoolCommandListAllocator>(mDevice.get(), mSimultaneousFramesInFlight);
        mDescriptorAllocator = std::make_unique<Memory::PoolDescriptorAllocator>(mDevice.get(), mSimultaneousFramesInFlight);

        mResourceProducer = std::make_unique<Memory::GPUResourceProducer>(
            mDevice.get(), 
            mResourceAllocator.get(), 
            mResourceStateTracker.get(), 
            mDescriptorAllocator.get());

        mPipelineResourceStorage = std::make_unique<PipelineResourceStorage>(
            mDevice.get(), 
            mResourceProducer.get(), 
            mDescriptorAllocator.get(), 
            mResourceStateTracker.get(), 
            mRenderSurfaceDescription, 
            &mRenderPassGraph);

        mResourceScheduler = std::make_unique<ResourceScheduler>(
            mPipelineResourceStorage.get(),
            mPassUtilityProvider.get(),
            &mRenderPassGraph);

        mShaderManager = std::make_unique<ShaderManager>(
            commandLineParser.ExecutableFolderPath(),
            commandLineParser.ShouldUseShadersFromProjectFolder(),
            commandLineParser.ShouldBuildDebugShaders(),
            &mAftermathCrashTracker->ShaderDatabase());

        mPipelineStateManager = std::make_unique<PipelineStateManager>(
            mDevice.get(),
            mShaderManager.get(), 
            mResourceProducer.get(), 
            mRenderSurfaceDescription);

        mPipelineStateCreator = std::make_unique<PipelineStateCreator>(mPipelineStateManager.get());
        mRootSignatureCreator = std::make_unique<RootSignatureCreator>(mPipelineStateManager.get());

        mRenderDevice = std::make_unique<RenderDevice>(
            *mDevice,
            &mDescriptorAllocator->CBSRUADescriptorHeap(), 
            mCommandListAllocator.get(), 
            mResourceStateTracker.get(), 
            mPipelineResourceStorage.get(), 
            mPipelineStateManager.get(), 
            &mRenderPassGraph, 
            mRenderSurfaceDescription);

        mSwapChain = std::make_unique<HAL::SwapChain>(
            mRenderDevice->GraphicsCommandQueue(),
            windowHandle,
            HAL::BackBufferingStrategy::Double, 
            HAL::ColorFormat::RGBA8_Usigned_Norm, 
            mRenderSurfaceDescription.Dimensions());

        mRenderPassContainer = std::make_unique<RenderPassContainer<ContentMediator>>(
            mRenderDevice.get(),
            mPipelineResourceStorage.get(), 
            mPassUtilityProvider.get(),
            &mRenderPassGraph);

        mFrameFence = std::make_unique<HAL::Fence>(*mDevice);

        for (auto& backBufferPtr : mSwapChain->BackBuffers())
        {
            mBackBuffers.emplace_back(mResourceProducer->NewTexture(backBufferPtr.get()));
        }

        // Prepare memory to be immediately used after engine construction
        mFrameFence->IncrementExpectedValue();
        NotifyStartFrame(mFrameFence->ExpectedValue());
        mRenderDevice->AllocateUploadCommandList();
        mResourceProducer->SetCommandList(mRenderDevice->PreRenderUploadsCommandList());
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::AddRenderPass(RenderPass<ContentMediator>* pass)
    {
        mRenderPassContainer->AddRenderPass(pass);
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
            mFrameFence->IncrementExpectedValue();
            // Notify internal listeners
            NotifyStartFrame(mFrameFence->ExpectedValue());
            // For first frame use upload cmd list allocated in constructor
            mRenderDevice->AllocateUploadCommandList();
            mResourceProducer->SetCommandList(mRenderDevice->PreRenderUploadsCommandList());
        }

        // Scheduler resources, build graph
        ScheduleFrame();

        // Compile new states and signatures, if any
        mPipelineStateManager->CompileUncompiledSignaturesAndStates();

        // Update render device with current frame back buffer
        mRenderDevice->SetBackBuffer(mBackBuffers[mCurrentBackBufferIndex].get());

        // Notify external listeners
        mPreRenderEvent.Raise();

        // Build AS
        mRenderDevice->PreRenderUploadsCommandList()->Close();
        mRenderDevice->AllocateRTASBuildsCommandList();
        BuildAccelerationStructures();

        // Render
        mRenderDevice->AllocateWorkerCommandLists();
        RecordCommandLists();
        mRenderDevice->ExecuteRenderGraph();

        // Put the picture on the screen
        mSwapChain->Present();

        // Issue a CPU wait if necessary
        mRenderDevice->GraphicsCommandQueue().SignalFence(*mFrameFence);
        mFrameFence->StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        // Notify internal listeners
        NotifyEndFrame(mFrameFence->CompletedValue());

        // Notify external listeners
        mPostRenderEvent.Raise();

        MoveToNextFrame();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::FlushAllQueuedFrames()
    {
        mFrameFence->StallCurrentThreadUntilCompletion();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::NotifyStartFrame(uint64_t newFrameNumber)
    {
        mShaderManager->BeginFrame();
        mPipelineResourceStorage->BeginFrame();
        mResourceAllocator->BeginFrame(newFrameNumber);
        mDescriptorAllocator->BeginFrame(newFrameNumber);
        mCommandListAllocator->BeginFrame(newFrameNumber);
        mResourceProducer->BeginFrame(newFrameNumber);

        mFrameStartTimestamp = std::chrono::steady_clock::now();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::NotifyEndFrame(uint64_t completedFrameNumber)
    {
        mShaderManager->EndFrame();
        mPipelineResourceStorage->EndFrame();
        mResourceProducer->EndFrame(completedFrameNumber);
        mResourceAllocator->EndFrame(completedFrameNumber);
        mDescriptorAllocator->EndFrame(completedFrameNumber);
        mCommandListAllocator->EndFrame(completedFrameNumber);

        using namespace std::chrono;
        mFrameDuration = duration_cast<microseconds>(steady_clock::now() - mFrameStartTimestamp);
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::MoveToNextFrame()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % (uint8_t)mBackBuffers.size();
        mFrameNumber++;
        mPassUtilityProvider->FrameNumber = mFrameNumber;

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
            mRenderDevice->RTASBuildsCommandList()->BuildRaytracingAccelerationStructure(blas->HALAccelerationStructure());
        }

        // Top RTAS needs to wait for Bottom RTAS
        mRenderDevice->RTASBuildsCommandList()->InsertBarriers(bottomRTASUABarriers);

        HAL::ResourceBarrierCollection topRTASUABarriers{};
        for (const TopRTAS* tlas : mTopRTASes)
        {
            topRTASUABarriers.AddBarrier(tlas->UABarrier());
            mRenderDevice->RTASBuildsCommandList()->BuildRaytracingAccelerationStructure(tlas->HALAccelerationStructure());
        }

        mRenderDevice->RTASBuildsCommandList()->InsertBarriers(topRTASUABarriers);

        mRenderDevice->RTASBuildsCommandList()->Close();
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
        mRenderDevice->SetBackBuffer(currentBackBuffer);

        auto recordCommandList = [this](auto&& passHelpers, const RenderPassGraph::Node& passNode)
        {
            mRenderDevice->RecordWorkerCommandList(passNode, [this, passHelpers]
            {
                RenderContext<ContentMediator> context = passHelpers->GetContext();
                context.SetContent(mContentMediator);
                passHelpers->Pass->Render(&context);
            });
        };

        for (const RenderPassGraph::Node* passNode : mRenderPassGraph.NodesInGlobalExecutionOrder())
        {
            if (auto passHelpers = mRenderPassContainer->GetRenderPass(passNode->PassMetadata().Name))
            {
                recordCommandList(passHelpers, *passNode);
            } 
            else if (auto passHelpers = mRenderPassContainer->GetRenderSubPass(passNode->PassMetadata().Name))
            {
                recordCommandList(passHelpers, *passNode);
            }
        }

        // TODO: When multi threading is implemented, insert a sync here
        // because render passes request transitions in the state tracker
        // and we treat those as prerender transitions alongside external resource 
        // upload transitions
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::ScheduleFrame()
    {
        mRenderPassGraph.Clear();

        // Run scheduling for standard render passes
        mPipelineResourceStorage->StartResourceScheduling();

        for (auto& [passName, passHelpers] : mRenderPassContainer->RenderPasses())
        {
            mResourceScheduler->SetCurrentlySchedulingPassNode(&mRenderPassGraph.Nodes()[passHelpers.GraphNodeIndex]);
            passHelpers.Pass->ScheduleResources(mResourceScheduler.get());

            if (!passHelpers.ArePipelineStatesScheduled)
            {
                passHelpers.Pass->SetupPipelineStates(mPipelineStateCreator.get(), mRootSignatureCreator.get());
                passHelpers.ArePipelineStatesScheduled = true;
            }
        }

        mPipelineResourceStorage->EndResourceScheduling();

        // Run scheduling for sub render passes
        mPipelineResourceStorage->StartResourceScheduling();

        for (auto& [passName, passHelpers] : mRenderPassContainer->RenderSubPasses())
        {
            mResourceScheduler->SetCurrentlySchedulingPassNode(&mRenderPassGraph.Nodes()[passHelpers.GraphNodeIndex]);
            passHelpers.Pass->ScheduleResources(mResourceScheduler.get());
        }

        mPipelineResourceStorage->EndResourceScheduling();

        // Finish graph and allocate memory 
        mRenderPassGraph.Build();
        mPipelineResourceStorage->AllocateScheduledResources();
    }

    template <class ContentMediator> 
    template <class Constants>
    void RenderEngine<ContentMediator>::SetFrameRootConstants(const Constants& constants)
    {
        mPipelineResourceStorage->UpdateFrameRootConstants(constants);
    }

    template <class ContentMediator>
    template <class Constants>
    void RenderEngine<ContentMediator>::SetGlobalRootConstants(const Constants& constants)
    {
        mPipelineResourceStorage->UpdateGlobalRootConstants(constants);
    }

}

