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
        void ClearUnorderedAccessResource(Foundation::Name resourceName, const glm::vec4& clearValue);
        void ClearUnorderedAccessResource(Foundation::Name resourceName, const glm::uvec4& clearValue);
        void ApplyPipelineState(Foundation::Name psoName);
        void SetViewport(const HAL::Viewport& viewport);
        void WaitUntilUnorderedAccessesComplete(Foundation::Name resourceName);

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1);
        void Draw(const DrawablePrimitive& primitive);
        void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);

        void BindBuffer(Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        template <class T> void BindExternalBuffer(const HAL::BufferResource<T>& resource, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

    private:
        GraphicsDevice* mGraphicsDevice;
    };

}

#include "GPUCommandRecorder.inl"