#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

#include "../Scene/Scene.hpp"

#include "RenderPass.hpp"
#include "VertexStorage.hpp"
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

namespace PathFinder
{

    class RenderEngine
    {
    public:
        RenderEngine(HWND windowHandle, const std::filesystem::path& executablePath,
            Scene* scene, const RenderPassExecutionGraph* passExecutionGraph);

        void ScheduleAndAllocatePipelineResources();
        void ProcessAndTransferAssets();
        void Render();

    private:
        HAL::DisplayAdapter FetchDefaultDisplayAdapter() const; 
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
        std::filesystem::path mExecutablePath;

        HAL::Device mDevice;
        HAL::Fence mFrameFence;
        HAL::Fence mAccelerationStructureFence;

        // Device to copy everything non-processable at the beginning of the frame
        CopyDevice mStandardCopyDevice;

        // Device to copy resources that first need to be preprocessed
        // by asset-preprocessing render passes
        CopyDevice mAssetPostprocessCopyDevice;

        VertexStorage mVertexStorage;
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
        AsyncComputeDevice mAsyncComputeDevice;
        RenderContext mContext;  

        HAL::SwapChain mSwapChain;

        Scene* mScene;

    public:
        inline VertexStorage& VertexGPUStorage() { return mVertexStorage; }
        inline AssetResourceStorage& AssetGPUStorage() { return mAssetResourceStorage; }
        inline CopyDevice& StandardCopyDevice() { return mStandardCopyDevice; }
        inline HAL::Device& Device() { return mDevice; }
    };

}
