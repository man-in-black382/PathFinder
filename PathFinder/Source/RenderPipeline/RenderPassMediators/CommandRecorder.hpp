#pragma once

#include "../RenderDevice.hpp"
#include "../RenderPassGraph.hpp"

namespace PathFinder
{

    class CommandRecorder
    {
    public:
        CommandRecorder(RenderDevice* graphicsDevice, const RenderPassGraph* passGraph, uint64_t graphNodeIndex);

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

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1);
        void Draw(const DrawablePrimitive& primitive);
        void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
        void DispatchRays(const Geometry::Dimensions& dispatchDimensions);
        void Dispatch(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize);

        void BindBuffer(Foundation::Name bufferName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        void BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        
        static Geometry::Dimensions DispatchGroupCount(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize);

    private:
        RenderDevice* mGraphicsDevice;
        const RenderPassGraph* mPassGraph;
        uint64_t mGraphNodeIndex;
    };

}

#include "CommandRecorder.inl"
