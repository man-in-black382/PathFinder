#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"

#include "../Scene/Scene.hpp"
#include "../Foundation/Event.hpp"
#include "../IO/CommandLineParser.hpp"

#include "../Memory/SegregatedPoolsResourceAllocator.hpp"
#include "../Memory/PoolDescriptorAllocator.hpp"
#include "../Memory/ResourceStateTracker.hpp"
#include "../Memory/GPUResourceProducer.hpp"

#include "RenderPass.hpp"
#include "MeshGPUStorage.hpp"
#include "PipelineResourceStorage.hpp"
#include "AssetResourceStorage.hpp"
#include "ResourceScheduler.hpp"
#include "RootConstantsUpdater.hpp"
#include "GraphicsDevice.hpp"
#include "AsyncComputeDevice.hpp"
#include "ShaderManager.hpp"
#include "PipelineStateManager.hpp"
#include "RenderContext.hpp"
#include "PipelineStateCreator.hpp"
#include "RenderPassExecutionGraph.hpp"
#include "GPUCommandRecorder.hpp"
#include "UIGPUStorage.hpp"

namespace PathFinder
{

    class RenderEngine
    {
    public:
        using Event = Foundation::Event<RenderEngine, std::string, void()>;

        RenderEngine(HWND windowHandle, const CommandLineParser& commandLineParser, Scene* scene, const RenderPassExecutionGraph* passExecutionGraph);

        void ScheduleAndAllocatePipelineResources();
        void ProcessAndTransferAssets();
        void Render();
        void FlushAllQueuedFrames();

    private:
        HAL::DisplayAdapter FetchDefaultDisplayAdapter() const; 
        void NotifyStartFrame(uint64_t newFrameNumber);
        void NotifyEndFrame(uint64_t completedFrameNumber);
        void MoveToNextFrame();
        void UploadCommonRootConstants();
        void UploadMeshInstanceData();
        void GatherReadbackData();

        void RunRenderPasses(const std::list<RenderPass*>& passes);

        const RenderPassExecutionGraph* mPassExecutionGraph;

        uint8_t mCurrentBackBufferIndex = 0;
        uint8_t mSimultaneousFramesInFlight = 2;
        uint64_t mFrameNumber = 0;

        RenderSurfaceDescription mDefaultRenderSurface;

        HAL::Device mDevice;

        Memory::SegregatedPoolsResourceAllocator mResourceAllocator;
        Memory::PoolCommandListAllocator mCommandListAllocator;
        Memory::PoolDescriptorAllocator mDescriptorAllocator;
        Memory::ResourceStateTracker mStateTracker;
        Memory::GPUResourceProducer mResourceProducer;

        MeshGPUStorage mMeshStorage;
        PipelineResourceStorage mPipelineResourceStorage;
        AssetResourceStorage mAssetResourceStorage;
        ResourceScheduler mResourceScheduler;
        ResourceProvider mResourceProvider;
        RootConstantsUpdater mRootConstantsUpdater;
        ShaderManager mShaderManager;
        PipelineStateManager mPipelineStateManager;
        PipelineStateCreator mPipelineStateCreator;
        GraphicsDevice mGraphicsDevice;
        AsyncComputeDevice<> mAsyncComputeDevice;
        GPUCommandRecorder mCommandRecorder;
        UIGPUStorage mUIStorage;
        RenderContext mContext;  

        HAL::Fence mGraphicsFence;
        HAL::Fence mAsyncComputeFence;
        HAL::Fence mUploadFence;

        HAL::SwapChain mSwapChain;

        Scene* mScene;

        Event mPreRenderEvent;
        Event mPostRenderEvent;

    public:
        inline MeshGPUStorage& VertexGPUStorage() { return mMeshStorage; }
        inline AssetResourceStorage& AssetGPUStorage() { return mAssetResourceStorage; }
        inline HAL::Device& Device() { return mDevice; }
        inline Event& PreRenderEvent() { return mPreRenderEvent; }
        inline Event& PostRenderEvent() { return mPostRenderEvent; }
    };

}
