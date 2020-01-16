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
        mDefaultRenderSurface{ { 1920, 1080 }, HAL::ColorFormat::RGBA16_Float, HAL::DepthStencilFormat::Depth32_Float },
        mDevice{ FetchDefaultDisplayAdapter() },
        mUploadFence{ mDevice },
        mAsyncComputeFence{ mDevice },
        mGraphicsFence{ mDevice },
        mReadbackFence{ mDevice },
        mUploadCopyDevice{ &mDevice, mSimultaneousFramesInFlight },
        mReadbackCopyDevice{ &mDevice, mSimultaneousFramesInFlight },
        mMeshStorage{ &mDevice, &mUploadCopyDevice, mSimultaneousFramesInFlight },
        mDescriptorStorage{ &mDevice },
        mPipelineResourceStorage{ &mDevice, &mDescriptorStorage, mDefaultRenderSurface, mSimultaneousFramesInFlight, mPassExecutionGraph },
        mAssetResourceStorage{ &mDevice, &mReadbackCopyDevice, &mDescriptorStorage},
        mResourceScheduler{ &mPipelineResourceStorage, mDefaultRenderSurface },
        mResourceProvider{ &mPipelineResourceStorage, &mDescriptorStorage },
        mRootConstantsUpdater{ &mPipelineResourceStorage },
        mShaderManager{ commandLineParser },
        mPipelineStateManager{ &mDevice, &mShaderManager, mDefaultRenderSurface },
        mPipelineStateCreator{ &mPipelineStateManager },
        mGraphicsDevice{ mDevice, &mDescriptorStorage.CBSRUADescriptorHeap(), &mPipelineResourceStorage, &mPipelineStateManager, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mAsyncComputeDevice{ mDevice, &mDescriptorStorage.CBSRUADescriptorHeap(), &mPipelineResourceStorage, &mPipelineStateManager, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mCommandRecorder{ &mGraphicsDevice },
        mUIStorage{ &mDevice, &mUploadCopyDevice, &mDescriptorStorage, mSimultaneousFramesInFlight },
        mContext{ scene, &mMeshStorage, &mUIStorage, &mCommandRecorder, &mRootConstantsUpdater, &mResourceProvider, mDefaultRenderSurface },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, HAL::ColorFormat::RGBA8_Usigned_Norm, mDefaultRenderSurface.Dimensions() },
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
        mUploadFence.IncrementExpectedValue();
        mAsyncComputeFence.IncrementExpectedValue();
        mGraphicsFence.IncrementExpectedValue();
        mReadbackFence.IncrementExpectedValue();

        // Allocate and transfer GPU memory, compile states, perform initial resource transitions
        mMeshStorage.UploadVerticesAndQueueForCopy();
        mMeshStorage.CreateBottomAccelerationStructures();

        mUploadCopyDevice.ExecuteCommands(nullptr, &mUploadFence);

        // Run setup and asset-processing passes
        RunRenderPasses(mPassExecutionGraph->OneTimePasses());

        // Transition resources after asset-processing passes completed their work
        mGraphicsDevice.CommandList().InsertBarriers(mAssetResourceStorage.AssetPostProcessingBarriers());
        mGraphicsDevice.ExecuteCommands(&mUploadFence, &mGraphicsFence);

        // Build BLASes once 
        for (const HAL::RayTracingBottomAccelerationStructure& blas : mMeshStorage.BottomAccelerationStructures())
        {
            mAsyncComputeDevice.CommandList().BuildRaytracingAccelerationStructure(blas);
        }

        mAsyncComputeDevice.CommandList().InsertBarriers(mMeshStorage.BottomASBuildBarriers());
        mAsyncComputeDevice.ExecuteCommands(&mGraphicsFence, &mAsyncComputeFence);

        // Read processed assets back if needed
        mReadbackCopyDevice.ExecuteCommands(&mAsyncComputeFence, &mReadbackFence);

        mReadbackFence.StallCurrentThreadUntilCompletion();

        // Unlock devices before rendering starts
        mUploadCopyDevice.ResetCommandList();
        mReadbackCopyDevice.ResetCommandList();
        mGraphicsDevice.ResetCommandList();
        mAsyncComputeDevice.ResetCommandList();
    }

    void RenderEngine::Render()
    {
        if (mPassExecutionGraph->DefaultPasses().empty()) return;
                
        mUploadFence.IncrementExpectedValue();
        mAsyncComputeFence.IncrementExpectedValue();
        mGraphicsFence.IncrementExpectedValue();
        mReadbackFence.IncrementExpectedValue();

        NotifyStartFrame(mReadbackFence.ExpectedValue());

        // Readback ring buffers moved to safe-to-read memory region.
        // Read safe region before scheduling more rendering commands.
        GatherReadbackData();

        mPreRenderEvent.Raise();

        mUIStorage.UploadUI();
        UploadCommonRootConstants();
        UploadMeshInstanceData();

        // Upload any data necessary.
        // Upload is not dependent on the last frame.
        mUploadCopyDevice.ExecuteCommands(nullptr, &mUploadFence);

        // Build AS
        mMeshStorage.CreateTopAccelerationStructure();
        mAsyncComputeDevice.CommandList().BuildRaytracingAccelerationStructure(mMeshStorage.TopAccelerationStructure());
        mAsyncComputeDevice.CommandList().InsertBarriers(mMeshStorage.TopASBuildBarriers());
        mAsyncComputeDevice.ExecuteCommands(&mUploadFence, &mAsyncComputeFence);

        // Wait for TLAS build then execute render passes
        HAL::Texture* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();
        HAL::ResourceTransitionBarrier preRenderBarrier{ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer };
        HAL::ResourceTransitionBarrier postRenderBarrier{ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer };

        // Execute render passes
        mGraphicsDevice.CommandList().InsertBarrier(preRenderBarrier);
        RunRenderPasses(mPassExecutionGraph->DefaultPasses());
        mGraphicsDevice.CommandList().InsertBarrier(postRenderBarrier);

        mGraphicsDevice.ExecuteCommands(&mAsyncComputeFence, &mGraphicsFence);
        mSwapChain.Present();

        // Wait for render passes then read back any data needed
        mReadbackCopyDevice.ExecuteCommands(&mGraphicsFence, &mReadbackFence);

        // Wait for read back then finish frame. Stall if N frames are in flight simultaneously.
        mReadbackFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        mPostRenderEvent.Raise();
        NotifyEndFrame(mReadbackFence.CompletedValue());

        if (mPipelineStateManager.HasModifiedStates())
        {
            FlushAllQueuedFrames();
            mPipelineStateManager.RecompileModifiedStates();
        }

        MoveToNextFrame();
    }

    void RenderEngine::FlushAllQueuedFrames()
    {
        mReadbackFence.StallCurrentThreadUntilCompletion();
    }

    HAL::DisplayAdapter RenderEngine::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch()[0];// .back();
    }

    void RenderEngine::NotifyStartFrame(uint64_t newFrameNumber)
    {
        mShaderManager.BeginFrame();
        mPipelineResourceStorage.BeginFrame(newFrameNumber);
        mGraphicsDevice.BeginFrame(newFrameNumber);
        mAsyncComputeDevice.BeginFrame(newFrameNumber);
        mUploadCopyDevice.BeginFrame(newFrameNumber);
        mReadbackCopyDevice.BeginFrame(newFrameNumber);
        mMeshStorage.BeginFrame(newFrameNumber);
        mUIStorage.BeginFrame(newFrameNumber);
    }

    void RenderEngine::NotifyEndFrame(uint64_t completedFrameNumber)
    {
        mShaderManager.EndFrame();
        mPipelineResourceStorage.EndFrame(completedFrameNumber);
        mGraphicsDevice.EndFrame(completedFrameNumber);
        mAsyncComputeDevice.EndFrame(completedFrameNumber);
        mUploadCopyDevice.EndFrame(completedFrameNumber);
        mReadbackCopyDevice.EndFrame(completedFrameNumber);
        mMeshStorage.EndFrame(completedFrameNumber);
        mUIStorage.EndFrame(completedFrameNumber);
    }

    void RenderEngine::MoveToNextFrame()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % (uint8_t)mSwapChain.BackBuffers().size();
        mPipelineResourceStorage.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
        mFrameNumber++;
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

        mPipelineResourceStorage.IterateDebugBuffers([this](PassName passName, const HAL::Buffer<float>* debugBuffer, const HAL::RingBufferResource<float>* debugReadbackBuffer) 
        {
            mUIStorage.ReadbackPassDebugBuffer(passName, *debugReadbackBuffer);
        });
    }

    void RenderEngine::RunRenderPasses(const std::list<RenderPass*>& passes)
    {
        for (auto passPtr : passes)
        {
            mGraphicsDevice.ResetViewportToDefault();
            mPipelineResourceStorage.SetCurrentPassName(passPtr->Name());
            mGraphicsDevice.CommandList().InsertBarriers(mPipelineResourceStorage.TransitionAndAliasingBarriersForCurrentPass());

            passPtr->Render(&mContext);

            // Queue debug buffer read
            auto debugBuffer = mPipelineResourceStorage.DebugBufferForCurrentPass();
            auto readackDebugBuffer = mPipelineResourceStorage.DebugReadbackBufferForCurrentPass();
            mReadbackCopyDevice.QueueBufferToBufferCopy(*debugBuffer, *readackDebugBuffer, 0, debugBuffer->Capacity(), readackDebugBuffer->CurrentFrameObjectOffset());
        }
    }

}
