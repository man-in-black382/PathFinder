#pragma once

#include "RenderSurfaceDescription.hpp"
#include "PipelineResourceStorage.hpp"
#include "PipelineStateManager.hpp"

#include "../Foundation/Name.hpp"
#include "../Foundation/Color.hpp"
#include "../Geometry/Dimensions.hpp"
#include "../HardwareAbstractionLayer/ShaderRegister.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp"
#include "../Memory/PoolCommandListAllocator.hpp"
#include "../Memory/GPUResource.hpp"

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
            PipelineResourceStorage* resourceStorage,
            PipelineStateManager* pipelineStateManager,
            const RenderSurfaceDescription& defaultRenderSurface
        );

        virtual void ApplyPipelineState(Foundation::Name psoName);
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
        virtual void BindBuffer(Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        void BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        template <class T>
        void SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace);

        void ExecuteCommands(const HAL::Fence* fenceToWaitFor = nullptr, const HAL::Fence* fenceToSignal = nullptr);

    protected:
        void AllocateNewCommandList();

        virtual void ApplyStateIfNeeded(const HAL::ComputePipelineState* state);
        virtual void ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state);

        const HAL::CBSRUADescriptorHeap* mUniversalGPUDescriptorHeap = nullptr;
        const HAL::ComputePipelineState* mAppliedComputeState = nullptr;
        const HAL::RayTracingPipelineState* mAppliedRayTracingState = nullptr;
        const HAL::RootSignature* mAppliedComputeRootSignature = nullptr;
        const HAL::Buffer* mBoundGlobalConstantBufferCompute = nullptr;
        const HAL::Buffer* mBoundFrameConstantBufferCompute = nullptr;
        const HAL::Buffer* mBoundPassConstantBufferCompute = nullptr;
        const HAL::Buffer* mBoundPassDebugBufferCompute = nullptr;

        Memory::PoolCommandListAllocator* mCommandListAllocator;
        PipelineResourceStorage* mResourceStorage;
        PipelineStateManager* mPipelineStateManager;
        RenderSurfaceDescription mDefaultRenderSurface;

        CommandQueueT mCommandQueue;
        CommandListPtr mCommandList;

    private:
        void ApplyCommonComputeResourceBindingsIfNeeded();

        bool mRebindingAfterSignatureChangeRequired = false;

    public:
        inline CommandListT* CommandList() { return mCommandList.get(); }
        inline CommandQueueT& CommandQueue() { return mCommandQueue; }
    };

}

#include "AsyncComputeDevice.inl"