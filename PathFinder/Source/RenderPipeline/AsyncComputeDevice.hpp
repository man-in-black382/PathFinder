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
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
        virtual void BindBuffer(Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        void BindExternalBuffer(const HAL::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        template <class T>
        void SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace);

        void ExecuteCommands(const HAL::Fence* fenceToWaitFor = nullptr, const HAL::Fence* fenceToSignal = nullptr);
        void ResetCommandList();

        void BeginFrame(uint64_t newFrameNumber);
        void EndFrame(uint64_t completedFrameNumber);

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
        HAL::RingCommandList<CommandListT, CommandAllocatorT> mRingCommandList;

    private:
        void ApplyCommonComputeResourceBindings();
        void BindCurrentPassBuffersCompute();

    public:
        inline CommandListT& CommandList() { return mRingCommandList.CurrentCommandList(); }
        inline CommandQueueT& CommandQueue() { return mCommandQueue; }
    };

}

#include "AsyncComputeDevice.inl"