#include "ResourceScheduler.hpp"

#include "../Foundation/Assert.hpp"

#include <cmath>

namespace PathFinder
{

    ResourceScheduler::ResourceScheduler(PipelineResourceStorage* manager, RenderPassUtilityProvider* utilityProvider, RenderPassGraph* passGraph)
        : mResourceStorage{ manager }, mUtilityProvider{ utilityProvider }, mRenderPassGraph{ passGraph } {}

    void ResourceScheduler::NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        NewRenderTarget(resourceName, MipSet::FirstMip(), properties);
    }

    void ResourceScheduler::NewRenderTarget(Foundation::Name resourceName, const MipSet& writtenMips, std::optional<NewTextureProperties> properties)
    {
        NewTextureProperties props = FillMissingFields(properties);

        bool canBeReadAcrossFrames = EnumMaskBitSet(properties->Flags, Flags::CrossFrameRead);

        HAL::FormatVariant format = *props.ShaderVisibleFormat;
        if (props.TypelessFormat) format = *props.TypelessFormat;

        mResourceStorage->QueueTextureAllocationIfNeeded(
            resourceName, format, *props.Kind, *props.Dimensions, *props.ClearValues, *props.MipCount, 

            [canBeReadAcrossFrames,
            passNode = mCurrentlySchedulingPassNode,
            typelessFormat = props.TypelessFormat,
            shaderVisibleFormat = props.ShaderVisibleFormat,
            resourceName,
            writtenMips,
            this]
            (PipelineResourceSchedulingInfo& schedulingInfo)
            {
                schedulingInfo.CanBeAliased = !canBeReadAcrossFrames;
                RegisterGraphDependency(*passNode, writtenMips, resourceName, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
                UpdateSubresourceInfos(
                    schedulingInfo,
                    writtenMips,
                    passNode->PassMetadata().Name,
                    HAL::ResourceState::RenderTarget,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureRT,
                    typelessFormat ? shaderVisibleFormat : std::nullopt);
            }
        );
    }

    void ResourceScheduler::NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties)
    {
        NewDepthStencilProperties props = FillMissingFields(properties);
        bool canBeReadAcrossFrames = EnumMaskBitSet(properties->Flags, Flags::CrossFrameRead);
        HAL::DepthStencilClearValue clearValue{ 1.0, 0 };

        mResourceStorage->QueueTextureAllocationIfNeeded(
            resourceName, *props.Format, HAL::TextureKind::Texture2D, *props.Dimensions, clearValue, *props.MipCount,

            [canBeReadAcrossFrames,
            passNode = mCurrentlySchedulingPassNode,
            resourceName,
            this]
            (PipelineResourceSchedulingInfo& schedulingInfo)
            {
                schedulingInfo.CanBeAliased = !canBeReadAcrossFrames;
                RegisterGraphDependency(*passNode, MipSet::FirstMip(), resourceName, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
                UpdateSubresourceInfos(
                    schedulingInfo,
                    MipSet::FirstMip(),
                    passNode->PassMetadata().Name,
                    HAL::ResourceState::DepthWrite,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureDS,
                    std::nullopt);
            }
        );
    }

    void ResourceScheduler::NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        NewTexture(resourceName, MipSet::FirstMip(), properties);
    }

    void ResourceScheduler::NewTexture(Foundation::Name resourceName, const MipSet& writtenMips, std::optional<NewTextureProperties> properties)
    {
        NewTextureProperties props = FillMissingFields(properties);
        bool canBeReadAcrossFrames = EnumMaskBitSet(properties->Flags, Flags::CrossFrameRead);

        HAL::FormatVariant format = *props.ShaderVisibleFormat;
        if (props.TypelessFormat) format = *props.TypelessFormat;

        mResourceStorage->QueueTextureAllocationIfNeeded(
            resourceName, format, *props.Kind, *props.Dimensions, *props.ClearValues, *props.MipCount,

            [canBeReadAcrossFrames,
            passNode = mCurrentlySchedulingPassNode,
            typelessFormat = props.TypelessFormat,
            shaderVisibleFormat = props.ShaderVisibleFormat,
            resourceName,
            writtenMips,
            this]
            (PipelineResourceSchedulingInfo& schedulingInfo)
            {
                schedulingInfo.CanBeAliased = !canBeReadAcrossFrames;
                RegisterGraphDependency(*passNode, writtenMips, resourceName, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
                UpdateSubresourceInfos(
                    schedulingInfo,
                    writtenMips,
                    passNode->PassMetadata().Name,
                    HAL::ResourceState::UnorderedAccess,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureUA,
                    typelessFormat ? shaderVisibleFormat : std::nullopt);
            }
        );
    }

    void ResourceScheduler::UseRenderTarget(Foundation::Name resourceName, const MipSet& writtenMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        AliasAndUseRenderTarget(resourceName, {}, writtenMips, concreteFormat);
    }

    void ResourceScheduler::AliasAndUseRenderTarget(Foundation::Name resourceName, Foundation::Name outputAliasName, const MipSet& writtenMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        Foundation::Name name = outputAliasName.IsValid() ? outputAliasName : resourceName;

        mResourceStorage->QueueResourceUsage(resourceName, outputAliasName.IsValid() ? std::optional(outputAliasName) : std::nullopt,

            [passNode = mCurrentlySchedulingPassNode,
            concreteFormat,
            writtenMips,
            name,
            this]
            (PipelineResourceSchedulingInfo& schedulingInfo)
        {
            bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(schedulingInfo.ResourceFormat().GetTextureProperties().Format);

            assert_format(concreteFormat || !isTypeless, "Redefinition of Render target format is not allowed");
            assert_format(!concreteFormat || isTypeless, "Render target is typeless and concrete color format was not provided");

            RegisterGraphDependency(*passNode, writtenMips, name, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
            UpdateSubresourceInfos(
                schedulingInfo,
                writtenMips,
                passNode->PassMetadata().Name,
                HAL::ResourceState::RenderTarget,
                PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureRT,
                isTypeless ? concreteFormat : std::nullopt);
        });
    }

    void ResourceScheduler::UseDepthStencil(Foundation::Name resourceName)
    {
        AliasAndUseDepthStencil(resourceName, {});
    }

    void ResourceScheduler::AliasAndUseDepthStencil(Foundation::Name resourceName, Foundation::Name outputAliasName)
    {
        Foundation::Name name = outputAliasName.IsValid() ? outputAliasName : resourceName;
        mCurrentlySchedulingPassNode->AddWriteDependency(name, 1);

        mResourceStorage->QueueResourceUsage(resourceName, outputAliasName.IsValid() ? std::optional(outputAliasName) : std::nullopt, 

            [passNode = mCurrentlySchedulingPassNode,
            name,
            this]
            (PipelineResourceSchedulingInfo& schedulingInfo)
        {
            assert_format(std::holds_alternative<HAL::DepthStencilFormat>(schedulingInfo.ResourceFormat().GetTextureProperties().Format), "Cannot reuse non-depth-stencil texture");

            RegisterGraphDependency(*passNode, MipSet::FirstMip(), name, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
            UpdateSubresourceInfos(
                schedulingInfo,
                MipSet::FirstMip(),
                passNode->PassMetadata().Name,
                HAL::ResourceState::DepthWrite,
                PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureDS,
                std::nullopt);
        });
    }

    void ResourceScheduler::ReadTexture(Foundation::Name resourceName, const MipSet& readMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        mResourceStorage->QueueResourceUsage(resourceName, {},
            [passNode = mCurrentlySchedulingPassNode, 
            readMips,
            concreteFormat,
            resourceName,
            this]
            (PipelineResourceSchedulingInfo& schedulingInfo)
        {
            bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(schedulingInfo.ResourceFormat().GetTextureProperties().Format);
            assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");

            HAL::ResourceState state = HAL::ResourceState::AnyShaderAccess;

            if (std::holds_alternative<HAL::DepthStencilFormat>(schedulingInfo.ResourceFormat().GetTextureProperties().Format))
            {
                state |= HAL::ResourceState::DepthRead;
            }

            RegisterGraphDependency(*passNode, readMips, resourceName, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, false);
            UpdateSubresourceInfos(
                schedulingInfo,
                readMips,
                passNode->PassMetadata().Name,
                state,
                PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureSR,
                std::nullopt);
        });
    }

    void ResourceScheduler::WriteTexture(Foundation::Name resourceName, const MipSet& writtenMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        AliasAndWriteTexture(resourceName, {}, writtenMips, concreteFormat);
    }

    void ResourceScheduler::AliasAndWriteTexture(Foundation::Name resourceName, Foundation::Name outputAliasName, const MipSet& writtenMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        Foundation::Name name = outputAliasName.IsValid() ? outputAliasName : resourceName;

        mResourceStorage->QueueResourceUsage(resourceName, outputAliasName.IsValid() ? std::optional(outputAliasName) : std::nullopt, 

            [passNode = mCurrentlySchedulingPassNode,
            concreteFormat,
            name,
            writtenMips,
            this]
            (PipelineResourceSchedulingInfo& schedulingInfo)
        {
            bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(schedulingInfo.ResourceFormat().GetTextureProperties().Format);

            assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");
            assert_format(!concreteFormat || isTypeless, "Texture is typeless and concrete color format was not provided");

            RegisterGraphDependency(*passNode, writtenMips, name, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
            UpdateSubresourceInfos(
                schedulingInfo,
                writtenMips,
                passNode->PassMetadata().Name,
                HAL::ResourceState::UnorderedAccess,
                PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureUA,
                isTypeless ? concreteFormat : std::nullopt);
        });
    }

    void ResourceScheduler::ReadBuffer(Foundation::Name resourceName, BufferReadContext readContext)
    {
        assert_format(false, "Not implemented");
    }

    void ResourceScheduler::WriteBuffer(Foundation::Name resourceName)
    {
        assert_format(false, "Not implemented");
    }

    void ResourceScheduler::WriteBuffer(Foundation::Name resourceName, Foundation::Name outputAliasName)
    {
        assert_format(false, "Not implemented");
    }

    void ResourceScheduler::WriteToBackBuffer()
    {
        mCurrentlySchedulingPassNode->WritesToBackBuffer = true;
    }

    void ResourceScheduler::ExecuteOnQueue(RenderPassExecutionQueue queue)
    {
        mCurrentlySchedulingPassNode->ExecutionQueueIndex = std::underlying_type_t<RenderPassExecutionQueue>(queue);
    }

    void ResourceScheduler::UseRayTracing()
    {
        mCurrentlySchedulingPassNode->UsesRayTracing = true;
    }

    void ResourceScheduler::SetCurrentlySchedulingPassNode(RenderPassGraph::Node* node)
    {
        mCurrentlySchedulingPassNode = node;
    }

    ResourceScheduler::NewTextureProperties ResourceScheduler::FillMissingFields(std::optional<NewTextureProperties> properties) const
    {
        NewTextureProperties filledProperties{
            mUtilityProvider->DefaultRenderSurfaceDescription.RenderTargetFormat(),
            HAL::TextureKind::Texture2D,
            mUtilityProvider->DefaultRenderSurfaceDescription.Dimensions(),
            std::nullopt,
            HAL::ColorClearValue{ 0, 0, 0, 1 },
            1,
            properties->Flags
        };

        if (properties)
        {
            if (properties->Kind) filledProperties.Kind = properties->Kind;
            if (properties->Dimensions) filledProperties.Dimensions = properties->Dimensions;
            if (properties->ShaderVisibleFormat) filledProperties.ShaderVisibleFormat = properties->ShaderVisibleFormat;
            if (properties->TypelessFormat) filledProperties.TypelessFormat = properties->TypelessFormat;
            if (properties->ClearValues) filledProperties.ClearValues = properties->ClearValues;

            if (properties->MipCount && properties->MipCount == FullMipChain)
            {
                properties->MipCount = MaxMipCount(*filledProperties.Dimensions);
            }

            if (properties->MipCount) filledProperties.MipCount = properties->MipCount;
        }

        return filledProperties;
    }

    ResourceScheduler::NewDepthStencilProperties ResourceScheduler::FillMissingFields(std::optional<NewDepthStencilProperties> properties) const
    {
        NewDepthStencilProperties filledProperties{
            mUtilityProvider->DefaultRenderSurfaceDescription.DepthStencilFormat(),
            mUtilityProvider->DefaultRenderSurfaceDescription.Dimensions(),
            1,
            properties->Flags
        };

        if (properties)
        {
            if (properties->Format) filledProperties.Format = properties->Format;
            if (properties->Dimensions) filledProperties.Dimensions = properties->Dimensions;

            if (properties->MipCount && properties->MipCount == FullMipChain)
            {
                properties->MipCount = MaxMipCount(*filledProperties.Dimensions);
            }

            if (properties->MipCount) filledProperties.MipCount = properties->MipCount;
        }

        return filledProperties;
    }

    uint32_t ResourceScheduler::MaxMipCount(const Geometry::Dimensions& dimensions) const
    {
        return 1 + floor(log2(dimensions.LargestDimension()));
    }

    void ResourceScheduler::RegisterGraphDependency(RenderPassGraph::Node& passNode, const MipSet& mips, Foundation::Name resourceName, uint32_t resourceMipCount, bool isWriteDependency)
    {
        if (!mips.Combination)
        {
            // No dependency is a valid case also
            return;
        }

        if (const MipList* explicitMipList = std::get_if<0>(&mips.Combination.value()))
        {
            isWriteDependency ?
                passNode.AddWriteDependency(resourceName, *explicitMipList) :
                passNode.AddReadDependency(resourceName, *explicitMipList);
        }
        else if (const MipRange* mipRange = std::get_if<1>(&mips.Combination.value()))
        {
            uint32_t lastMip = mipRange->second.value_or(resourceMipCount - 1);

            isWriteDependency ?
                passNode.AddWriteDependency(resourceName, mipRange->first, lastMip) :
                passNode.AddReadDependency(resourceName, mipRange->first, lastMip);
        }
        else if (const uint32_t* indexFromStart = std::get_if<2>(&mips.Combination.value()))
        {
            isWriteDependency ?
                passNode.AddWriteDependency(resourceName, *indexFromStart, *indexFromStart) :
                passNode.AddReadDependency(resourceName, *indexFromStart, *indexFromStart);
        }
        else if (const uint32_t* indexFromEnd = std::get_if<3>(&mips.Combination.value()))
        {
            uint32_t index = resourceMipCount - *indexFromEnd - 1;

            isWriteDependency ?
                passNode.AddWriteDependency(resourceName, index, index) :
                passNode.AddReadDependency(resourceName, index, index);
        }
    }

    void ResourceScheduler::UpdateSubresourceInfos(
        PipelineResourceSchedulingInfo& resourceShcedulingInfo, 
        const MipSet& mips,
        Foundation::Name passName,
        HAL::ResourceState state, 
        PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag accessFlag,
        std::optional<HAL::ColorFormat> concreteFormat)
    {
        if (!mips.Combination)
        {
            return;
        }

        uint32_t firstMip = 0;
        uint32_t lastMip = resourceShcedulingInfo.ResourceFormat().GetTextureProperties().MipCount - 1;

        if (const MipList* explicitMipList = std::get_if<0>(&mips.Combination.value()))
        {
            for (uint32_t mip : *explicitMipList)
            {
                resourceShcedulingInfo.SetSubresourceInfo(passName, mip, state, accessFlag, concreteFormat);
            }

            return;
        }
        else if (const MipRange* mipRange = std::get_if<1>(&mips.Combination.value()))
        {
            firstMip = mipRange->first;
            lastMip = mipRange->second.value_or(lastMip);
        }
        else if (const uint32_t* indexFromStart = std::get_if<2>(&mips.Combination.value()))
        {
            firstMip = *indexFromStart;
            lastMip = *indexFromStart;
        }
        else if (const uint32_t* indexFromEnd = std::get_if<3>(&mips.Combination.value()))
        {
            firstMip = lastMip - *indexFromStart;
            lastMip = lastMip - *indexFromStart;
        }

        for (auto mip = firstMip; mip <= lastMip; ++mip)
        {
            resourceShcedulingInfo.SetSubresourceInfo(passName, mip, state, accessFlag, concreteFormat);
        }
    }

    ResourceScheduler::MipSet ResourceScheduler::MipSet::Empty()
    {
        return MipSet{};
    }

    ResourceScheduler::MipSet ResourceScheduler::MipSet::Explicit(const MipList& mips)
    {
        MipSet set{};
        set.Combination = mips;
        return set;
    }

    ResourceScheduler::MipSet ResourceScheduler::MipSet::IndexFromStart(uint32_t index)
    {
        MipSet set{};
        set.Combination = MipSet::MipVariant{};
        set.Combination->emplace<2>(index);
        return set;
    }

    ResourceScheduler::MipSet ResourceScheduler::MipSet::IndexFromEnd(uint32_t index)
    {
        MipSet set{};
        set.Combination = MipSet::MipVariant{};
        set.Combination->emplace<3>(index);
        return set;
    }

    ResourceScheduler::MipSet ResourceScheduler::MipSet::FirstMip()
    {
        return IndexFromStart(0);
    }

    ResourceScheduler::MipSet ResourceScheduler::MipSet::LastMip()
    {
        return IndexFromEnd(0);
    }

    ResourceScheduler::MipSet ResourceScheduler::MipSet::Range(uint32_t firstMip, std::optional<uint32_t> lastMip)
    {
        MipSet set{};
        set.Combination = MipRange{ firstMip, lastMip };
        return set;
    }

}
