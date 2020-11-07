#pragma once

#include "../RenderDevice.hpp"
#include "../PipelineResourceStorage.hpp"
#include "../RenderPassGraph.hpp"
#include "../PipelineStateManager.hpp"

#include <Memory/PoolDescriptorAllocator.hpp>

namespace PathFinder
{

    class CommandRecorder
    {
    public:
        CommandRecorder(
            RenderDevice* renderDevice, 
            PipelineResourceStorage* resourceStorage,
            PipelineStateManager* pipelineStateManager,
            Memory::PoolDescriptorAllocator* descriptorAllocator,
            const RenderPassGraph* passGraph,
            uint64_t graphNodeIndex
        );

        template <class T> 
        void SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace);

        template <size_t RTCount> 
        void SetRenderTargets(const std::array<Foundation::Name, RTCount>& rtNames, std::optional<Foundation::Name> dsName = std::nullopt);

        void SetRenderTarget(Foundation::Name rtName, std::optional<Foundation::Name> dsName = std::nullopt);
        void SetBackBufferAsRenderTarget(std::optional<Foundation::Name> dsName = std::nullopt);
        void ClearRenderTarget(Foundation::Name rtName);
        void ClearDepth(Foundation::Name rtName);
        void ApplyPipelineState(Foundation::Name psoName);
        void SetViewport(const HAL::Viewport& viewport);
        void SetScissor(const Geometry::Rect2D& scissor);

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1);
        void Draw(const DrawablePrimitive& primitive);
        void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
        void DispatchRays(const Geometry::Dimensions& dispatchDimensions);
        void Dispatch(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize);

        void BindBuffer(Foundation::Name bufferName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        void BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        
        static Geometry::Dimensions DispatchGroupCount(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize);

    private:

        void ApplyState(const HAL::GraphicsPipelineState* state);
        void ApplyState(const HAL::ComputePipelineState* state);
        void ApplyState(const HAL::RayTracingPipelineState* state, const HAL::RayDispatchInfo* dispatchInfo);

        void BindGraphicsCommonResources(const HAL::RootSignature* rootSignature, HAL::GraphicsCommandListBase* cmdList);
        void BindComputeCommonResources(const HAL::RootSignature* rootSignature, HAL::ComputeCommandListBase* cmdList);
        void BindGraphicsPassRootConstantBuffer(HAL::GraphicsCommandListBase* cmdList);
        void BindComputePassRootConstantBuffer(HAL::ComputeCommandListBase* cmdList);

        void CheckSignatureAndStatePresense(const RenderDevice::PassHelpers& passHelpers) const;

        const RenderPassGraph::Node& GetPassNode() const;
        HAL::GraphicsCommandList* GetGraphicsCommandList() const;
        HAL::ComputeCommandListBase* GetComputeCommandListBase() const;
        RenderDevice::PassHelpers& GetPassHelpers() const;

        RenderDevice* mRenderDevice;
        PipelineResourceStorage* mResourceStorage;
        PipelineStateManager* mPipelineStateManager;
        Memory::PoolDescriptorAllocator* mDescriptorAllocator;
        const RenderPassGraph* mPassGraph;
        uint64_t mGraphNodeIndex;
    };

}

#include "CommandRecorder.inl"
