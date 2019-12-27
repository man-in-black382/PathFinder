#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

#include "../Scene/Scene.hpp"
#include "../Foundation/Event.hpp"
#include "../IO/CommandLineParser.hpp"

#include "RenderPass.hpp"
#include "MeshGPUStorage.hpp"
#include "PipelineResourceStorage.hpp"
#include "AssetResourceStorage.hpp"
#include "ResourceScheduler.hpp"
#include "ResourceDescriptorStorage.hpp"
#include "RootConstantsUpdater.hpp"
#include "GraphicsDevice.hpp"
#include "AsyncComputeDevice.hpp"
#include "ShaderManager.hpp"
#include "PipelineStateManager.hpp"
#include "RenderContext.hpp"
#include "PipelineStateCreator.hpp"
#include "RenderPassExecutionGraph.hpp"
#include "CopyDevice.hpp"
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
        void NotifyStartFrame();
        void NotifyEndFrame();
        void MoveToNextBackBuffer();
        void BuildBottomAccelerationStructures();
        void BuildTopAccelerationStructures();
        void UploadCommonRootConstants();
        void UploadMeshInstanceData();

        void RunRenderPasses(const std::list<RenderPass*>& passes);

        const RenderPassExecutionGraph* mPassExecutionGraph;

        uint8_t mCurrentBackBufferIndex = 0;
        uint8_t mSimultaneousFramesInFlight = 3;

        RenderSurfaceDescription mDefaultRenderSurface;

        HAL::Device mDevice;
        HAL::Fence mFrameFence;
        HAL::Fence mAccelerationStructureFence;

        // Device to copy everything non-processable at the beginning of the frame
        CopyDevice mUploadCopyDevice;

        // Device to copy resources that first need to be preprocessed
        // by asset-preprocessing render passes
        CopyDevice mReadbackCopyDevice;

        MeshGPUStorage mMeshStorage;
        ResourceDescriptorStorage mDescriptorStorage;
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

        HAL::SwapChain mSwapChain;

        Scene* mScene;

        Event mPreRenderEvent;
        Event mPostRenderEvent;

    public:
        inline MeshGPUStorage& VertexGPUStorage() { return mMeshStorage; }
        inline AssetResourceStorage& AssetGPUStorage() { return mAssetResourceStorage; }
        inline CopyDevice& StandardCopyDevice() { return mUploadCopyDevice; }
        inline HAL::Device& Device() { return mDevice; }
        inline Event& PreRenderEvent() { return mPreRenderEvent; }
        inline Event& PostRenderEvent() { return mPostRenderEvent; }
    };

}
