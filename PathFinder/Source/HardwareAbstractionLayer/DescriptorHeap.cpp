#include "DescriptorHeap.hpp"
#include "Utils.h"

#include "../Foundation/Assert.hpp"

namespace HAL
{

    RTDescriptorHeap::RTDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, { capacity }, D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {}



    const RTDescriptor RTDescriptorHeap::EmplaceRTDescriptor(uint64_t indexInHeap, const Texture& texture, uint8_t mipLevel, std::optional<ColorFormat> shaderVisisbleFormat)
    {
        RangeAllocationInfo& range = GetRange(0);
        D3D12_RESOURCE_DESC d3dDesc = d3dDesc = texture.D3DDescription();

        if (shaderVisisbleFormat)
        {
            assert_format(std::holds_alternative<TypelessColorFormat>(texture.Format()), "Format redefinition for texture that has it's own format");
            d3dDesc.Format = ResourceFormat::D3DFormat(*shaderVisisbleFormat);
        }
        else {
            assert_format(std::holds_alternative<ColorFormat>(texture.Format()), "Texture format is not suited for render targets");
        }

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ GetCPUAddress(indexInHeap, 0) };

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = ResourceToRTVDescription(d3dDesc, mipLevel);
        mDevice->D3DDevice()->CreateRenderTargetView(texture.D3DResource(), &rtvDesc, cpuHandle);

        return RTDescriptor{ cpuHandle };
    }

    D3D12_RENDER_TARGET_VIEW_DESC RTDescriptorHeap::ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc, uint8_t mipLevel) const
    {
        D3D12_RENDER_TARGET_VIEW_DESC desc{};

        switch (resourceDesc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension = D3D12_RTV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)resourceDesc.Width;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MipSlice = mipLevel;
                desc.Texture1DArray.FirstArraySlice = 0;
                desc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = mipLevel;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = mipLevel;
                desc.Texture2DArray.FirstArraySlice = 0;
                desc.Texture2DArray.PlaneSlice = 0;
                desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = mipLevel;
                desc.Texture2D.PlaneSlice = 0;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = mipLevel;
            desc.Texture3D.FirstWSlice = 0;
            desc.Texture3D.WSize = -1; // A value of - 1 indicates all of the slices along the w axis, starting from FirstWSlice.
            break;
        }

        desc.Format = resourceDesc.Format;

        return desc;
    }



    DSDescriptorHeap::DSDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, { capacity }, D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {}

    const DSDescriptor DSDescriptorHeap::EmplaceDSDescriptor(uint64_t indexInHeap, const Texture& texture)
    {
        RangeAllocationInfo& range = GetRange(0);

        assert_format(std::holds_alternative<DepthStencilFormat>(texture.Format()), "Texture is not of depth-stencil format");

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ GetCPUAddress(indexInHeap, 0) };

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = ResourceToDSVDescription(texture.D3DDescription());
        mDevice->D3DDevice()->CreateDepthStencilView(texture.D3DResource(), &dsvDesc, cpuHandle);

        return DSDescriptor{ cpuHandle };
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC DSDescriptorHeap::ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC desc{};

        switch (resourceDesc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = 0;
                desc.Texture2DArray.FirstArraySlice = 0;
                desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;
            }
            break;

        default:
            throw std::invalid_argument("Unsupported depth resource dimension");
        }

        desc.Format = resourceDesc.Format;

        return desc;
    }



    CBSRUADescriptorHeap::CBSRUADescriptorHeap(const Device* device, uint64_t shaderResourceRangeCapacity, uint64_t unorderedAccessRangeCapacity, uint64_t constantBufferRangeCapacity)
        : DescriptorHeap(device, { shaderResourceRangeCapacity, unorderedAccessRangeCapacity, constantBufferRangeCapacity }, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {}

    D3D12_SHADER_RESOURCE_VIEW_DESC CBSRUADescriptorHeap::ResourceToSRVDescription(
        const D3D12_RESOURCE_DESC& resourceDesc, 
        uint64_t bufferStride,
        std::optional<ColorFormat> explicitFormat) const
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};

        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        switch (resourceDesc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)(resourceDesc.Width / bufferStride);
            desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            desc.Buffer.StructureByteStride = (UINT)bufferStride;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MostDetailedMip = 0;
                desc.Texture1DArray.MipLevels = resourceDesc.MipLevels;
                desc.Texture1DArray.FirstArraySlice = 0;
                desc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
                desc.Texture1DArray.ResourceMinLODClamp = 0.0f;
            }
            else {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MostDetailedMip = 0;
                desc.Texture1D.MipLevels = resourceDesc.MipLevels;
                desc.Texture1D.ResourceMinLODClamp = 0.0f;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MostDetailedMip = 0;
                desc.Texture2DArray.MipLevels = resourceDesc.MipLevels;
                desc.Texture2DArray.FirstArraySlice = 0;
                desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
                desc.Texture2DArray.PlaneSlice = 0;
                desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            }
            else {
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MostDetailedMip = 0;
                desc.Texture2D.MipLevels = resourceDesc.MipLevels;
                desc.Texture2D.PlaneSlice = 0;
                desc.Texture2D.ResourceMinLODClamp = 0.0f;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MostDetailedMip = 0;
            desc.Texture3D.MipLevels = resourceDesc.MipLevels;
            desc.Texture3D.ResourceMinLODClamp = 0.0f;
            break;
        }
        
        if (explicitFormat) 
        {
            desc.Format = ResourceFormat::D3DFormat(*explicitFormat);
            return desc;
        }
        
        if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && 
            resourceDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
            desc.Format = resourceDesc.Format;
            return desc;
        }

        ResourceFormat::FormatVariant format = ResourceFormat::FormatFromD3DFormat(resourceDesc.Format);

        if (std::holds_alternative<DepthStencilFormat>(format))
        {
            auto d3dDepthStencilSRVFormats = ResourceFormat::D3DDepthStecilShaderAccessFormats(std::get<DepthStencilFormat>(format));
            // Current implementation ignores stencil
            desc.Format = d3dDepthStencilSRVFormats.first;
        }
        else {
            desc.Format = resourceDesc.Format;
        }

        return desc;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC CBSRUADescriptorHeap::BufferToAccelerationStructureDescription(const Buffer& buffer) const
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        desc.RaytracingAccelerationStructure.Location = buffer.GPUVirtualAddress();
        return desc;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC CBSRUADescriptorHeap::ResourceToUAVDescription(
        const D3D12_RESOURCE_DESC& resourceDesc,
        uint64_t bufferStride,
        uint8_t mipLevel,
        std::optional<ColorFormat> explicitFormat) const
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};

        switch (resourceDesc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)(resourceDesc.Width / bufferStride);
            desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
            desc.Buffer.StructureByteStride = (UINT)bufferStride;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MipSlice = mipLevel;
                desc.Texture1DArray.FirstArraySlice = 0;
                desc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = mipLevel;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = mipLevel;
                desc.Texture2DArray.FirstArraySlice = 0;
                desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
                desc.Texture2DArray.PlaneSlice = 0;
            }
            else {
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = mipLevel;
                desc.Texture2D.PlaneSlice = 0;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = mipLevel;
            desc.Texture3D.FirstWSlice = 0;
            desc.Texture3D.WSize = -1; // A value of - 1 indicates all of the slices along the w axis, starting from FirstWSlice.
            break;
        }

        desc.Format = explicitFormat.has_value() ? ResourceFormat::D3DFormat(explicitFormat.value()) : resourceDesc.Format;

        return desc;
    }

    const SRDescriptor CBSRUADescriptorHeap::EmplaceSRDescriptor(uint64_t indexInHeapRange, const Texture& texture, std::optional<ColorFormat> shaderVisibleFormat)
    {
        assert_format(!shaderVisibleFormat || std::holds_alternative<TypelessColorFormat>(texture.Format()), "Format redefinition for typed texture");

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ GetCPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::ShaderResource)) };
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ GetGPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::ShaderResource)) };

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = ResourceToSRVDescription(texture.D3DDescription(), 1, shaderVisibleFormat);
        mDevice->D3DDevice()->CreateShaderResourceView(texture.D3DResource(), &desc, cpuHandle);

        return SRDescriptor{ cpuHandle, gpuHandle, indexInHeapRange };
    }

    const UADescriptor CBSRUADescriptorHeap::EmplaceUADescriptor(uint64_t indexInHeapRange, const Texture& texture, uint8_t mipLevel, std::optional<ColorFormat> shaderVisibleFormat)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ GetCPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::UnorderedAccess)) };
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ GetGPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::UnorderedAccess)) };

        assert_format(!shaderVisibleFormat || std::holds_alternative<TypelessColorFormat>(texture.Format()), "Format redefinition for typed texture");

        if (!(texture.D3DDescription().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        {
            printf("");
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = ResourceToUAVDescription(texture.D3DDescription(), 1, mipLevel, shaderVisibleFormat);
        mDevice->D3DDevice()->CreateUnorderedAccessView(texture.D3DResource(), nullptr, &desc, cpuHandle);

        return UADescriptor{ cpuHandle, gpuHandle, indexInHeapRange };
    }

    DescriptorAddress CBSRUADescriptorHeap::RangeStartGPUAddress(Range range) const
    {
        auto index = std::underlying_type_t<Range>(range);
        const RangeAllocationInfo& allocInfo = GetRange(index);
        return allocInfo.StartGPUHandle.ptr;
    }

    const CBDescriptor CBSRUADescriptorHeap::EmplaceCBDescriptor(uint64_t indexInHeapRange, const Buffer& buffer, uint64_t stride)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ GetCPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::ConstantBuffer)) };
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ GetGPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::ConstantBuffer)) };

        D3D12_CONSTANT_BUFFER_VIEW_DESC desc{ buffer.GPUVirtualAddress(), (UINT)stride };
        mDevice->D3DDevice()->CreateConstantBufferView(&desc, cpuHandle);

        return CBDescriptor{ cpuHandle, gpuHandle, indexInHeapRange };
    }

    const SRDescriptor CBSRUADescriptorHeap::EmplaceSRDescriptor(uint64_t indexInHeapRange, const Buffer& buffer, uint64_t stride)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ GetCPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::ShaderResource)) };
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ GetGPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::ShaderResource)) };

        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
        
        if (EnumMaskBitSet(buffer.InitialStates(), ResourceState::RaytracingAccelerationStructure) ||
            EnumMaskBitSet(buffer.ExpectedStates(), ResourceState::RaytracingAccelerationStructure))
        {
            desc = BufferToAccelerationStructureDescription(buffer);
            // Resource pointer is not required for AS SR view, 
            // since its address is already encoded into VIEW_DESC structure
            mDevice->D3DDevice()->CreateShaderResourceView(nullptr, &desc, cpuHandle);
        }
        else {
            desc = ResourceToSRVDescription(buffer.D3DDescription(), stride);
            mDevice->D3DDevice()->CreateShaderResourceView(buffer.D3DResource(), &desc, cpuHandle);
        }

        return SRDescriptor{ cpuHandle, gpuHandle, indexInHeapRange };
    }

    const UADescriptor CBSRUADescriptorHeap::EmplaceUADescriptor(uint64_t indexInHeapRange, const Buffer& buffer, uint64_t stride)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ GetCPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::UnorderedAccess)) };
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ GetGPUAddress(indexInHeapRange, std::underlying_type_t<Range>(Range::UnorderedAccess)) };

        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = ResourceToUAVDescription(buffer.D3DDescription(), stride);
        mDevice->D3DDevice()->CreateUnorderedAccessView(buffer.D3DResource(), nullptr, &desc, cpuHandle);

        return UADescriptor{ cpuHandle, gpuHandle, indexInHeapRange };
    }

 }
