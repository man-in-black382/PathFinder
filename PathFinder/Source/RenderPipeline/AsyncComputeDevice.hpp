#pragma once

#include "RenderSurfaceDescription.hpp"
#include "PipelineResourceStorage.hpp"
#include "PipelineStateManager.hpp"

#include "../Foundation/Name.hpp"
#include "../Foundation/Color.hpp"
#include "../Geometry/Dimensions.hpp"
#include "../HardwareAbstractionLayer/ShaderRegister.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/RingCommandList.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp"

namespace PathFinder
{

    template<
        class CommandListT = HAL::ComputeCommandList,
        class CommandAllocatorT = HAL::ComputeCommandAllocator,
        class CommandQueueT = HAL::ComputeCommandQueue
    >
    class AsyncComputeDevice
    {
    public:
        AsyncComputeDevice(
            const HAL::Device& device,
            const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
            PipelineResourceStorage* resourceStorage,
            PipelineStateManager* pipelineStateManager,
            const RenderSurfaceDescription& defaultRenderSurface,
            uint8_t simultaneousFramesInFlight
        );

        virtual void ApplyPipelineState(Foundation::Name psoName);
        virtual void WaitUntilUnorderedAccessesComplete(Foundation::Name resourceName);
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);

        virtual void BindBuffer(Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        template <class T>
        void BindExternalBuffer(const HAL::BufferResource<T>& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        void ClearUnorderedAccessResource(Foundation::Name resourceName, const glm::vec4& clearValue);
        void ClearUnorderedAccessResource(Foundation::Name resourceName, const glm::uvec4& clearValue);

        void WaitFence(HAL::Fence& fence);
        void ExecuteCommands();
        void ResetCommandList();
        void SignalFence(HAL::Fence& fence);

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

    protected:
        virtual void ApplyStateIfNeeded(const HAL::ComputePipelineState* state);
        virtual void ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state);

        const HAL::CBSRUADescriptorHeap* mUniversalGPUDescriptorHeap;
        const HAL::ComputePipelineState* mAppliedComputeState;
        const HAL::RayTracingPipelineState* mAppliedRayTracingState;

        const HAL::RootSignature* mAppliedComputeRootSignature;

        PipelineResourceStorage* mResourceStorage;
        PipelineStateManager* mPipelineStateManager;
        RenderSurfaceDescription mDefaultRenderSurface;

        CommandQueueT mCommandQueue;

    private:
        void ApplyCommonComputeResourceBindings();
        void BindCurrentPassConstantBufferCompute();

        const HAL::Device* mDevice;
        HAL::RingCommandList<CommandListT, CommandAllocatorT> mRingCommandList;

    public:
        inline CommandListT& CommandList() { return mRingCommandList.CurrentCommandList(); }
        inline CommandQueueT& CommandQueue() { return mCommandQueue; }
    };

}

#include "AsyncComputeDevice.inl"