#pragma once

#include "BufferResource.hpp"

#include "../Foundation/Assert.hpp"

#include <cstdint>
#include <d3d12.h>
#include <vector>

namespace HAL
{
    
    class RayTracingBottomAS
    {
    public:
        template <class Vertex, class Index>
        struct Geometry
        {
            template <class Vertex, class Index>
            Geometry(const BufferResource<Vertex>* vertexBuffer, uint32_t vertexOffset, uint32_t vertexCount, ResourceFormat::Color vertexFormat,
                const BufferResource<Index>* indexBuffer, uint32_t indexOffset, uint32_t indexCount, ResourceFormat::Color indexFormat,
                bool isOpaque = true)
                :
                VertexBuffer{ vertexBuffer }, IndexBuffer{ indexBuffer }, VertexOffset{ vertexOffset }, VertexCount{ vertexCount },
                IndexOffset{ indexOffset }, IndexCount{ indexCount }, VertexFormat{ vertexFormat }, IndexFormat{ indexFormat }, IsOpaque{ isOpaque } {}

            template <class Vertex, class Index>
            Geometry(const BufferResource<Vertex>* vertexBuffer, ResourceFormat::Color vertexFormat,
                const BufferResource<Index>* indexBuffer, ResourceFormat::Color indexFormat,
                bool isOpaque = true)
                :
                Geometry(vertexBuffer, 0, vertexBuffer.Capacity(), vertexFormat, indexBuffer, 0, indexBuffer.Capacity(), indexFormat, isOpaque) {}

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

        RayTracingBottomAS(const Device* device);

        template <class Vertex, class Index> 
        void AddGeometry(const Geometry<Vertex, Index>& geometry);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Build();

    private:
        const Device* mDevice;
        std::unique_ptr<BufferResource<uint8_t>> mBuildScratchBuffer;
        std::unique_ptr<BufferResource<uint8_t>> mResultBuffer;
        // There could also be an Update Scratch Buffer
        
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> mD3DGeometries;
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS mD3DInputs{};

    public:

    };

}

#include "RayTracingBottomAS.inl"