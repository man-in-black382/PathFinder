#include "DescriptorHeap.hpp"
#include "Utils.h"

namespace HAL
{

    RTDescriptorHeap::RTDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, capacity, 1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {}

    const RTDescriptor& RTDescriptorHeap::EmplaceDescriptorForTexture(const ColorTexture& resource)
    {
        ValidateCapacity(0);

        RangeAllocationInfo& range = GetRange(0);

        RTDescriptor& descriptor = std::get<DescriptorContainer<RTDescriptor>>(mDescriptors).emplace_back(range.CurrentCPUHandle, range.InsertedDescriptorCount);
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = ResourceToRTVDescription(resource.D3DDescription());
        mDevice->D3DPtr()->CreateRenderTargetView(resource.D3DPtr(), &rtvDesc, range.CurrentCPUHandle);

        IncrementCounters(0);

        return descriptor;
    }

    const RTDescriptor& RTDescriptorHeap::EmplaceDescriptorForTexture(const TypelessTexture& resource, ResourceFormat::Color concreteFormat)
    {
        ValidateCapacity(0);

        RangeAllocationInfo& range = GetRange(0);

        RTDescriptor& descriptor = std::get<DescriptorContainer<RTDescriptor>>(mDescriptors).emplace_back(range.CurrentCPUHandle, range.InsertedDescriptorCount);

        D3D12_RESOURCE_DESC d3dDesc = resource.D3DDescription();
        d3dDesc.Format = ResourceFormat::D3DFormat(concreteFormat);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = ResourceToRTVDescription(d3dDesc);
        mDevice->D3DPtr()->CreateRenderTargetView(resource.D3DPtr(), &rtvDesc, range.CurrentCPUHandle);

        IncrementCounters(0);

        return descriptor;
    }

    D3D12_RENDER_TARGET_VIEW_DESC RTDescriptorHeap::ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const
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
                desc.Texture1DArray.MipSlice = 0;
                desc.Texture1DArray.FirstArraySlice = 0;
                desc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = 0;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = 0;
                desc.Texture2DArray.FirstArraySlice = 0;
                desc.Texture2DArray.PlaneSlice = 0;
                desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;
                desc.Texture2D.PlaneSlice = 0;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = 0;
            desc.Texture3D.FirstWSlice = 0;
            desc.Texture3D.WSize = -1; // A value of - 1 indicates all of the slices along the w axis, starting from FirstWSlice.
            break;
        }

        desc.Format = resourceDesc.Format;

        return desc;
    }



    DSDescriptorHeap::DSDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, capacity, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {}

    const DSDescriptor& DSDescriptorHeap::EmplaceDescriptorForResource(const DepthStencilTexture& resource)
    {
        ValidateCapacity(0);

        RangeAllocationInfo& range = GetRange(0);

        const DSDescriptor& descriptor = std::get<DescriptorContainer<DSDescriptor>>(mDescriptors).emplace_back(range.CurrentCPUHandle, range.InsertedDescriptorCount);
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = ResourceToDSVDescription(resource.D3DDescription());
        mDevice->D3DPtr()->CreateDepthStencilView(resource.D3DPtr(), &dsvDesc, range.CurrentCPUHandle);

        IncrementCounters(0);

        return descriptor;
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



    CBSRUADescriptorHeap::CBSRUADescriptorHeap(const Device* device, uint32_t rangeCapacity)
        : DescriptorHeap(device, rangeCapacity, std::underlying_type_t<Range>(Range::TotalCount), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {}

    const SRDescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForTexture(const ColorTexture& texture)
    {
        return EmplaceDescriptorForSRTexture(texture);
    }

    const SRDescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForTexture(const DepthStencilTexture& texture)
    {
        return EmplaceDescriptorForSRTexture(texture);
    }

    const SRDescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForTexture(const TypelessTexture& texture, ResourceFormat::Color shaderVisibleFormat)
    {
        return EmplaceDescriptorForSRTexture(texture, shaderVisibleFormat);
    }

    const UADescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForUnorderedAccessTexture(const ColorTexture& texture)
    {
        return EmplaceDescriptorForUATexture(texture);
    }

    const UADescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForUnorderedAccessTexture(const DepthStencilTexture& texture)
    {
        return EmplaceDescriptorForUATexture(texture);
    }

    const UADescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForUnorderedAccessTexture(const TypelessTexture& texture, ResourceFormat::Color shaderVisibleFormat)
    {
        return EmplaceDescriptorForUATexture(texture, shaderVisibleFormat);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC CBSRUADescriptorHeap::ResourceToSRVDescription(const D3D12_RESOURCE_DESC& resourceDesc, std::optional<ResourceFormat::Color> explicitFormat) const
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};

        switch (resourceDesc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)resourceDesc.Width;
            desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            desc.Buffer.StructureByteStride = 1; // TODO: Change to an actual stride of a buffer content
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

        desc.Format = explicitFormat.has_value() ? ResourceFormat::D3DFormat(explicitFormat.value()) : resourceDesc.Format;

        return desc;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC CBSRUADescriptorHeap::ResourceToUAVDescription(const D3D12_RESOURCE_DESC& resourceDesc, std::optional<ResourceFormat::Color> explicitFormat) const
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};

        switch (resourceDesc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)resourceDesc.Width;
            desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
            desc.Buffer.StructureByteStride = 1; // TODO: Change to an actual stride of a buffer content
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MipSlice = 0;
                desc.Texture1DArray.FirstArraySlice = 0;
                desc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = 0;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = 0;
                desc.Texture2DArray.FirstArraySlice = 0;
                desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
                desc.Texture2DArray.PlaneSlice = 0;
            }
            else {
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;
                desc.Texture2D.PlaneSlice = 0;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = 0;
            desc.Texture3D.FirstWSlice = 0;
            desc.Texture3D.WSize = -1; // A value of - 1 indicates all of the slices along the w axis, starting from FirstWSlice.
            break;
        }

        desc.Format = explicitFormat.has_value() ? ResourceFormat::D3DFormat(explicitFormat.value()) : resourceDesc.Format;

        return desc;
    }

    const SRDescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForSRTexture(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat)
    {
        auto index = std::underlying_type_t<Range>(RangeTypeForTexture(texture));

        ValidateCapacity(index);

        RangeAllocationInfo& range = GetRange(index);

        const SRDescriptor& descriptor = std::get<DescriptorContainer<SRDescriptor>>(mDescriptors).emplace_back(range.CurrentCPUHandle, range.CurrentGPUHandle, range.InsertedDescriptorCount);
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = ResourceToSRVDescription(texture.D3DDescription(), shaderVisibleFormat);

        mDevice->D3DPtr()->CreateShaderResourceView(texture.D3DPtr(), &desc, range.CurrentCPUHandle);

        IncrementCounters(index);

        return descriptor;
    }

    const UADescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForUATexture(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat)
    {
        auto index = std::underlying_type_t<Range>(RangeTypeForTexture(texture));

        ValidateCapacity(index);

        RangeAllocationInfo& range = GetRange(index);

        const UADescriptor& descriptor = std::get<DescriptorContainer<UADescriptor>>(mDescriptors).emplace_back(range.CurrentCPUHandle, range.CurrentGPUHandle, range.InsertedDescriptorCount);
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = ResourceToUAVDescription(texture.D3DDescription(), shaderVisibleFormat);

        mDevice->D3DPtr()->CreateUnorderedAccessView(texture.D3DPtr(), nullptr, &desc, range.CurrentCPUHandle);

        IncrementCounters(index);

        return descriptor;
    }

    CBSRUADescriptorHeap::Range CBSRUADescriptorHeap::RangeTypeForTexture(const TextureResource& texture) const
    {
        switch (texture.Kind())
        {
        case ResourceFormat::TextureKind::Texture1D: return Range::Texture1D;
        case ResourceFormat::TextureKind::Texture2D: return texture.IsArray() ? Range::Texture2DArray : Range::Texture2D;
        case ResourceFormat::TextureKind::Texture3D: return Range::Texture3D;
        }
    }

    CBSRUADescriptorHeap::Range CBSRUADescriptorHeap::UARangeTypeForTexture(const TextureResource& texture) const
    {
        switch (texture.Kind())
        {
        case ResourceFormat::TextureKind::Texture1D: return Range::UATexture1D;
        case ResourceFormat::TextureKind::Texture2D: return texture.IsArray() ? Range::UATexture2DArray : Range::UATexture2D;
        case ResourceFormat::TextureKind::Texture3D: return Range::UATexture3D;
        }
    }

 }
