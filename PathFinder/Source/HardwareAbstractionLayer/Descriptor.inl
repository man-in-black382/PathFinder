#pragma once

namespace HAL
{
   
    template <class Vertex>
    VertexBufferDescriptor::VertexBufferDescriptor(const BufferResource<Vertex>& vertexBuffer)
    {
        mDescriptor.BufferLocation = vertexBuffer.D3DResource()->GetGPUVirtualAddress();
        mDescriptor.SizeInBytes = (UINT)vertexBuffer.D3DDescription().Width;
        mDescriptor.StrideInBytes = (UINT)vertexBuffer.PaddedElementSize();
    }

    template <class Index>
    IndexBufferDescriptor::IndexBufferDescriptor(const BufferResource<Index>& indexBuffer, ResourceFormat::Color format)
    {
        mDescriptor.BufferLocation = indexBuffer.D3DResource()->GetGPUVirtualAddress();
        mDescriptor.SizeInBytes = (UINT)indexBuffer.D3DDescription().Width;
        mDescriptor.Format = ResourceFormat::D3DFormat(format);
    }

}

