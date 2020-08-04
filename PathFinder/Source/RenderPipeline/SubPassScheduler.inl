namespace PathFinder
{

    template <class ContentMediator>
    SubPassScheduler<ContentMediator>::SubPassScheduler(const RenderPassContainer<ContentMediator>* passContainer, const PipelineResourceStorage* resourceStorage, const RenderPassUtilityProvider* utilityProvider)
        : mPassContainer{ passContainer }, mResourceStorage{ resourceStorage }, mUtilityProvider{ utilityProvider } {}

    template <class ContentMediator>
    void SubPassScheduler<ContentMediator>::AddRenderSubPass(const RenderSubPass<ContentMediator>* pass)
    {
        mPassContainer->AddRenderSubPass(pass);
    }

    template <class ContentMediator>
    const HAL::BufferProperties<uint8_t>& SubPassScheduler<ContentMediator>::GetBufferProperties(Foundation::Name textureName) const
    {
        assert_format(false, "Buffers are not implemented");
    }

    template <class ContentMediator>
    const HAL::TextureProperties& SubPassScheduler<ContentMediator>::GetTextureProperties(Foundation::Name textureName) const
    {
        PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(textureName);
        assert_format(resourceObjects, "Resource ", textureName.ToString(), " wasn't scheduled for creation");
        return resourceObjects->SchedulingInfo.ResourceFormat().GetTextureProperties();
    }

}