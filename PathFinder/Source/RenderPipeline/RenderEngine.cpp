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
        mDefaultRenderSurface{ { 1920, 1080 }, HAL::ColorFormat::RGBA16_Float, HAL::DepthStencilFormat::Depth32_Float },
        mDevice{ FetchDefaultDisplayAdapter() },
        mResourceAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mCommandListAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mDescriptorAllocator{ &mDevice, mSimultaneousFramesInFlight },
        mResourceProducer{ &mDevice, &mResourceAllocator, &mStateTracker, &mDescriptorAllocator },
        mMeshStorage{ &mDevice, mSimultaneousFramesInFlight },
        mPipelineResourceStorage{ &mDevice, &mResourceProducer, &mDescriptorAllocator, mDefaultRenderSurface, mPassExecutionGraph },
        mResourceScheduler{ &mPipelineResourceStorage, mDefaultRenderSurface },
        mResourceProvider{ &mPipelineResourceStorage  },
        mRootConstantsUpdater{ &mPipelineResourceStorage },
        mShaderManager{ commandLineParser },
        mPipelineStateManager{ &mDevice, &mShaderManager, mDefaultRenderSurface },
        mPipelineStateCreator{ &mPipelineStateManager },
        mGraphicsDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mPipelineResourceStorage, &mPipelineStateManager, mDefaultRenderSurface },
        mAsyncComputeDevice{ mDevice, &mDescriptorAllocator.CBSRUADescriptorHeap(), &mCommandListAllocator, &mPipelineResourceStorage, &mPipelineStateManager, mDefaultRenderSurface },
        mCommandRecorder{ &mGraphicsDevice },
        mUIStorage{ &mDevice, mSimultaneousFramesInFlight },
        mContext{ scene, &mMeshStorage, &mUIStorage, &mCommandRecorder, &mRootConstantsUpdater, &mResourceProvider, mDefaultRenderSurface },
        mAsyncComputeFence{ mDevice },
        mGraphicsFence{ mDevice },
        mUploadFence{ mDevice },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, HAL::ColorFormat::RGBA8_Usigned_Norm, mDefaultRenderSurface.Dimensions() },
        mScene{ scene }
    {
        mPipelineResourceStorage.CreateSwapChainBackBufferDescriptors(mSwapChain);
        mAsyncComputeDevice.AllocateNewCommandList();
        mGraphicsDevice.AllocateNewCommandList();
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
        mMeshStorage.UploadVertices();
        mMeshStorage.CreateBottomAccelerationStructures();

        // Run setup and asset-processing passes
        RunRenderPasses(mPassExecutionGraph->OneTimePasses());

        // Upload and process assets
        mGraphicsDevice.ExecuteCommands(nullptr, &mGraphicsFence);

        // Build BLASes once 
        for (const HAL::RayTracingBottomAccelerationStructure& blas : mMeshStorage.BottomAccelerationStructures())
        {
            mAsyncComputeDevice.CommandList()->BuildRaytracingAccelerationStructure(blas);
        }

        mAsyncComputeDevice.CommandList()->InsertBarriers(mMeshStorage.BottomASBuildBarriers());
        mAsyncComputeDevice.ExecuteCommands(&mGraphicsFence, &mAsyncComputeFence);

        // Execute readback commands
        mGraphicsFence.IncrementExpectedValue();
        mAssetStorage.ReadbackAllAssets();
        mGraphicsDevice.ExecuteCommands(nullptr, &mGraphicsFence);

        // Wait until both devices are finished
        mGraphicsFence.StallCurrentThreadUntilCompletion();
        mAsyncComputeFence.StallCurrentThreadUntilCompletion();

        mAssetStorage.ReportAllAssetsPostprocessed();
    }

    void RenderEngine::Render()
    {
        if (mPassExecutionGraph->DefaultPasses().empty()) return;

        mAsyncComputeFence.IncrementExpectedValue();
        mGraphicsFence.IncrementExpectedValue();
        mUploadFence.IncrementExpectedValue();

        NotifyStartFrame(mGraphicsFence.ExpectedValue());

        mPreRenderEvent.Raise();

        mUIStorage.UploadUI();
        UploadCommonRootConstants();
        UploadMeshInstanceData();

        // Execute any pending upload commands 
        mGraphicsDevice.ExecuteCommands(nullptr, &mUploadFence);

        // Build AS
        mMeshStorage.CreateTopAccelerationStructure();
        mAsyncComputeDevice.CommandList()->BuildRaytracingAccelerationStructure(mMeshStorage.TopAccelerationStructure());
        mAsyncComputeDevice.CommandList()->InsertBarriers(mMeshStorage.TopASBuildBarriers());
        mAsyncComputeDevice.ExecuteCommands(&mUploadFence, &mAsyncComputeFence);

        // Wait for TLAS build then execute render passes
        HAL::Texture* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();
        HAL::ResourceTransitionBarrier preRenderBarrier{ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer };
        HAL::ResourceTransitionBarrier postRenderBarrier{ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer };

        // Execute render passes
        mGraphicsDevice.CommandList()->InsertBarrier(preRenderBarrier);
        RunRenderPasses(mPassExecutionGraph->DefaultPasses());
        mGraphicsDevice.CommandList()->InsertBarrier(postRenderBarrier);

        mGraphicsDevice.ExecuteCommands(&mAsyncComputeFence, &mGraphicsFence);
        mSwapChain.Present();

        mGraphicsFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        mPostRenderEvent.Raise();
        NotifyEndFrame(mGraphicsFence.CompletedValue());

        GatherReadbackData();

        if (mPipelineStateManager.HasModifiedStates())
        {
            FlushAllQueuedFrames();
            mPipelineStateManager.RecompileModifiedStates();
        }

        // Discard old command lists, get new ones, propagate to resources
        mAsyncComputeDevice.AllocateNewCommandList();
        mGraphicsDevice.AllocateNewCommandList();
        mResourceProducer.SetCommandList(mGraphicsDevice.CommandList());

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
        mResourceAllocator.BeginFrame(newFrameNumber);
        mDescriptorAllocator.BeginFrame(newFrameNumber);
        mCommandListAllocator.BeginFrame(newFrameNumber);
        mShaderManager.BeginFrame();
        mResourceProducer.BeginFrame(newFrameNumber);
        mMeshStorage.BeginFrame(newFrameNumber);
        mUIStorage.BeginFrame(newFrameNumber);
    }

    void RenderEngine::NotifyEndFrame(uint64_t completedFrameNumber)
    {
        mShaderManager.EndFrame();
        mMeshStorage.EndFrame(completedFrameNumber);
        mUIStorage.EndFrame(completedFrameNumber);
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

    void RenderEngine::UploadCommonRootConstants()
    {
      /*  GlobalRootConstants* globalConstants = mPipelineResourceStorage.GlobalRootConstantData();
        PerFrameRootConstants* perFrameConstants = mPipelineResourceStorage.PerFrameRootConstantData();

        globalConstants->PipelineRTResolution = { mDefaultRenderSurface.Dimensions().Width, mDefaultRenderSurface.Dimensions().Height };

        const Camera& camera = mScene->MainCamera();

        perFrameConstants->CameraPosition = glm::vec4{ camera.Position(), 1.0 };
        perFrameConstants->CameraView = camera.View();
        perFrameConstants->CameraProjection = camera.Projection();
        perFrameConstants->CameraViewProjection = camera.ViewProjection();
        perFrameConstants->CameraInverseView = camera.InverseView();
        perFrameConstants->CameraInverseProjection = camera.InverseProjection();
        perFrameConstants->CameraInverseViewProjection = camera.InverseViewProjection();*/
    }

    void RenderEngine::UploadMeshInstanceData()
    {
        auto iterator = [&](MeshInstance& instance)
        {
            auto blasIndex = instance.AssosiatedMesh()->LocationInVertexStorage().BottomAccelerationStructureIndex;
            auto& blas = mMeshStorage.BottomAccelerationStructures()[blasIndex];
            mMeshStorage.StoreMeshInstance(instance, blas);
        };

        mScene->IterateMeshInstances(iterator);
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
            mGraphicsDevice.CommandList()->InsertBarriers(mPipelineResourceStorage.AliasingBarriersForCurrentPass());

            passPtr->Render(&mContext);

            // Queue debug buffer read
          /*  auto debugBuffer = mPipelineResourceStorage.DebugBufferForCurrentPass();
            auto readackDebugBuffer = mPipelineResourceStorage.DebugReadbackBufferForCurrentPass();
            mReadbackCopyDevice.QueueBufferToBufferCopy(*debugBuffer, *readackDebugBuffer, 0, debugBuffer->Capacity(), readackDebugBuffer->CurrentFrameObjectOffset());*/
        }
    }

}
