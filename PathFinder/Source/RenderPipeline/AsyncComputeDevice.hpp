#pragma once

#include "RenderSurfaceDescription.hpp"
#include "PipelineResourceStorage.hpp"
#include "PipelineStateManager.hpp"
#include "ResourceKey.hpp"

#include "../Foundation/Name.hpp"
#include "../Foundation/Color.hpp"
#include "../Geometry/Dimensions.hpp"
#include "../HardwareAbstractionLayer/ShaderRegister.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp"
#include "../Memory/PoolCommandListAllocator.hpp"
#include "../Memory/GPUResource.hpp"
#include "../Memory/ResourceStateTracker.hpp"

namespace PathFinder
{

    template<
        class CommandListT = HAL::ComputeCommandList,
        class CommandQueueT = HAL::ComputeCommandQueue
    >
    class AsyncComputeDevice
    {
    public:
        using CommandListPtr = Memory::PoolCommandListAllocator::CommandListPtr<CommandListT>;

        AsyncComputeDevice(
            const HAL::Device& device,
            const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
            Memory::PoolCommandListAllocator* commandListAllocator,
            Memory::ResourceStateTracker* resourceStateTracker,
            PipelineResourceStorage* resourceStorage,
            PipelineStateManager* pipelineStateManager,
            const RenderSurfaceDescription& defaultRenderSurface
        );

        virtual void ApplyPipelineState(Foundation::Name psoName);
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
        virtual void DispatchRays(uint32_t width, uint32_t height = 1, uint32_t depth = 1);
        virtual void BindBuffer(const ResourceKey& resourceKey, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        void BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        template <class T> 
        void SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace);

        void ExecuteCommands(const HAL::Fence* fenceToWaitFor = nullptr, const HAL::Fence* fenceToSignal = nullptr);

    protected:
        void AllocateNewCommandList();
        void InsertResourceTransitionsIfNeeded();

        virtual void ApplyStateIfNeeded(const HAL::ComputePipelineState* state);
        virtual void ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state, const HAL::RayDispatchInfo* dispatchInfo);

        const HAL::CBSRUADescriptorHeap* mUniversalGPUDescriptorHeap = nullptr;
        const HAL::ComputePipelineState* mAppliedComputeState = nullptr;
        const HAL::RayTracingPipelineState* mAppliedRayTracingState = nullptr;
        const HAL::RayDispatchInfo* mAppliedRayTracingDispatchInfo = nullptr;
        const HAL::RootSignature* mAppliedComputeRootSignature = nullptr;
        const HAL::Buffer* mBoundGlobalConstantBufferCompute = nullptr;
        const HAL::Buffer* mBoundFrameConstantBufferCompute = nullptr;
        const HAL::Buffer* mBoundPassDebugBufferCompute = nullptr;
        HAL::GPUAddress mBoundFrameConstantBufferAddressCompute = 0;

        Memory::PoolCommandListAllocator* mCommandListAllocator;
        Memory::ResourceStateTracker* mResourceStateTracker;
        PipelineResourceStorage* mResourceStorage;
        PipelineStateManager* mPipelineStateManager;
        RenderSurfaceDescription mDefaultRenderSurface;

        CommandQueueT mCommandQueue;
        CommandListPtr mCommandList;

    private:
        void ApplyCommonComputeResourceBindingsIfNeeded();

        bool mRebindingAfterSignatureChangeRequired = false;
        HAL::ResourceBarrierCollection mUABarriersToApply;
        Foundation::Name mLastDetectedPassName;

    public:
        inline CommandListT* CommandList() { return mCommandList.get(); }
        inline CommandQueueT& CommandQueue() { return mCommandQueue; }
    };

}

#include "AsyncComputeDevice.inl"