#pragma once

#include "GraphicsDevice.hpp"

namespace PathFinder
{

    class GPUCommandRecorder
    {
    public:
        GPUCommandRecorder(GraphicsDevice* graphicsDevice);

        template <class T> 
        void SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace);

        template <size_t RTCount> 
        void SetRenderTargets(const std::array<ResourceKey, RTCount>& rtKeys, std::optional<ResourceKey> dsKey = std::nullopt);

        void SetRenderTarget(const ResourceKey& rtKey, std::optional<ResourceKey> dsKey = std::nullopt);
        void SetBackBufferAsRenderTarget(std::optional<ResourceKey> dsKey = std::nullopt);
        void ClearRenderTarget(const ResourceKey& rtKey);
        void ClearDepth(const ResourceKey& rtKey);
        void ApplyPipelineState(Foundation::Name psoName);
        void SetViewport(const HAL::Viewport& viewport);

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1);
        void Draw(const DrawablePrimitive& primitive);
        void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
        void DispatchRays(const Geometry::Dimensions& dispatchDimensions);
        void Dispatch(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize);

        void BindBuffer(const ResourceKey& bufferKey, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        void BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        

    private:
        GraphicsDevice* mGraphicsDevice;
    };

}

#include "GPUCommandRecorder.inl"
