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

    void RayTracingAccelerationStructure::AllocateBuffers()
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
        mDevice->D3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&mD3DInputs, &prebuildInfo);

        mBuildScratchBuffer = std::make_unique<BufferResource<uint8_t>>(
            *mDevice, prebuildInfo.ScratchDataSizeInBytes, 1,
            ResourceState::RaytracingAccelerationStructure, ResourceState::UnorderedAccess);

        mFinalBuffer = std::make_unique<BufferResource<uint8_t>>(
            *mDevice, prebuildInfo.ResultDataMaxSizeInBytes, 1,
            ResourceState::RaytracingAccelerationStructure, ResourceState::UnorderedAccess);

        mD3DAccelerationStructure.ScratchAccelerationStructureData = mBuildScratchBuffer->GPUVirtualAddress();
        mD3DAccelerationStructure.DestAccelerationStructureData = mFinalBuffer->GPUVirtualAddress();
        mD3DAccelerationStructure.Inputs = mD3DInputs;
    }

    void RayTracingBottomAccelerationStructure::AllocateBuffers()
    {
        mD3DInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        mD3DInputs.NumDescs = (UINT)mD3DGeometries.size();
        mD3DInputs.pGeometryDescs = mD3DGeometries.data();

        RayTracingAccelerationStructure::AllocateBuffers();
    }

    void RayTracingTopAccelerationStructure::AddInstance(const RayTracingBottomAccelerationStructure& blas)
    {
        assert_format(blas.FinalBuffer(), "Bottom-Level acceleration structure buffers must be allocated before using them in Top-Level structures");

        D3D12_RAYTRACING_INSTANCE_DESC instance{};
        instance.InstanceID = mD3DInstances.size();
        instance.InstanceContributionToHitGroupIndex = 0; // Choose hit group shader
        instance.InstanceMask = 1; // Bitwise AND with TraceRay() parameter
        instance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Transparency? Culling?
        instance.AccelerationStructure = blas.FinalBuffer()->GPUVirtualAddress();

        glm::mat4 identity{ 1.0 };

        for (auto row = 0u; row < 3; row++) {
            for (auto column = 0u; column < 4; column++) {
                instance.Transform[row][column] = identity[row][column];
            }
        }

        mD3DInstances.push_back(instance);
    }

    void RayTracingTopAccelerationStructure::AllocateBuffers()
    {
        mInstanceBuffer = std::make_unique<BufferResource<D3D12_RAYTRACING_INSTANCE_DESC>>(
            *mDevice, mD3DInstances.size(), 1, CPUAccessibleHeapType::Upload);

        mInstanceBuffer->Write(0, mD3DInstances.data(), mD3DInstances.size());

        mD3DInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        mD3DInputs.NumDescs = (UINT)mD3DInstances.size();
        mD3DInputs.InstanceDescs = mInstanceBuffer->GPUVirtualAddress();

        RayTracingAccelerationStructure::AllocateBuffers();
    }

}
