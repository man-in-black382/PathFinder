#pragma once

#include "GraphicAPIObject.hpp"
#include "Buffer.hpp"

#include "../Foundation/Assert.hpp"

#include <cstdint>
#include <d3d12.h>
#include <vector>

#include <glm/mat4x4.hpp>

namespace HAL
{
    
    struct RayTracingGeometry
    {
        RayTracingGeometry(const Buffer* vertexBuffer, uint32_t vertexOffset, uint32_t vertexCount, uint32_t vertexStride, ColorFormat vertexPositionFormat,
            const Buffer* indexBuffer, uint32_t indexOffset, uint32_t indexCount, uint32_t indexStride, ColorFormat indexFormat, const glm::mat4x4& transform,
            bool isOpaque = true)
            :
            VertexBuffer{ vertexBuffer }, IndexBuffer{ indexBuffer }, VertexOffset{ vertexOffset }, VertexCount{ vertexCount }, VertexStride{ vertexStride },
            IndexOffset{ indexOffset }, IndexCount{ indexCount }, IndexStride{ indexStride }, VertexPositionFormat{ vertexPositionFormat }, IndexFormat{ indexFormat },
            Transform{ transform }, IsOpaque{ isOpaque } {}

        // A format for 'Position' vertex structure field
        // Implies that 'Position' must be placed first in vertex's memory
        ColorFormat VertexPositionFormat;
        ColorFormat IndexFormat;
        const Buffer* VertexBuffer;
        const Buffer* IndexBuffer;
        uint32_t VertexOffset;
        uint32_t VertexCount;
        uint32_t VertexStride;
        uint32_t IndexOffset;
        uint32_t IndexCount;
        uint32_t IndexStride;
        glm::mat4x4 Transform;
        bool IsOpaque;
    };



    class RayTracingAccelerationStructure : public GraphicAPIObject
    {
    public:
        RayTracingAccelerationStructure(const Device* device);

        virtual void AllocateBuffersIfNeeded() = 0;
        virtual void ResetInputs() = 0;
        virtual void SetDebugName(const std::string& name) override;

    protected:
        const Device* mDevice;
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS mD3DInputs{};
    
    private:
        std::string mDebugName;
        std::unique_ptr<Buffer> mBuildScratchBuffer;
        std::unique_ptr<Buffer> mFinalBuffer;
        // There could also be an Update Scratch Buffer. Isn't needed right now.

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC mD3DAccelerationStructure{};

    public:
        inline const auto& D3DAccelerationStructure() const { return mD3DAccelerationStructure; }
        inline const auto* FinalBuffer() const { return mFinalBuffer.get(); }
    };



    class RayTracingBottomAccelerationStructure : public RayTracingAccelerationStructure
    {
    public:
        using RayTracingAccelerationStructure::RayTracingAccelerationStructure;

        void AddGeometry(const RayTracingGeometry& geometry);

        virtual void AllocateBuffersIfNeeded() override;
        virtual void ResetInputs() override;

    private:
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> mD3DGeometries;
    };



    class RayTracingTopAccelerationStructure : public RayTracingAccelerationStructure
    {
    public:
        using RayTracingAccelerationStructure::RayTracingAccelerationStructure;

        void AddInstance(const RayTracingBottomAccelerationStructure& blas, uint32_t instanceId, const glm::mat4& transform);

        virtual void AllocateBuffersIfNeeded() override;
        virtual void ResetInputs() override;

    private:
        std::unique_ptr<Buffer> mInstanceBuffer;
        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> mD3DInstances;
    };

}
