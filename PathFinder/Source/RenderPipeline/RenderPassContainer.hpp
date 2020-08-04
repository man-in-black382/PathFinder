#pragma once

#include "../Foundation/Name.hpp"

#include "RenderPassGraph.hpp"
#include "RenderDevice.hpp"
#include "PipelineResourceStorage.hpp"
#include "RenderPassUtilityProvider.hpp"

#include <robinhood/robin_hood.h>

#include <vector>
#include <string>

namespace PathFinder
{

    template <class ContentMediator>
    class RenderPass;

    template <class ContentMediator>
    class RenderSubPass;

    template <class ContentMediator>
    class RenderPassContainer
    {
    public:
        template <template <class> class RenderPassT>
        struct PassHelpers
        {
            PassHelpers(
                RenderPassT<ContentMediator>* renderPass,
                RenderDevice* renderDevice,
                PipelineResourceStorage* resourceStorage,
                RenderPassUtilityProvider* utilityProvider,
                RenderPassGraph* graph,
                uint64_t graphNodeIndex
            );

            RenderPassT<ContentMediator>* Pass;
            CommandRecorder PassCommandRecorder;
            ResourceProvider PassResourceProvider;
            RootConstantsUpdater PassRootConstantsUpdater;
            RenderPassUtilityProvider* UtilityProvider;
            uint64_t GraphNodeIndex;

            bool ArePipelineStatesScheduled = false;

            RenderContext<ContentMediator> GetContext();
        };

        using RenderPassHelpers = PassHelpers<RenderPass>;
        using RenderSubPassHelpers = PassHelpers<RenderSubPass>;

        RenderPassContainer(RenderDevice* renderDevice, PipelineResourceStorage* resourceStorage, RenderPassUtilityProvider* utilityProvider, RenderPassGraph* graph);

        void AddRenderPass(RenderPass<ContentMediator>* pass);
        void AddRenderSubPass(RenderSubPass<ContentMediator>* pass);

        PassHelpers<RenderPass>* GetRenderPass(Foundation::Name passName);
        PassHelpers<RenderSubPass>* GetRenderSubPass(Foundation::Name passName);

    private:
        robin_hood::unordered_map<Foundation::Name, RenderPassHelpers> mRenderPasses;
        robin_hood::unordered_map<Foundation::Name, RenderSubPassHelpers> mRenderSubPasses;

        RenderDevice* mRenderDevice = nullptr;
        PipelineResourceStorage* mResourceStorage = nullptr;
        RenderPassGraph* mRenderPassGraph = nullptr;
        RenderPassUtilityProvider* mUtilityProvider = nullptr;

    public:
        inline const auto& RenderPasses() const { return mRenderPasses; }
        inline const auto& RenderSubPasses() const { return mRenderSubPasses; }
        inline auto& RenderPasses() { return mRenderPasses; }
        inline auto& RenderSubPasses() { return mRenderSubPasses; }
    };

}

#include "RenderPass.hpp"
#include "RenderSubPass.hpp"
#include "RenderPassContainer.inl"
