namespace PathFinder
{

    template <class ContentMediator>
    template <template <class> class RenderPassT>
    RenderPassContainer<ContentMediator>::PassHelpers<RenderPassT>::PassHelpers(
        RenderPassT<ContentMediator>* renderPass,
        RenderDevice* renderDevice,
        PipelineResourceStorage* resourceStorage,
        RenderPassUtilityProvider* utilityProvider,
        RenderPassGraph* graph,
        uint64_t graphNodeIndex)
        :
        Pass{ renderPass },
        PassCommandRecorder{ renderDevice, graph, graphNodeIndex },
        PassResourceProvider{ resourceStorage, graph, graphNodeIndex },
        PassRootConstantsUpdater{ resourceStorage, graph, graphNodeIndex },
        UtilityProvider{ utilityProvider },
        GraphNodeIndex{ graphNodeIndex } {}

    template <class ContentMediator>
    template <template <class> class RenderPassT>
    RenderContext<ContentMediator> RenderPassContainer<ContentMediator>::PassHelpers<RenderPassT>::GetContext()
    {
        return { &PassCommandRecorder, &PassRootConstantsUpdater, &PassResourceProvider, UtilityProvider };
    }

    template <class ContentMediator>
    RenderPassContainer<ContentMediator>::RenderPassContainer(
        RenderDevice* renderDevice,
        PipelineResourceStorage* resourceStorage,
        RenderPassUtilityProvider* utilityProvider,
        RenderPassGraph* graph)
        :
        mRenderDevice{ renderDevice },
        mResourceStorage{ resourceStorage },
        mUtilityProvider{ utilityProvider },
        mRenderPassGraph{ graph } {}

    template <class ContentMediator>
    void RenderPassContainer<ContentMediator>::AddRenderPass(RenderPass<ContentMediator>* pass)
    {
        if (mRenderPasses.contains(pass->Metadata().Name))
        {
            return;
        }

        uint64_t nodeIndex = mRenderPassGraph->AddPass(pass->Metadata());
        mRenderPasses.emplace(pass->Metadata().Name, PassHelpers<RenderPass>{ pass, mRenderDevice, mResourceStorage, mUtilityProvider, mRenderPassGraph, nodeIndex });
        mResourceStorage->CreatePerPassData(pass->Metadata().Name);
    }

    template <class ContentMediator>
    void RenderPassContainer<ContentMediator>::AddRenderSubPass(RenderSubPass<ContentMediator>* pass)
    {
        if (mRenderSubPasses.contains(pass->Metadata().Name))
        {
            return;
        }

        uint64_t nodeIndex = mRenderPassGraph->AddPass(pass->Metadata());
        mRenderSubPasses.emplace(pass->Metadata().Name, PassHelpers<RenderSubPass>{ pass, mRenderDevice, mResourceStorage, mUtilityProvider, mRenderPassGraph, nodeIndex });
        mResourceStorage->CreatePerPassData(pass->Metadata().Name);
    }

    template <class ContentMediator>
    RenderPassContainer<ContentMediator>::PassHelpers<RenderPass>* RenderPassContainer<ContentMediator>::GetRenderPass(Foundation::Name passName)
    {
        auto it = mRenderPasses.find(passName);
        if (it == mRenderPasses.end()) return nullptr;
        return &(it->second);
    }

    template <class ContentMediator>
    RenderPassContainer<ContentMediator>::PassHelpers<RenderSubPass>* RenderPassContainer<ContentMediator>::GetRenderSubPass(Foundation::Name passName)
    {
        auto it = mRenderSubPasses.find(passName);
        if (it == mRenderSubPasses.end()) return nullptr;
        return &(it->second);
    }

}

