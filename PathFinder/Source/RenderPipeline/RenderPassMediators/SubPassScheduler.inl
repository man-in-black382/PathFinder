namespace PathFinder
{

    template <class ContentMediator>
    SubPassScheduler<ContentMediator>::SubPassScheduler(RenderPassContainer<ContentMediator>* passContainer, const PipelineResourceStorage* resourceStorage, const RenderPassUtilityProvider* utilityProvider)
        : mPassContainer{ passContainer }, mResourceStorage{ resourceStorage }, mUtilityProvider{ utilityProvider } {}

    template <class ContentMediator>
    void SubPassScheduler<ContentMediator>::AddRenderSubPass(RenderSubPass<ContentMediator>* pass)
    {
        mPassContainer->AddRenderSubPass(pass);
    }

    template <class ContentMediator>
    const HAL::BufferProperties& SubPassScheduler<ContentMediator>::GetBufferProperties(Foundation::Name bufferName) const
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(bufferName);
        assert_format(resourceObjects, "Buffer ", bufferName.ToString(), " wasn't scheduled for creation");
        return resourceObjects->SchedulingInfo.ResourceFormat().GetBufferProperties();
    }

    template <class ContentMediator>
    const HAL::TextureProperties& SubPassScheduler<ContentMediator>::GetTextureProperties(Foundation::Name textureName) const
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(textureName);
        assert_format(resourceObjects, "Texture ", textureName.ToString(), " wasn't scheduled for creation");
        return resourceObjects->SchedulingInfo.ResourceFormat().GetTextureProperties();
    }

}