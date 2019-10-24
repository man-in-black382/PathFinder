#pragma once

#include "GraphicAPIObject.hpp"
#include "BufferResource.hpp"

#include "../Foundation/Assert.hpp"

#include <cstdint>
#include <d3d12.h>
#include <vector>

#include <glm/mat4x4.hpp>

namespace HAL
{
    
    template <class Vertex, class Index>
    struct RayTracingGeometry
    {
        template <class Vertex, class Index>
        RayTracingGeometry(const BufferResource<Vertex>* vertexBuffer, uint32_t vertexOffset, uint32_t vertexCount, ResourceFormat::Color vertexPositionFormat,
            const BufferResource<Index>* indexBuffer, uint32_t indexOffset, uint32_t indexCount, ResourceFormat::Color indexFormat, const glm::mat4x4& transform,
            bool isOpaque = true)
            :
            VertexBuffer{ vertexBuffer }, IndexBuffer{ indexBuffer }, VertexOffset{ vertexOffset }, VertexCount{ vertexCount },
            IndexOffset{ indexOffset }, IndexCount{ indexCount }, VertexPositionFormat{ vertexPositionFormat }, IndexFormat{ indexFormat },
            Transform{ transform }, IsOpaque{ isOpaque } {}

        template <class Vertex, class Index>
        RayTracingGeometry(const BufferResource<Vertex>* vertexBuffer, ResourceFormat::Color vertexFormat,
            const BufferResource<Index>* indexBuffer, ResourceFormat::Color indexFormat)
            :
            RayTracingGeometry(vertexBuffer, 0, vertexBuffer.Capacity(), vertexFormat, indexBuffer, 0, indexBuffer.Capacity(), indexFormat, glm::mat4x4(1.0), true) {}

        // A format for 'Position' vertex structure field
        // Implies that 'Position' must be placed first in vertex's memory
        ResourceFormat::Color VertexPositionFormat;
        ResourceFormat::Color IndexFormat;
        const BufferResource<Vertex>* VertexBuffer;
        const BufferResource<Index>* IndexBuffer;
        uint32_t VertexOffset;
        uint32_t VertexCount;
        uint32_t IndexOffset;
        uint32_t IndexCount;
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
        std::unique_ptr<BufferResource<uint8_t>> mBuildScratchBuffer;
        std::unique_ptr<BufferResource<uint8_t>> mFinalBuffer;
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

        template <class Vertex, class Index> 
        void AddGeometry(const RayTracingGeometry<Vertex, Index>& geometry);

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
        std::unique_ptr<BufferResource<D3D12_RAYTRACING_INSTANCE_DESC>> mInstanceBuffer;
        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> mD3DInstances;
    };

}

#include "RayTracingAccelerationStructure.inl"