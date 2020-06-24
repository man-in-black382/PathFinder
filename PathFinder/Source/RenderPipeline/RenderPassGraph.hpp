#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceState.hpp"

#include "RenderPassMetadata.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <functional>
#include <stack>

namespace PathFinder
{

    class RenderPassGraph
    {
    public:
        using SubresourceName = uint64_t;
        using WriteDependencyRegistry = std::unordered_set<SubresourceName>;

        class Node
        {
        public:
            using SynchronizationIndexSet = std::vector<uint64_t>;
            using SubresourceList = std::vector<uint32_t>;
            using QueueIndex = uint64_t;

            Node(const RenderPassMetadata& passMetadata, WriteDependencyRegistry* writeDependencyRegistry);

            bool operator==(const Node& that) const;
            bool operator!=(const Node& that) const;

            void AddReadDependency(Foundation::Name resourceName, uint32_t subresourceCount);
            void AddReadDependency(Foundation::Name resourceName, uint32_t firstSubresourceIndex, uint32_t lastSubresourceIndex);
            void AddReadDependency(Foundation::Name resourceName, const SubresourceList& subresources);

            void AddWriteDependency(Foundation::Name resourceName, uint32_t subresourceCount);
            void AddWriteDependency(Foundation::Name resourceName, uint32_t firstSubresourceIndex, uint32_t lastSubresourceIndex);
            void AddWriteDependency(Foundation::Name resourceName, const SubresourceList& subresources);

            bool HasDependency(Foundation::Name resourceName, uint32_t subresourceIndex) const;

            uint64_t ExecutionQueueIndex = 0;
            bool UsesRayTracing = false;

        private:
            friend RenderPassGraph;

            SubresourceName CreateSubresourceName(Foundation::Name resourceName, uint32_t subresourceIndex) const;
            void EnsureSingleWriteDependency(SubresourceName name);
            void Clear();

            uint64_t mGlobalExecutionIndex = 0;
            uint64_t mDependencyLevelIndex = 0;
            uint64_t mLocalToDependencyLevelExecutionIndex = 0;
            uint64_t mLocalToQueueExecutionIndex = 0;
            uint64_t mIndexInUnorderedList = 0;

            RenderPassMetadata mPassMetadata;
            WriteDependencyRegistry* mWriteDependencyRegistry = nullptr;
            std::unordered_set<SubresourceName> mReadSubresources;
            std::unordered_set<SubresourceName> mWrittenSubresources;
            std::unordered_set<SubresourceName> mAllSubresources;
            std::unordered_set<Foundation::Name> mAllResources;
            SynchronizationIndexSet mSynchronizationIndexSet;
            std::vector<const Node*> mNodesToSyncWith;
            bool mSyncSignalRequired = false;

        public:
            inline const auto& PassMetadata() const { return mPassMetadata; }
            inline const auto& ReadSubresources() const { return mReadSubresources; }
            inline const auto& WrittenSubresources() const { return mWrittenSubresources; }
            inline const auto& AllSubresources() const { return mAllSubresources; }
            inline const auto& AllResources() const { return mAllResources; }
            inline const auto& NodesToSyncWith() const { return mNodesToSyncWith; }
            inline auto GlobalExecutionIndex() const { return mGlobalExecutionIndex; }
            inline auto DependencyLevelIndex() const { return mDependencyLevelIndex; }
            inline auto LocalToDependencyLevelExecutionIndex() const { return mLocalToDependencyLevelExecutionIndex; }
            inline auto LocalToQueueExecutionIndex() const { return mLocalToQueueExecutionIndex; }
            inline bool IsSyncSignalRequired() const { return mSyncSignalRequired; }
        };

        class DependencyLevel
        {
        public:
            friend RenderPassGraph;

            using NodeList = std::list<Node*>;
            using NodeIterator = NodeList::iterator;

        private:
            void AddNode(Node* node);
            Node* RemoveNode(NodeIterator it);

            uint64_t mLevelIndex = 0;
            NodeList mNodes;
            std::vector<std::vector<const Node*>> mNodesPerQueue;

            // Storage for queues that read at least one common resource. Resource state transitions
            // for such queues need to be handled differently.
            std::unordered_set<Node::QueueIndex> mQueuesInvoledInCrossQueueResourceReads;
            std::unordered_set<SubresourceName> mSubresourcesReadByMultipleQueues;

        public:
            inline const auto& Nodes() const { return mNodes; }
            inline const auto& NodesForQueue(Node::QueueIndex queueIndex) const { return mNodesPerQueue[queueIndex]; }
            inline const auto& QueuesInvoledInCrossQueueResourceReads() const { return mQueuesInvoledInCrossQueueResourceReads; }
            inline const auto& SubresourcesReadByMultipleQueues() const { return mSubresourcesReadByMultipleQueues; }
            inline auto LevelIndex() const { return mLevelIndex; }
        };

        using NodeList = std::vector<Node>;
        using NodeListIterator = NodeList::iterator;
        using ResourceUsageTimeline = std::pair<uint64_t, uint64_t>;
        using ResourceUsageTimelines = std::unordered_map<Foundation::Name, ResourceUsageTimeline>;

        static std::pair<Foundation::Name, uint32_t> DecodeSubresourceName(SubresourceName name);

        uint64_t NodeCountForQueue(uint64_t queueIndex) const;
        const ResourceUsageTimeline& GetResourceUsageTimeline(Foundation::Name resourceName) const;

        void AddPass(const RenderPassMetadata& passMetadata);
        void RemovePass(NodeListIterator it);

        void Build();
        void Clear();

        void IterateNodesInExecutionOrder(const std::function<void(const Node&)>& iterator) const;

    private:
        using DependencyLevelList = std::vector<DependencyLevel>;
        using OrderedNodeList = std::vector<Node*>;
        using RenderPassRegistry = std::unordered_set<Foundation::Name>;
        using QueueNodeCounters = std::unordered_map<uint64_t, uint64_t>;
        using AdjacencyLists = std::vector<std::vector<uint64_t>>;

        enum class NodeMark
        {
            Temporary, Permanent
        };

        void EnsureRenderPassUniqueness(Foundation::Name passName);
        void BuildAdjacencyLists();
        void DepthFirstSearch(uint64_t nodeIndex, std::vector<bool>& visited, std::vector<bool>& onStack, bool& isCyclic);
        void TopologicalSort();
        void BuildDependencyLevels();
        void FinalizeDependencyLevels();
        void CullRedundantSynchronizations();

        NodeList mPassNodes;
        AdjacencyLists mAdjacencyLists;
        DependencyLevelList mDependencyLevels;

        // In order to avoid any unambiguity in graph nodes execution order
        // and avoid cyclic dependencies to make graph builds fully automatic
        // we must ensure that there can only be one write dependency for each subresource in a frame
        WriteDependencyRegistry mGlobalWriteDependencyRegistry;

        ResourceUsageTimelines mResourceUsageTimelines;
        RenderPassRegistry mRenderPassRegistry;
        QueueNodeCounters mQueueNodeCounters;
        OrderedNodeList mTopologicallySortedNodes;
        OrderedNodeList mNodesInGlobalExecutionOrder;
        const Node* mFirstNodeThatUsesRayTracing = nullptr;
        uint64_t mDetectedQueueCount = 1;

    public:
        inline const auto& NodesInGlobalExecutionOrder() const { return mNodesInGlobalExecutionOrder; }
        inline const auto& Nodes() const { return mPassNodes; }
        inline auto& Nodes() { return mPassNodes; }
        inline const auto& DependencyLevels() const { return mDependencyLevels; }
        inline const Node* FirstNodeThatUsesRayTracing() const { return mFirstNodeThatUsesRayTracing; }
        inline auto DetectedQueueCount() const { return mDetectedQueueCount; }
    };

}
