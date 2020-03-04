#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <chrono>

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"

#include "../Scene/Scene.hpp"
#include "../Foundation/Event.hpp"
#include "../IO/CommandLineParser.hpp"

#include "../Memory/SegregatedPoolsResourceAllocator.hpp"
#include "../Memory/PoolDescriptorAllocator.hpp"
#include "../Memory/ResourceStateTracker.hpp"
#include "../Memory/GPUResourceProducer.hpp"

#include "SceneGPUStorage.hpp"
#include "PipelineResourceStorage.hpp"
#include "PreprocessableAssetStorage.hpp"
#include "ResourceScheduler.hpp"
#include "RootConstantsUpdater.hpp"
#include "GraphicsDevice.hpp"
#include "AsyncComputeDevice.hpp"
#include "ShaderManager.hpp"
#include "PipelineStateManager.hpp"
#include "RenderContext.hpp"
#include "PipelineStateCreator.hpp"
#include "RootSignatureCreator.hpp"
#include "RenderPassExecutionGraph.hpp"
#include "GPUCommandRecorder.hpp"
#include "UIGPUStorage.hpp"
#include "BottomRTAS.hpp"
#include "TopRTAS.hpp"

namespace PathFinder
{

    template <class ContentMediator>
    class RenderPass;

    template <class ContentMediator>
    class RenderEngine
    {
    public:
        using Event = Foundation::Event<RenderEngine<ContentMediator>, std::string, void()>;

        RenderEngine(HWND windowHandle, const CommandLineParser& commandLineParser);

        void AddRenderPass(RenderPass<ContentMediator>* pass);

        void ScheduleAndAllocatePipelineResources();
        void UploadProcessAndTransferAssets();
        void Render();
        void FlushAllQueuedFrames();

        void AddBottomRayTracingAccelerationStructure(const BottomRTAS* bottomRTAS);
        void AddTopRayTracingAccelerationStructure(const TopRTAS* topRTAS);

        void SetContentMediator(ContentMediator* mediator);

        template <class Constants>
        void SetGlobalRootConstants(const Constants& constants);

        template <class Constants>
        void SetFrameRootConstants(const Constants& constants);

    private:
        HAL::DisplayAdapter FetchDefaultDisplayAdapter() const;
        void NotifyStartFrame(uint64_t newFrameNumber);
        void NotifyEndFrame(uint64_t completedFrameNumber);
        void MoveToNextFrame();
        void BuildAccelerationStructures(HAL::Fence& fenceToWaitFor, HAL::Fence& fenceToSignal);
        void RunAssetProcessingPasses();
        void RunDefaultPasses();

        RenderPassExecutionGraph mPassExecutionGraph;
        std::unordered_map<PassName, RenderPass<ContentMediator>*> mRenderPasses;

        uint8_t mCurrentBackBufferIndex = 0;
        uint8_t mSimultaneousFramesInFlight = 2;
        uint64_t mFrameNumber = 0;
        std::chrono::time_point<std::chrono::steady_clock> mFrameStartTimestamp;
        std::chrono::microseconds mFrameDuration = std::chrono::microseconds::zero();

        RenderSurfaceDescription mRenderSurfaceDescription;

        HAL::Device mDevice;

        Memory::SegregatedPoolsResourceAllocator mResourceAllocator;
        Memory::PoolCommandListAllocator mCommandListAllocator;
        Memory::PoolDescriptorAllocator mDescriptorAllocator;
        Memory::ResourceStateTracker mResourceStateTracker;
        Memory::GPUResourceProducer mResourceProducer;

        PipelineResourceStorage mPipelineResourceStorage;
        PreprocessableAssetStorage mAssetStorage;
        ResourceScheduler mResourceScheduler;
        ResourceProvider mResourceProvider;
        RootConstantsUpdater mRootConstantsUpdater;
        ShaderManager mShaderManager;
        PipelineStateManager mPipelineStateManager;
        PipelineStateCreator mPipelineStateCreator;
        RootSignatureCreator mRootSignatureCreator;
        GraphicsDevice mGraphicsDevice;
        AsyncComputeDevice<> mAsyncComputeDevice;
        GPUCommandRecorder mCommandRecorder;
        RenderContext<ContentMediator> mContext;

        HAL::Fence mGraphicsFence;
        HAL::Fence mAsyncComputeFence;
        HAL::Fence mUploadFence;

        HAL::SwapChain mSwapChain;

        Event mPreRenderEvent;
        Event mPostRenderEvent;

        std::vector<const TopRTAS*> mTopRTASes;
        std::vector<const BottomRTAS*> mBottomRTASes;

    public:
        inline PreprocessableAssetStorage& AssetStorage() { return mAssetStorage; }
        inline const RenderSurfaceDescription& RenderSurface() const { return mRenderSurfaceDescription; }
        inline Memory::GPUResourceProducer& ResourceProducer() { return mResourceProducer; }
        inline HAL::Device& Device() { return mDevice; }
        inline Event& PreRenderEvent() { return mPreRenderEvent; }
        inline Event& PostRenderEvent() { return mPostRenderEvent; }
        inline uint64_t FrameDurationMicroseconds() const { return mFrameDuration.count(); }
    };

}

#include "RenderEngine.inl"
