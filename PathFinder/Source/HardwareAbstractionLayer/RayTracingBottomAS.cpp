#include "RayTracingBottomAS.hpp"

#include <type_traits>

namespace HAL
{

    RayTracingBottomAS::RayTracingBottomAS(const Device* device)
        : mDevice{ device }
    {
        mD3DInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        mD3DInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        mD3DInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC RayTracingBottomAS::Build()
    {
        mD3DInputs.NumDescs = mD3DGeometries.size();
        mD3DInputs.pGeometryDescs = mD3DGeometries.data();
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
        mDevice->D3DPtr()->GetRaytracingAccelerationStructurePrebuildInfo(&mD3DInputs, &prebuildInfo);

        mBuildScratchBuffer = std::make_unique<BufferResource<uint8_t>>(
            *mDevice, prebuildInfo.ScratchDataSizeInBytes, 1,
            HAL::ResourceState::RaytracingAccelerationStructure, HAL::ResourceState::UnorderedAccess);

        mResultBuffer = std::make_unique<BufferResource<uint8_t>>(
            *mDevice, prebuildInfo.ResultDataMaxSizeInBytes, 1,
            HAL::ResourceState::RaytracingAccelerationStructure, HAL::ResourceState::UnorderedAccess);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildInfo{};

        buildInfo.ScratchAccelerationStructureData = mBuildScratchBuffer->GPUVirtualAddress();
        buildInfo.DestAccelerationStructureData = mResultBuffer->GPUVirtualAddress();
        buildInfo.Inputs = mD3DInputs;

        return buildInfo;
    }

}
