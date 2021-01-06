#include <HardwareAbstractionLayer/DebugLayer.hpp>
#include "CopyRequestHandling.hpp"

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

        HAL::DisplayAdapter* hwAdapter = &mAdapterFetcher.GetHardwareAdapter(0);
        mSelectedAdapter = hwAdapter;

        mDevice = std::make_unique<HAL::Device>(*hwAdapter, commandLineParser.ShouldEnableAftermath());

        if (commandLineParser.ShouldEnableAftermath())
        {
            mAftermathCrashTracker->RegisterDevice(*mDevice);
        }
        
        mPassUtilityProvider = std::make_unique<RenderPassUtilityProvider>(RenderPassUtilityProvider{ 0, mRenderSurfaceDescription });
        mResourceStateTracker = std::make_unique<Memory::ResourceStateTracker>();
        mResourceAllocator = std::make_unique<Memory::SegregatedPoolsResourceAllocator>(mDevice.get(), mSimultaneousFramesInFlight);
        mCommandListAllocator = std::make_unique<Memory::PoolCommandListAllocator>(mDevice.get(), mSimultaneousFramesInFlight);
        mDescriptorAllocator = std::make_unique<Memory::PoolDescriptorAllocator>(mDevice.get(), mSimultaneousFramesInFlight);
        mCopyRequestManager = std::make_unique<Memory::CopyRequestManager>();

        mResourceProducer = std::make_unique<Memory::GPUResourceProducer>(
            mDevice.get(), 
            mResourceAllocator.get(), 
            mResourceStateTracker.get(), 
            mDescriptorAllocator.get(),
            mCopyRequestManager.get());

        mPipelineResourceStorage = std::make_unique<PipelineResourceStorage>(
            mDevice.get(), 
            mResourceProducer.get(), 
            mDescriptorAllocator.get(), 
            mResourceStateTracker.get(), 
            mRenderSurfaceDescription, 
            &mRenderPassGraph);

        mPipelineResourceStorage->SetMemoryAliasingEnabled(!commandLineParser.DisableMemoryAliasing());

        mResourceScheduler = std::make_unique<ResourceScheduler>(
            mPipelineResourceStorage.get(),
            mPassUtilityProvider.get(),
            &mRenderPassGraph);

        mShaderManager = std::make_unique<ShaderManager>(
            commandLineParser.ExecutableFolderPath(),
            commandLineParser.ShouldUseShadersFromProjectFolder(),
            commandLineParser.ShouldBuildDebugShaders(),
            commandLineParser.ShouldEnableAftermath(),
            &mAftermathCrashTracker->ShaderDatabase());

        mPipelineStateManager = std::make_unique<PipelineStateManager>(
            mDevice.get(),
            mShaderManager.get(), 
            mResourceProducer.get(), 
            mRenderSurfaceDescription);

        mPipelineStateCreator = std::make_unique<PipelineStateCreator>(mPipelineStateManager.get());
        mRootSignatureCreator = std::make_unique<RootSignatureCreator>(mPipelineStateManager.get());
        mSamplerCreator = std::make_unique<SamplerCreator>(mPipelineResourceStorage.get());
        mGPUProfiler = std::make_unique<GPUProfiler>(*mDevice, 1024, mSimultaneousFramesInFlight, mResourceProducer.get());

        mRenderDevice = std::make_unique<RenderDevice>(
            *mDevice,
            mDescriptorAllocator.get(),
            mCommandListAllocator.get(), 
            mResourceStateTracker.get(), 
            mCopyRequestManager.get(),
            mPipelineResourceStorage.get(), 
            mPipelineStateManager.get(), 
            mGPUProfiler.get(),
            &mRenderPassGraph, 
            mRenderSurfaceDescription);

        mSwapChain = std::make_unique<HAL::SwapChain>(
            &hwAdapter->Displays().front(),
            mRenderDevice->GraphicsCommandQueue(),
            windowHandle,
            true,
            HAL::BackBufferingStrategy::Double, 
            mRenderSurfaceDescription.Dimensions());

        mRenderPassContainer = std::make_unique<RenderPassContainer<ContentMediator>>(
            mRenderDevice.get(),
            mPipelineResourceStorage.get(), 
            mPassUtilityProvider.get(),
            mPipelineStateManager.get(),
            mDescriptorAllocator.get(),
            &mRenderPassGraph);

        mSubPassScheduler = std::make_unique<SubPassScheduler<ContentMediator>>(
            mRenderPassContainer.get(),
            mPipelineResourceStorage.get(),
            mPassUtilityProvider.get());

        mFrameFence = std::make_unique<FrameFence>(*mDevice);

        // Start first frame here to prepare engine for external data transfer requests
        mFrameFence->HALFence().IncrementExpectedValue();
        NotifyStartFrame(mFrameFence->HALFence().ExpectedValue());
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
    void RenderEngine<ContentMediator>::Render()
    {
        if (mRenderPassGraph.Nodes().empty()) return;

        // First frame statrts in constructor
        if (mFrameNumber > 0)
        {
            mFrameFence->HALFence().IncrementExpectedValue();
            // Notify internal listeners
            NotifyStartFrame(mFrameFence->HALFence().ExpectedValue());
        }

        // Scheduler resources, build graph
        ScheduleFrame();

        // Compile new states and signatures, if any
        mPipelineStateManager->CompileUncompiledSignaturesAndStates();

        // Notify external listeners
        mPreRenderEvent.Raise();

        // External listeners might've caused back buffer reallocation
        if (mSwapChain->AreBackBuffersUpdated())
            UpdateBackBuffers();

        // Update render device with current frame back buffer
        mRenderDevice->SetBackBuffer(mBackBuffers[mCurrentBackBufferIndex].get());

        UploadAssets();
        BuildAccelerationStructures();

        // Render
        mRenderDevice->AllocateWorkerCommandLists();
        RecordCommandLists();
        mRenderDevice->ExecuteRenderGraph();

        // Put the picture on the screen
        mSwapChain->Present();

        // Issue a CPU wait if necessary
        mRenderDevice->GraphicsCommandQueue().SignalFence(mFrameFence->HALFence());
        mFrameFence->StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        // Notify internal listeners
        NotifyEndFrame(mFrameFence->HALFence().CompletedValue());

        // Gather extracted measurement
        mRenderDevice->GatherMeasurements();

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
        mResourceAllocator->BeginFrame(newFrameNumber);
        mDescriptorAllocator->BeginFrame(newFrameNumber);
        mCommandListAllocator->BeginFrame(newFrameNumber);
        mResourceProducer->BeginFrame(newFrameNumber);
        mPipelineResourceStorage->BeginFrame();
        mGPUProfiler->BeginFrame(newFrameNumber);

        mFrameStartTimestamp = std::chrono::steady_clock::now();
    }

    template <class ContentMediator>
    void RenderEngine<ContentMediator>::NotifyEndFrame(uint64_t completedFrameNumber)
    {
        mShaderManager->EndFrame();
        mResourceProducer->EndFrame(completedFrameNumber);
        mResourceAllocator->EndFrame(completedFrameNumber);
        mDescriptorAllocator->EndFrame(completedFrameNumber);
        mCommandListAllocator->EndFrame(completedFrameNumber);
        mPipelineResourceStorage->EndFrame();
        mGPUProfiler->EndFrame(completedFrameNumber);

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
    void RenderEngine<ContentMediator>::UploadAssets()
    {
        mRenderDevice->AllocateUploadCommandList();
        RecordUploadRequests(*mRenderDevice->PreRenderUploadsCommandList(), *mResourceStateTracker, *mCopyRequestManager, true);
        mRenderDevice->PreRenderUploadsCommandList()->Close();

        assert_format(mCopyRequestManager->ReadbackRequests().empty(), "We shouldn't have any readback requests at this stage");
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

        mRenderDevice->AllocateRTASBuildsCommandList();

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

            if (!passHelpers.AreSamplersScheduled)
            {
                passHelpers.Pass->ScheduleSamplers(mSamplerCreator.get());
                passHelpers.AreSamplersScheduled = true;
            }
        }

        mPipelineResourceStorage->EndResourceScheduling();

        // Run sub pass scheduling after scheduling first wave of resources
        for (auto& [passName, passHelpers] : mRenderPassContainer->RenderPasses())
        {
            passHelpers.Pass->ScheduleSubPasses(mSubPassScheduler.get());
        }

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
    void RenderEngine<ContentMediator>::UpdateBackBuffers()
    {
        // Because this function is called before rendering, but after new frame fence increase,
        // we pass 2 instead of 1 to stall CPU thread
        mFrameFence->StallCurrentThreadUntilCompletion(2);
        mBackBuffers.clear();

        for (auto& backBufferPtr : mSwapChain->BackBuffers())
        {
            mBackBuffers.emplace_back(mResourceProducer->NewTexture(backBufferPtr.get()));
        }
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

