#pragma once

#include "GraphicsDevice.hpp"

namespace PathFinder
{

    class GPUCommandRecorder
    {
    public:
        GPUCommandRecorder(GraphicsDevice* graphicsDevice);

        void SetRenderTarget(Foundation::Name resourceName);
        void SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName = std::nullopt);
        void SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName);
        void ClearBackBuffer(const Foundation::Color& color);
        void ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color);
        void ClearDepth(Foundation::Name resourceName, float depthValue);
        void ApplyPipelineState(Foundation::Name psoName);
        void SetViewport(const HAL::Viewport& viewport);

        void Draw(uint32_t vertexCount, uint32_t vertexStart);
        void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount);
        void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart);
        void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount);
        void Draw(const VertexStorageLocation& vertexStorageLocation);
        void Draw(const DrawablePrimitive& primitive);

        void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);

        void BindBuffer(Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        
        template <class T> 
        void BindExternalBuffer(const HAL::BufferResource<T>& resource, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

    private:
        GraphicsDevice* mGraphicsDevice;
    };

}

#include "GPUCommandRecorder.inl"
