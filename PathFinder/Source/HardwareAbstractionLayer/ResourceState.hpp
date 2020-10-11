#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <string>

#include <Foundation/BitwiseEnum.hpp>

namespace HAL
{

    enum class ResourceState : uint32_t
    {
        Common                          = 0,
        UnorderedAccess                 = 1 << 1,
        PixelShaderAccess               = 1 << 2,
        NonPixelShaderAccess            = 1 << 3,
        StreamOut                       = 1 << 4,
        IndirectArgument                = 1 << 5,
        CopyDestination                 = 1 << 6,
        CopySource                      = 1 << 7,
        GenericRead                     = 1 << 8,
        RaytracingAccelerationStructure = 1 << 9,
        Predication                     = 1 << 10,
        RenderTarget                    = 1 << 11,
        ResolveDestination              = 1 << 12,
        ResolveSource                   = 1 << 13,
        Present                         = 1 << 14,
        DepthRead                       = 1 << 15,
        DepthWrite                      = 1 << 16,
        ConstantBuffer                  = 1 << 18,

        AnyShaderAccess = PixelShaderAccess | NonPixelShaderAccess
    };

    bool IsResourceStateReadOnly(ResourceState state);
    bool IsResourceStateUsageSupportedOnGraphicsQueue(ResourceState state);
    bool IsResourceStateTransitionsSupportedOnGraphicsQueue(ResourceState state);
    bool IsResourceStateUsageSupportedOnComputeQueue(ResourceState state);
    bool IsResourceStateTransitionSupportedOnComputeQueue(ResourceState state);
    bool IsResourceStateUsageSupportedOnCopyQueue(ResourceState state);
    bool IsResourceStateTransitionSupportedOnCopyQueue(ResourceState state);

    D3D12_RESOURCE_STATES D3DResourceState(ResourceState state);
    std::string StateString(ResourceState state);

}

ENABLE_BITMASK_OPERATORS(HAL::ResourceState);

