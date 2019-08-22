#include "DescriptorHeap.hpp"
#include "Utils.h"

#include "../Foundation/Assert.hpp"

namespace HAL
{

    RTDescriptorHeap::RTDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, capacity, 1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {}



    const RTDescriptor& RTDescriptorHeap::EmplaceRTDescriptor(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisisbleFormat)
    {
        ValidateCapacity(0);
        RangeAllocationInfo& range = GetRange(0);
        D3D12_RESOURCE_DESC d3dDesc = d3dDesc = texture.D3DDescription();

        if (shaderVisisbleFormat)
        {
            assert_format(std::holds_alternative<ResourceFormat::TypelessColor>(texture.Format()), "Format redefinition for texture that has it's own format");
            d3dDesc.Format = ResourceFormat::D3DFormat(*shaderVisisbleFormat);
        }
        else {
            assert_format(std::holds_alternative<ResourceFormat::Color>(texture.Format()), "Texture format is not suited for render targets");
        }

        auto& descriptor = dynamic_cast<RTDescriptor&>(*range.Descriptors.emplace_back(
            std::make_unique<RTDescriptor>(range.CurrentCPUHandle)));

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = ResourceToRTVDescription(d3dDesc);
        mDevice->D3DPtr()->CreateRenderTargetView(texture.D3DPtr(), &rtvDesc, range.CurrentCPUHandle);

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

    const DSDescriptor& DSDescriptorHeap::EmplaceDSDescriptor(const TextureResource& texture)
    {
        ValidateCapacity(0);
        RangeAllocationInfo& range = GetRange(0);

        assert_format(std::holds_alternative<ResourceFormat::DepthStencil>(texture.Format()), "Texture is not of depth-stencil format");

        auto& descriptor = dynamic_cast<DSDescriptor&>(*range.Descriptors.emplace_back(
            std::make_unique<DSDescriptor>(range.CurrentCPUHandle)));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = ResourceToDSVDescription(texture.D3DDescription());
        mDevice->D3DPtr()->CreateDepthStencilView(texture.D3DPtr(), &dsvDesc, range.CurrentCPUHandle);

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
  
    D3D12_SHADER_RESOURCE_VIEW_DESC CBSRUADescriptorHeap::ResourceToSRVDescription(
        const D3D12_RESOURCE_DESC& resourceDesc, 
        uint64_t bufferStride,
        std::optional<ResourceFormat::Color> explicitFormat) const
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};

        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        switch (resourceDesc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)resourceDesc.Width;
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

        desc.Format = explicitFormat.has_value() ? ResourceFormat::D3DFormat(explicitFormat.value()) : resourceDesc.Format;

        return desc;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC CBSRUADescriptorHeap::ResourceToUAVDescription(
        const D3D12_RESOURCE_DESC& resourceDesc,
        uint64_t bufferStride,
        std::optional<ResourceFormat::Color> explicitFormat) const
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};

        switch (resourceDesc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)resourceDesc.Width;
            desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
            desc.Buffer.StructureByteStride = (UINT)bufferStride;
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

    uint32_t CBSRUADescriptorHeap::RangeCapacity() const
    {
        return mRangeCapacity;
    }

    uint32_t CBSRUADescriptorHeap::RangeStartIndex(Range range) const
    {
        return mRangeCapacity * std::underlying_type_t<Range>(range);
    }

    const GPUDescriptor* CBSRUADescriptorHeap::GetDescriptor(Range range, uint32_t indexInRange) const
    {
        auto rangeIndex = std::underlying_type_t<Range>(range);
        const RangeAllocationInfo& rangeInfo = GetRange(rangeIndex);
        
        if (indexInRange >= rangeInfo.Descriptors.size()) return nullptr;

        return rangeInfo.Descriptors[indexInRange].get();
    }

    const SRDescriptor& CBSRUADescriptorHeap::EmplaceSRDescriptor(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat)
    {
        auto index = std::underlying_type_t<Range>(RangeTypeForTexture(texture));
        ValidateCapacity(index);
        RangeAllocationInfo& range = GetRange(index);

        assert_format(!shaderVisibleFormat || std::holds_alternative<ResourceFormat::TypelessColor>(texture.Format()), "Format redefinition for typed texture");

        auto& descriptor = dynamic_cast<SRDescriptor&>(*range.Descriptors.emplace_back(
            std::make_unique<SRDescriptor>(range.CurrentCPUHandle, range.CurrentGPUHandle, range.Descriptors.size())));

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = ResourceToSRVDescription(texture.D3DDescription(), 1, shaderVisibleFormat);
        mDevice->D3DPtr()->CreateShaderResourceView(texture.D3DPtr(), &desc, range.CurrentCPUHandle);

        IncrementCounters(index);
        return descriptor;
    }

    const UADescriptor& CBSRUADescriptorHeap::EmplaceUADescriptor(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat)
    {
        auto index = std::underlying_type_t<Range>(UARangeTypeForTexture(texture));
        ValidateCapacity(index);
        RangeAllocationInfo& range = GetRange(index);

        assert_format(!shaderVisibleFormat || std::holds_alternative<ResourceFormat::TypelessColor>(texture.Format()), "Format redefinition for typed texture");

        auto& descriptor = dynamic_cast<UADescriptor&>(*range.Descriptors.emplace_back(
            std::make_unique<UADescriptor>(range.CurrentCPUHandle, range.CurrentGPUHandle, range.Descriptors.size())));

        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = ResourceToUAVDescription(texture.D3DDescription(), 1, shaderVisibleFormat);
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
