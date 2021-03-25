namespace PathFinder
{

    template <class ContentMediator>
    ResourceScheduler<ContentMediator>::ResourceScheduler(
        PipelineResourceStorage* manager, 
        RenderPassUtilityProvider* utilityProvider, 
        RenderPassGraph* passGraph, 
        const PipelineSettings* settings)
        :
        mResourceStorage{ manager },
        mUtilityProvider{ utilityProvider },
        mRenderPassGraph{ passGraph },
        mPipelineSettings{ settings }
    {}

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        NewRenderTarget(resourceName, MipSet::FirstMip(), properties);
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::NewRenderTarget(Foundation::Name resourceName, const MipSet& writtenMips, std::optional<NewTextureProperties> properties)
    {
        NewTextureProperties props = FillMissingFields(properties);

        bool canBeReadAcrossFrames = EnumMaskContains(props.Flags, ResourceSchedulingFlags::CrossFrameRead);

        HAL::FormatVariant format = *props.ShaderVisibleFormat;
        if (props.TypelessFormat) format = *props.TypelessFormat;

        mResourceStorage->QueueResourceAllocationIfNeeded(
            mCurrentlySchedulingPassNode->PassMetadata().Name,
            resourceName,
            HAL::TextureProperties{ format, *props.Kind, *props.Dimensions, *props.ClearValues, HAL::ResourceState::Common, *props.MipCount },
            props.TextureToCopyPropertiesFrom,

            [canBeReadAcrossFrames,
            passNode = mCurrentlySchedulingPassNode,
            typelessFormat = props.TypelessFormat,
            shaderVisibleFormat = props.ShaderVisibleFormat,
            resourceName,
            writtenMips,
            this]
        (PipelineResourceSchedulingInfo& schedulingInfo)
        {
            schedulingInfo.CanBeAliased = !canBeReadAcrossFrames && mPipelineSettings->IsMemoryAliasingEnabled;
            RegisterGraphDependency(*passNode, writtenMips, resourceName, {}, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
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

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties)
    {
        NewDepthStencilProperties props = FillMissingFields(properties);
        bool canBeReadAcrossFrames = EnumMaskContains(props.Flags, ResourceSchedulingFlags::CrossFrameRead);
        HAL::DepthStencilClearValue clearValue{ 1.0, 0 };

        mResourceStorage->QueueResourceAllocationIfNeeded(
            mCurrentlySchedulingPassNode->PassMetadata().Name,
            resourceName,
            HAL::TextureProperties{ *props.Format, HAL::TextureKind::Texture2D, *props.Dimensions, clearValue, HAL::ResourceState::Common, *props.MipCount },
            props.TextureToCopyPropertiesFrom,

            [canBeReadAcrossFrames,
            passNode = mCurrentlySchedulingPassNode,
            resourceName,
            this]
        (PipelineResourceSchedulingInfo& schedulingInfo)
            {
                schedulingInfo.CanBeAliased = !canBeReadAcrossFrames && mPipelineSettings->IsMemoryAliasingEnabled;
                RegisterGraphDependency(*passNode, MipSet::FirstMip(), resourceName, {}, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
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

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        NewTexture(resourceName, MipSet::FirstMip(), properties);
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::NewTexture(Foundation::Name resourceName, const MipSet& writtenMips, std::optional<NewTextureProperties> properties)
    {
        NewTextureProperties props = FillMissingFields(properties);
        bool canBeReadAcrossFrames = EnumMaskContains(props.Flags, ResourceSchedulingFlags::CrossFrameRead);

        HAL::FormatVariant format = *props.ShaderVisibleFormat;
        if (props.TypelessFormat) format = *props.TypelessFormat;

        mResourceStorage->QueueResourceAllocationIfNeeded(
            mCurrentlySchedulingPassNode->PassMetadata().Name,
            resourceName,
            HAL::TextureProperties{ format, *props.Kind, *props.Dimensions, *props.ClearValues, HAL::ResourceState::Common, *props.MipCount },
            props.TextureToCopyPropertiesFrom,

            [canBeReadAcrossFrames,
            passNode = mCurrentlySchedulingPassNode,
            typelessFormat = props.TypelessFormat,
            shaderVisibleFormat = props.ShaderVisibleFormat,
            resourceName,
            writtenMips,
            this]
        (PipelineResourceSchedulingInfo& schedulingInfo)
        {
            schedulingInfo.CanBeAliased = !canBeReadAcrossFrames && mPipelineSettings->IsMemoryAliasingEnabled;
            RegisterGraphDependency(*passNode, writtenMips, resourceName, {}, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
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

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::UseRenderTarget(Foundation::Name resourceName, const MipSet& writtenMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        AliasAndUseRenderTarget(resourceName, {}, writtenMips, concreteFormat);
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::AliasAndUseRenderTarget(Foundation::Name resourceName, Foundation::Name outputAliasName, const MipSet& writtenMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        mResourceStorage->QueueResourceUsage(
            mCurrentlySchedulingPassNode->PassMetadata().Name,
            resourceName, 
            outputAliasName.IsValid() ? std::optional(outputAliasName) : std::nullopt,

            [passNode = mCurrentlySchedulingPassNode,
            concreteFormat,
            writtenMips,
            resourceName,
            outputAliasName,
            this]
        (PipelineResourceSchedulingInfo& schedulingInfo)
            {
                bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(schedulingInfo.ResourceFormat().GetTextureProperties().Format);

                assert_format(concreteFormat || !isTypeless, "Redefinition of Render target format is not allowed");
                assert_format(!concreteFormat || isTypeless, "Render target is typeless and concrete color format was not provided");

                RegisterGraphDependency(*passNode, writtenMips, resourceName, outputAliasName, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
                UpdateSubresourceInfos(
                    schedulingInfo,
                    writtenMips,
                    passNode->PassMetadata().Name,
                    HAL::ResourceState::RenderTarget,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureRT,
                    isTypeless ? concreteFormat : std::nullopt);
            });
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::UseDepthStencil(Foundation::Name resourceName)
    {
        AliasAndUseDepthStencil(resourceName, {});
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::AliasAndUseDepthStencil(Foundation::Name resourceName, Foundation::Name outputAliasName)
    {
        mResourceStorage->QueueResourceUsage(
            mCurrentlySchedulingPassNode->PassMetadata().Name,
            resourceName, 
            outputAliasName.IsValid() ? std::optional(outputAliasName) : std::nullopt,

            [passNode = mCurrentlySchedulingPassNode,
            resourceName,
            outputAliasName,
            this]
        (PipelineResourceSchedulingInfo& schedulingInfo)
            {
                assert_format(std::holds_alternative<HAL::DepthStencilFormat>(schedulingInfo.ResourceFormat().GetTextureProperties().Format), "Cannot reuse non-depth-stencil texture");

                RegisterGraphDependency(*passNode, MipSet::FirstMip(), resourceName, outputAliasName, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
                UpdateSubresourceInfos(
                    schedulingInfo,
                    MipSet::FirstMip(),
                    passNode->PassMetadata().Name,
                    HAL::ResourceState::DepthWrite,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureDS,
                    std::nullopt);
            });
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::ReadTexture(Foundation::Name resourceName, const MipSet& readMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        mResourceStorage->QueueResourceUsage(
            mCurrentlySchedulingPassNode->PassMetadata().Name,
            resourceName, 
            {},

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

                RegisterGraphDependency(*passNode, readMips, resourceName, {}, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, false);
                UpdateSubresourceInfos(
                    schedulingInfo,
                    readMips,
                    passNode->PassMetadata().Name,
                    state,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureSR,
                    std::nullopt);
            });
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::WriteTexture(Foundation::Name resourceName, const MipSet& writtenMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        AliasAndWriteTexture(resourceName, {}, writtenMips, concreteFormat);
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::AliasAndWriteTexture(Foundation::Name resourceName, Foundation::Name outputAliasName, const MipSet& writtenMips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        mResourceStorage->QueueResourceUsage(
            mCurrentlySchedulingPassNode->PassMetadata().Name,
            resourceName, 
            outputAliasName.IsValid() ? std::optional(outputAliasName) : std::nullopt,

            [passNode = mCurrentlySchedulingPassNode,
            concreteFormat,
            resourceName,
            outputAliasName,
            writtenMips,
            this]
        (PipelineResourceSchedulingInfo& schedulingInfo)
            {
                bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(schedulingInfo.ResourceFormat().GetTextureProperties().Format);

                assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");
                assert_format(!concreteFormat || isTypeless, "Texture is typeless and concrete color format was not provided");

                RegisterGraphDependency(*passNode, writtenMips, resourceName, outputAliasName, schedulingInfo.ResourceFormat().GetTextureProperties().MipCount, true);
                UpdateSubresourceInfos(
                    schedulingInfo,
                    writtenMips,
                    passNode->PassMetadata().Name,
                    HAL::ResourceState::UnorderedAccess,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureUA,
                    isTypeless ? concreteFormat : std::nullopt);
            });
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::ReadBuffer(Foundation::Name resourceName, BufferReadContext readContext)
    {
        assert_format(false, "Not implemented");
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::WriteBuffer(Foundation::Name resourceName)
    {
        assert_format(false, "Not implemented");
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::WriteBuffer(Foundation::Name resourceName, Foundation::Name outputAliasName)
    {
        assert_format(false, "Not implemented");
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::WriteToBackBuffer()
    {
        mCurrentlySchedulingPassNode->AddWriteDependency(RenderPassGraph::Node::BackBufferName, std::nullopt, 1);
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::ExecuteOnQueue(RenderPassExecutionQueue queue)
    {
        // Ignore render pass queue preference if async is disabled
        mCurrentlySchedulingPassNode->ExecutionQueueIndex =
            mPipelineSettings->IsAsyncComputeEnabled ? 
            std::underlying_type_t<RenderPassExecutionQueue>(queue) : 
            std::underlying_type_t<RenderPassExecutionQueue>(RenderPassExecutionQueue::Graphics);
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::UseRayTracing()
    {
        mCurrentlySchedulingPassNode->UsesRayTracing = true;
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::Export(Foundation::Name resourceName)
    {
        mResourceStorage->QueueResourceReadback(mCurrentlySchedulingPassNode->PassMetadata().Name, resourceName,
            [resourceName, node = mCurrentlySchedulingPassNode](PipelineResourceSchedulingInfo& schedulingInfo)
        {
            PipelineResourceSchedulingInfo::PassInfo* passInfo = schedulingInfo.GetInfoForPass(node->PassMetadata().Name);
            assert_format(passInfo, "Resource ", resourceName.ToString(), " wasn't scheduled for usage in ", node->PassMetadata().Name.ToString());
            passInfo->IsReadbackRequested = true;
        });
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::SetCurrentlySchedulingPassNode(RenderPassGraph::Node* node)
    {
        mCurrentlySchedulingPassNode = node;
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::SetContent(const ContentMediator* content)
    {
        mContent = content;
    }

    template <class ContentMediator>
    NewTextureProperties ResourceScheduler<ContentMediator>::FillMissingFields(std::optional<NewTextureProperties> properties) const
    {
        NewTextureProperties filledProperties{
            mUtilityProvider->DefaultRenderSurfaceDescription.RenderTargetFormat(),
            HAL::TextureKind::Texture2D,
            mUtilityProvider->DefaultRenderSurfaceDescription.Dimensions(),
            std::nullopt,
            HAL::ColorClearValue{ 0, 0, 0, 1 },
            1,
            properties ? properties->Flags : ResourceSchedulingFlags::None
        };

        if (properties)
        {
            if (properties->Kind) filledProperties.Kind = properties->Kind;
            if (properties->Dimensions) filledProperties.Dimensions = properties->Dimensions;
            if (properties->ShaderVisibleFormat) filledProperties.ShaderVisibleFormat = properties->ShaderVisibleFormat;
            if (properties->TypelessFormat) filledProperties.TypelessFormat = properties->TypelessFormat;
            if (properties->ClearValues) filledProperties.ClearValues = properties->ClearValues;

            if (properties->MipCount && properties->MipCount == NewTextureProperties::FullMipChain)
            {
                properties->MipCount = MaxMipCount(*filledProperties.Dimensions);
            }

            if (properties->MipCount) filledProperties.MipCount = properties->MipCount;
            if (properties->TextureToCopyPropertiesFrom) filledProperties.TextureToCopyPropertiesFrom = properties->TextureToCopyPropertiesFrom;
        }

        return filledProperties;
    }

    template <class ContentMediator>
    NewDepthStencilProperties ResourceScheduler<ContentMediator>::FillMissingFields(std::optional<NewDepthStencilProperties> properties) const
    {
        NewDepthStencilProperties filledProperties{
            mUtilityProvider->DefaultRenderSurfaceDescription.DepthStencilFormat(),
            mUtilityProvider->DefaultRenderSurfaceDescription.Dimensions(),
            1,
            properties ? properties->Flags : ResourceSchedulingFlags::None
        };

        if (properties)
        {
            if (properties->Format) filledProperties.Format = properties->Format;
            if (properties->Dimensions) filledProperties.Dimensions = properties->Dimensions;

            if (properties->MipCount && properties->MipCount == NewDepthStencilProperties::FullMipChain)
            {
                properties->MipCount = MaxMipCount(*filledProperties.Dimensions);
            }

            if (properties->MipCount) filledProperties.MipCount = properties->MipCount;
            if (properties->TextureToCopyPropertiesFrom) filledProperties.TextureToCopyPropertiesFrom = properties->TextureToCopyPropertiesFrom;
        }

        return filledProperties;
    }

    template <class ContentMediator>
    uint32_t ResourceScheduler<ContentMediator>::MaxMipCount(const Geometry::Dimensions& dimensions) const
    {
        return 1 + floor(log2(dimensions.LargestDimension()));
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::RegisterGraphDependency(
        RenderPassGraph::Node& passNode,
        const MipSet& mips,
        Foundation::Name resourceName,
        Foundation::Name outputAliasName,
        uint32_t resourceMipCount,
        bool isWriteDependency)
    {
        if (!mips.Combination)
        {
            // No dependency is a valid case also
            return;
        }

        // If resource name aliasing is involved we need to provide both new name and old name to the graph
        Foundation::Name newResourceName = outputAliasName.IsValid() ? outputAliasName : resourceName;
        std::optional<Foundation::Name> originalResourceName = outputAliasName.IsValid() ? std::optional(resourceName) : std::nullopt;

        if (const MipList* explicitMipList = std::get_if<0>(&mips.Combination.value()))
        {
            isWriteDependency ?
                passNode.AddWriteDependency(newResourceName, originalResourceName, *explicitMipList) :
                passNode.AddReadDependency(newResourceName, *explicitMipList);
        }
        else if (const MipRange* mipRange = std::get_if<1>(&mips.Combination.value()))
        {
            uint32_t lastMip = mipRange->second.value_or(resourceMipCount - 1);

            isWriteDependency ?
                passNode.AddWriteDependency(newResourceName, originalResourceName, mipRange->first, lastMip) :
                passNode.AddReadDependency(newResourceName, mipRange->first, lastMip);
        }
        else if (const uint32_t* indexFromStart = std::get_if<2>(&mips.Combination.value()))
        {
            isWriteDependency ?
                passNode.AddWriteDependency(newResourceName, originalResourceName, *indexFromStart, *indexFromStart) :
                passNode.AddReadDependency(newResourceName, *indexFromStart, *indexFromStart);
        }
        else if (const uint32_t* indexFromEnd = std::get_if<3>(&mips.Combination.value()))
        {
            uint32_t index = resourceMipCount - *indexFromEnd - 1;

            isWriteDependency ?
                passNode.AddWriteDependency(newResourceName, originalResourceName, index, index) :
                passNode.AddReadDependency(newResourceName, index, index);
        }
    }

    template <class ContentMediator>
    void ResourceScheduler<ContentMediator>::UpdateSubresourceInfos(
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
            firstMip = lastMip - *indexFromEnd;
            lastMip = lastMip - *indexFromEnd;
        }

        for (auto mip = firstMip; mip <= lastMip; ++mip)
        {
            resourceShcedulingInfo.SetSubresourceInfo(passName, mip, state, accessFlag, concreteFormat);
        }
    }

    template <class ContentMediator>
    template <class T>
    void ResourceScheduler<ContentMediator>::NewBuffer(Foundation::Name resourceName, const NewBufferProperties<T>& bufferProperties)
    {
        bool canBeReadAcrossFrames = EnumMaskContains(bufferProperties.Flags, ResourceSchedulingFlags::CrossFrameRead);

        mResourceStorage->QueueResourceAllocationIfNeeded(
            mCurrentlySchedulingPassNode->PassMetadata().Name,
            resourceName,
            HAL::BufferProperties::Create<T>(bufferProperties.Capacity, bufferProperties.PerElementAlignment),
            bufferProperties.BufferToCopyPropertiesFrom,

            [this, resourceName, canBeReadAcrossFrames, node = mCurrentlySchedulingPassNode](PipelineResourceSchedulingInfo& schedulingInfo)
            {
                RegisterGraphDependency(*node, MipSet::FirstMip(), resourceName, {}, 1, true);

                schedulingInfo.SetSubresourceInfo(
                    node->PassMetadata().Name,
                    0, 
                    HAL::ResourceState::UnorderedAccess,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::BufferUA,
                    std::nullopt);

                schedulingInfo.CanBeAliased = !canBeReadAcrossFrames && mPipelineSettings->IsMemoryAliasingEnabled;
            }
        );
    }

    template <class ContentMediator>
    template <class Lambda>
    void ResourceScheduler<ContentMediator>::FillCurrentPassInfo(const PipelineResourceStorageResource* resourceData, const MipList& mipList, const Lambda& lambda)
    {
        if (mipList.empty())
        {
            for (auto subresourceIdx = 0u; subresourceIdx < resourceData->SchedulingInfo.SubresourceCount(); ++subresourceIdx)
            {
                lambda(subresourceIdx);
            }
        }
        else
        {
            for (uint8_t mip : mipList)
            {
                lambda(mip);
            }
        }
    }

}

