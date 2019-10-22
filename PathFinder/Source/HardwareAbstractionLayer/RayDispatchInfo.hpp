#pragma once

#include <d3d12.h>

namespace HAL
{

    /* typedef struct D3D12_GPU_VIRTUAL_ADDRESS_RANGE
     {
         D3D12_GPU_VIRTUAL_ADDRESS StartAddress;
         UINT64 SizeInBytes;
     }*/

    //typedef struct D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE
    //{
    //    D3D12_GPU_VIRTUAL_ADDRESS StartAddress;
    //    UINT64 SizeInBytes;
    //    UINT64 StrideInBytes;
    //}

    //D3D12_GPU_VIRTUAL_ADDRESS_RANGE RayGenerationShaderRecord;
    //D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE MissShaderTable;
    //D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE HitGroupTable;
    //D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CallableShaderTable;
    //UINT Width;
    //UINT Height;
    //UINT Depth;

    class RayDispatchInfo
    {
    private:
        D3D12_DISPATCH_RAYS_DESC mDesc{};
    };

}

