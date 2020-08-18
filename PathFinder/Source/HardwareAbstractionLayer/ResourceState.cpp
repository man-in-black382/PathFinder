#include "ResourceState.hpp"

namespace HAL
{

    D3D12_RESOURCE_STATES D3DResourceState(ResourceState state)
    {
        D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_COMMON;
        if (EnumMaskContains(state, ResourceState::UnorderedAccess)) states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        if (EnumMaskContains(state, ResourceState::PixelShaderAccess)) states |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        if (EnumMaskContains(state, ResourceState::NonPixelShaderAccess)) states |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (EnumMaskContains(state, ResourceState::StreamOut)) states |= D3D12_RESOURCE_STATE_STREAM_OUT;
        if (EnumMaskContains(state, ResourceState::IndirectArgument)) states |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        if (EnumMaskContains(state, ResourceState::CopyDestination)) states |= D3D12_RESOURCE_STATE_COPY_DEST;
        if (EnumMaskContains(state, ResourceState::CopySource)) states |= D3D12_RESOURCE_STATE_COPY_SOURCE;
        if (EnumMaskContains(state, ResourceState::GenericRead)) states |= D3D12_RESOURCE_STATE_GENERIC_READ;
        if (EnumMaskContains(state, ResourceState::RaytracingAccelerationStructure)) states |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if (EnumMaskContains(state, ResourceState::Predication)) states |= D3D12_RESOURCE_STATE_PREDICATION;
        if (EnumMaskContains(state, ResourceState::RenderTarget)) states |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        if (EnumMaskContains(state, ResourceState::ResolveDestination)) states |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
        if (EnumMaskContains(state, ResourceState::ResolveSource)) states |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        if (EnumMaskContains(state, ResourceState::Present)) states |= D3D12_RESOURCE_STATE_PRESENT;
        if (EnumMaskContains(state, ResourceState::DepthRead)) states |= D3D12_RESOURCE_STATE_DEPTH_READ;
        if (EnumMaskContains(state, ResourceState::DepthWrite)) states |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if (EnumMaskContains(state, ResourceState::ConstantBuffer)) states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        return states;
    }

    std::string StateString(ResourceState state)
    {
        std::string string = "";

        auto addStateString = [&string](const std::string& newString)
        {
            if (!string.empty())
            {
                string += " | ";
            }

            string += newString;
        };

        if (EnumMaskContains(state, ResourceState::UnorderedAccess)) addStateString("UnorderedAccess");
        if (EnumMaskContains(state, ResourceState::PixelShaderAccess))  addStateString("PixelShaderAccess");
        if (EnumMaskContains(state, ResourceState::NonPixelShaderAccess))  addStateString("NonPixelShaderAccess");
        if (EnumMaskContains(state, ResourceState::StreamOut))  addStateString("StreamOut");
        if (EnumMaskContains(state, ResourceState::IndirectArgument))  addStateString("IndirectArgument");
        if (EnumMaskContains(state, ResourceState::CopyDestination))  addStateString("CopyDestination");
        if (EnumMaskContains(state, ResourceState::CopySource))  addStateString("CopySource");
        if (EnumMaskContains(state, ResourceState::GenericRead))  addStateString("GenericRead");
        if (EnumMaskContains(state, ResourceState::RaytracingAccelerationStructure))  addStateString("RaytracingAccelerationStructure");
        if (EnumMaskContains(state, ResourceState::Predication))  addStateString("Predication");
        if (EnumMaskContains(state, ResourceState::RenderTarget))  addStateString("RenderTarget");
        if (EnumMaskContains(state, ResourceState::ResolveDestination))  addStateString("ResolveDestination");
        if (EnumMaskContains(state, ResourceState::ResolveSource))  addStateString("ResolveSource");
        if (EnumMaskContains(state, ResourceState::Present))  addStateString("Present");
        if (EnumMaskContains(state, ResourceState::DepthRead))  addStateString("DepthRead");
        if (EnumMaskContains(state, ResourceState::DepthWrite))  addStateString("DepthWrite");
        if (EnumMaskContains(state, ResourceState::ConstantBuffer))  addStateString("ConstantBuffer");

        if (string.empty())
        {
            string = "Common";
        }

        return string;
    }

    bool IsResourceStateReadOnly(ResourceState state)
    {
        return (EnumMaskContains(state, ResourceState::PixelShaderAccess)) ||
            (EnumMaskContains(state, ResourceState::NonPixelShaderAccess)) ||
            (EnumMaskContains(state, ResourceState::IndirectArgument)) ||
            (EnumMaskContains(state, ResourceState::CopySource)) ||
            (EnumMaskContains(state, ResourceState::GenericRead)) ||
            (EnumMaskContains(state, ResourceState::RaytracingAccelerationStructure)) ||
            (EnumMaskContains(state, ResourceState::Predication)) ||
            //(EnumMaskBitSet(state, ResourceState::ResolveSource)) ||
            (EnumMaskContains(state, ResourceState::Present)) ||
            (EnumMaskContains(state, ResourceState::DepthRead)) ||
            (EnumMaskContains(state, ResourceState::ConstantBuffer));

    }

    bool IsResourceStateUsageSupportedOnGraphicsQueue(ResourceState state)
    {
        return true;
    }

    bool IsResourceStateTransitionsSupportedOnGraphicsQueue(ResourceState state)
    {
        return true;
    }

    bool IsResourceStateUsageSupportedOnComputeQueue(ResourceState state)
    {
        HAL::ResourceState allowedStates =
            ResourceState::Common | 
            ResourceState::AnyShaderAccess |
            ResourceState::GenericRead | 
            ResourceState::CopyDestination |
            ResourceState::CopySource | 
            ResourceState::UnorderedAccess |
            ResourceState::RaytracingAccelerationStructure |
            ResourceState::ConstantBuffer | 
            ResourceState::DepthRead;

        return !EnumMaskContains(state, ~allowedStates);
    }

    bool IsResourceStateTransitionSupportedOnComputeQueue(ResourceState state)
    {
        HAL::ResourceState allowedStates =
            ResourceState::Common |
            ResourceState::NonPixelShaderAccess |
            ResourceState::GenericRead |
            ResourceState::CopyDestination |
            ResourceState::CopySource |
            ResourceState::UnorderedAccess |
            ResourceState::RaytracingAccelerationStructure |
            ResourceState::ConstantBuffer;

        return !EnumMaskContains(state, ~allowedStates);
    }

    bool IsResourceStateUsageSupportedOnCopyQueue(ResourceState state)
    {
        HAL::ResourceState allowedStates = ResourceState::Common | ResourceState::CopyDestination | ResourceState::CopySource;
        return !EnumMaskContains(state, ~allowedStates);
    }

    bool IsResourceStateTransitionSupportedOnCopyQueue(ResourceState state)
    {
        HAL::ResourceState allowedStates = ResourceState::Common | ResourceState::CopyDestination | ResourceState::CopySource;
        return !EnumMaskContains(state, ~allowedStates);
    }

}

