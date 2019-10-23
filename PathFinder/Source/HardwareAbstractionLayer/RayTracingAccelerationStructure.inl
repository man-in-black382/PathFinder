#pragma once

namespace HAL
{

    template <class Vertex, class Index>
    void RayTracingBottomAccelerationStructure::AddGeometry(const RayTracingGeometry<Vertex, Index>& geometry)
    {
        D3D12_RAYTRACING_GEOMETRY_DESC d3dGeometry{};

        d3dGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

        d3dGeometry.Triangles.VertexBuffer.StartAddress = geometry.VertexBuffer->GPUVirtualAddress() + geometry.VertexOffset * geometry.VertexBuffer->PaddedElementSize();
        d3dGeometry.Triangles.VertexBuffer.StrideInBytes = geometry.VertexBuffer->PaddedElementSize();
        d3dGeometry.Triangles.VertexCount = geometry.VertexCount;
        d3dGeometry.Triangles.VertexFormat = ResourceFormat::D3DFormat(geometry.VertexPositionFormat);

        d3dGeometry.Triangles.IndexBuffer = geometry.IndexBuffer->GPUVirtualAddress() + geometry.IndexOffset * geometry.IndexBuffer->PaddedElementSize();
        d3dGeometry.Triangles.IndexCount = geometry.IndexCount;
        d3dGeometry.Triangles.IndexFormat = ResourceFormat::D3DFormat(geometry.IndexFormat);

        d3dGeometry.Triangles.Transform3x4 = 0;

        if (geometry.IsOpaque) d3dGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        mD3DGeometries.push_back(d3dGeometry);
    }

}

