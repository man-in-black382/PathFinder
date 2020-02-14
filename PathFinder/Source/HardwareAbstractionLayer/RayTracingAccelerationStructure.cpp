#include "RayTracingAccelerationStructure.hpp"

#include "../Foundation/Assert.hpp"

#include <type_traits>

namespace HAL
{

    RayTracingAccelerationStructure::RayTracingAccelerationStructure(const Device* device)
        : mDevice{ device }
    {
        mD3DInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        mD3DInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    }

    RayTracingAccelerationStructure::CommonMemoryRequirements RayTracingAccelerationStructure::QueryCommonMemoryRequirements() const
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
        mDevice->D3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&mD3DInputs, &prebuildInfo);
        return { prebuildInfo.ResultDataMaxSizeInBytes, prebuildInfo.ScratchDataSizeInBytes, prebuildInfo.UpdateScratchDataSizeInBytes };
    }

    void RayTracingAccelerationStructure::SetBuffers(const Buffer* destinationBuffer, const Buffer* scratchBuffer, const Buffer* updateBuffer)
    {
        mFinalBuffer = destinationBuffer;
        mBuildScratchBuffer = scratchBuffer;
        mUpdateBuffer = updateBuffer;

        if (mBuildScratchBuffer) mD3DAccelerationStructure.ScratchAccelerationStructureData = mBuildScratchBuffer->GPUVirtualAddress();
        if (mFinalBuffer) mD3DAccelerationStructure.DestAccelerationStructureData = mFinalBuffer->GPUVirtualAddress();
        if (mUpdateBuffer) mD3DAccelerationStructure.SourceAccelerationStructureData = mUpdateBuffer->GPUVirtualAddress();

        mD3DAccelerationStructure.Inputs = mD3DInputs;
    }

    void RayTracingAccelerationStructure::Clear()
    {
        mBuildScratchBuffer = nullptr;
        mFinalBuffer = nullptr;
        mUpdateBuffer = nullptr;
    }



    void RayTracingBottomAccelerationStructure::Clear()
    {
        RayTracingAccelerationStructure::Clear();
        mD3DGeometries.clear();
    }

    void RayTracingBottomAccelerationStructure::SetBuffers(const Buffer* destinationBuffer, const Buffer* scratchBuffer, const Buffer* updateBuffer)
    {
        mD3DInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        mD3DInputs.NumDescs = (UINT)mD3DGeometries.size();
        mD3DInputs.pGeometryDescs = mD3DGeometries.data();

        RayTracingAccelerationStructure::SetBuffers(destinationBuffer, scratchBuffer, updateBuffer);
    }

    void RayTracingBottomAccelerationStructure::AddGeometry(const RayTracingGeometry& geometry)
    {
        D3D12_RAYTRACING_GEOMETRY_DESC d3dGeometry{};

        d3dGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

        d3dGeometry.Triangles.VertexBuffer.StartAddress = geometry.VertexBuffer->GPUVirtualAddress() + geometry.VertexOffset * geometry.VertexStride;
        d3dGeometry.Triangles.VertexBuffer.StrideInBytes = geometry.VertexStride;
        d3dGeometry.Triangles.VertexCount = geometry.VertexCount;
        d3dGeometry.Triangles.VertexFormat = ResourceFormat::D3DFormat(geometry.VertexPositionFormat);

        d3dGeometry.Triangles.IndexBuffer = geometry.IndexBuffer->GPUVirtualAddress() + geometry.IndexOffset * geometry.IndexStride;
        d3dGeometry.Triangles.IndexCount = geometry.IndexCount;
        d3dGeometry.Triangles.IndexFormat = ResourceFormat::D3DFormat(geometry.IndexFormat);

        d3dGeometry.Triangles.Transform3x4 = 0;

        if (geometry.IsOpaque) d3dGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        mD3DGeometries.push_back(d3dGeometry);
    }

    RayTracingBottomAccelerationStructure::MemoryRequirements RayTracingBottomAccelerationStructure::QueryMemoryRequirements() const
    {
        CommonMemoryRequirements commonRequirements = QueryCommonMemoryRequirements();
        return {
            commonRequirements.DestinationBufferMaxSizeInBytes,
            commonRequirements.BuildScratchBufferSizeInBytes,
            commonRequirements.UpdateScratchBufferSizeInBytes
        };
    }



    void RayTracingTopAccelerationStructure::AddInstance(const RayTracingBottomAccelerationStructure& blas, uint32_t instanceId, const glm::mat4& transform)
    {
        assert_format(blas.FinalBuffer(), "Bottom-Level acceleration structure buffers must be allocated before using them in Top-Level structures");

        D3D12_RAYTRACING_INSTANCE_DESC instance{};
        instance.InstanceID = instanceId;
        instance.InstanceContributionToHitGroupIndex = 0; // Choose hit group shader
        instance.InstanceMask = 1; // Bitwise AND with TraceRay() parameter
        instance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Transparency? Culling?
        instance.AccelerationStructure = blas.FinalBuffer()->GPUVirtualAddress();

        for (auto row = 0u; row < 3; row++) {
            for (auto column = 0u; column < 4; column++) {
                instance.Transform[row][column] = transform[row][column];
            }
        }

        mD3DInstances.push_back(instance);
    }

    RayTracingTopAccelerationStructure::MemoryRequirements RayTracingTopAccelerationStructure::QueryMemoryRequirements() const
    {
        CommonMemoryRequirements commonRequirements = QueryCommonMemoryRequirements();
        return {
            commonRequirements.DestinationBufferMaxSizeInBytes, commonRequirements.BuildScratchBufferSizeInBytes,
            commonRequirements.UpdateScratchBufferSizeInBytes, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * mD3DInstances.size()
        };
    }

    void RayTracingTopAccelerationStructure::SetBuffers(
        uint8_t* instanceDataUploadPtr,
        const Buffer* instanceBuffer, 
        const Buffer* destinationBuffer,
        const Buffer* scratchBuffer,
        const Buffer* updateBuffer)
    {
        mInstanceBuffer = instanceBuffer;

        memcpy(instanceDataUploadPtr, mD3DInstances.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * mD3DInstances.size());

        mD3DInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        mD3DInputs.NumDescs = (UINT)mD3DInstances.size();
        mD3DInputs.InstanceDescs = mInstanceBuffer->GPUVirtualAddress();

        RayTracingAccelerationStructure::SetBuffers(destinationBuffer, scratchBuffer, updateBuffer);
    }

    void RayTracingTopAccelerationStructure::Clear()
    {
        RayTracingAccelerationStructure::Clear();
        mInstanceBuffer = nullptr;
        mD3DInstances.clear();
    }

}
