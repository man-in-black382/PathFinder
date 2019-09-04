#pragma once

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
        RayTracingGeometry(const BufferResource<Vertex>* vertexBuffer, uint32_t vertexOffset, uint32_t vertexCount, ResourceFormat::Color vertexFormat,
            const BufferResource<Index>* indexBuffer, uint32_t indexOffset, uint32_t indexCount, ResourceFormat::Color indexFormat,
            bool isOpaque = true)
            :
            VertexBuffer{ vertexBuffer }, IndexBuffer{ indexBuffer }, VertexOffset{ vertexOffset }, VertexCount{ vertexCount },
            IndexOffset{ indexOffset }, IndexCount{ indexCount }, VertexFormat{ vertexFormat }, IndexFormat{ indexFormat }, IsOpaque{ isOpaque } {}

        template <class Vertex, class Index>
        RayTracingGeometry(const BufferResource<Vertex>* vertexBuffer, ResourceFormat::Color vertexFormat,
            const BufferResource<Index>* indexBuffer, ResourceFormat::Color indexFormat,
            bool isOpaque = true)
            :
            RayTracingGeometry(vertexBuffer, 0, vertexBuffer.Capacity(), vertexFormat, indexBuffer, 0, indexBuffer.Capacity(), indexFormat, isOpaque) {}

        const BufferResource<Vertex>* VertexBuffer;
        const BufferResource<Vertex>* IndexBuffer;
        uint32_t VertexOffset;
        uint32_t VertexCount;
        uint32_t IndexOffset;
        uint32_t IndexCount;
        ResourceFormat::Color VertexFormat;
        ResourceFormat::Color IndexFormat;
        bool IsOpaque;
    };



    class RayTracingAccelerationStructure
    {
    public:
        RayTracingAccelerationStructure(const Device* device);

        virtual void AllocateBuffers() = 0;

    protected:
        const Device* mDevice;
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS mD3DInputs{};
    
    private:
        std::unique_ptr<BufferResource<uint8_t>> mBuildScratchBuffer;
        std::unique_ptr<BufferResource<uint8_t>> mFinalBuffer;
        // There could also be an Update Scratch Buffer. Isn't needed right now.

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC mD3DAccelerationStructure;

    public:
        inline const auto& D3DAccelerationStructure() const { return mD3DAccelerationStructure; }
        inline const auto* FinalBuffer() const { return mFinalBuffer.get(); }
    };



    class RayTracingBottomAccelerationStructure : public RayTracingAccelerationStructure
    {
    public:
        template <class Vertex, class Index> 
        void AddGeometry(const RayTracingGeometry<Vertex, Index>& geometry);
        virtual void AllocateBuffers() override;

    private:
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> mD3DGeometries;
    };


    class RayTracingTopAccelerationStructure : public RayTracingAccelerationStructure
    {
    public:
        void AddInstance(const RayTracingBottomAccelerationStructure& blas);
        virtual void AllocateBuffers() override;

    private:
        std::unique_ptr<BufferResource<D3D12_RAYTRACING_INSTANCE_DESC>> mInstanceBuffer;
        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> mD3DInstances;
    };

}

#include "RayTracingAccelerationStructure.inl"