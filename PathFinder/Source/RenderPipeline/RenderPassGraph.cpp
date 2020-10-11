#include "RenderPassGraph.hpp"

#include "RenderPass.hpp"



namespace PathFinder
{

    RenderPassGraph::SubresourceName RenderPassGraph::ConstructSubresourceName(Foundation::Name resourceName, uint32_t subresourceIndex)
    {
        SubresourceName name = resourceName.ToId();
        name <<= 32;
        name |= subresourceIndex;
        return name;
    }

    std::pair<Foundation::Name, uint32_t> RenderPassGraph::DecodeSubresourceName(SubresourceName name)
    {
        return { Foundation::Name{ name >> 32 }, name & 0x0000FFFF };
    }

    uint64_t RenderPassGraph::NodeCountForQueue(uint64_t queueIndex) const
    {
        auto countIt = mQueueNodeCounters.find(queueIndex);
        return countIt != mQueueNodeCounters.end() ? countIt->second : 0;
    }

    const RenderPassGraph::ResourceUsageTimeline& RenderPassGraph::GetResourceUsageTimeline(Foundation::Name resourceName) const
    {
        auto timelineIt = mResourceUsageTimelines.find(resourceName);
        assert_format(timelineIt != mResourceUsageTimelines.end(), "Resource timeline (", resourceName.ToString(), ") doesn't exist");
        return timelineIt->second;
    }

    const RenderPassGraph::Node* RenderPassGraph::GetNodeThatWritesToSubresource(SubresourceName subresourceName) const
    {
        auto it = mWrittenSubresourceToPassMap.find(subresourceName);
        assert_format(it != mWrittenSubresourceToPassMap.end(), "Subresource ", DecodeSubresourceName(subresourceName).first.ToString(), " is not registered for writing in the graph.");
        return it->second;
    }

    uint64_t RenderPassGraph::AddPass(const RenderPassMetadata& passMetadata)
    {
        EnsureRenderPassUniqueness(passMetadata.Name);
        mPassNodes.emplace_back(Node{ passMetadata, &mGlobalWriteDependencyRegistry });
        mPassNodes.back().mIndexInUnorderedList = mPassNodes.size() - 1;
        return mPassNodes.size() - 1;
    }

    void RenderPassGraph::Build()
    {
        BuildAdjacencyLists();
        TopologicalSort();
        BuildDependencyLevels();
        FinalizeDependencyLevels();
        CullRedundantSynchronizations();
    }

    void RenderPassGraph::Clear()
    {
        mGlobalWriteDependencyRegistry.clear();
        mDependencyLevels.clear();
        mResourceUsageTimelines.clear();
        mQueueNodeCounters.clear();
        mTopologicallySortedNodes.clear();
        mNodesInGlobalExecutionOrder.clear();
        mAdjacencyLists.clear();
        mFirstNodeThatUsesRayTracing = nullptr;
        mDetectedQueueCount = 1;

        for (Node& node : mPassNodes)
        {
            node.Clear();
        }
    }

    void RenderPassGraph::EnsureRenderPassUniqueness(Foundation::Name passName)
    {
        assert_format(mRenderPassRegistry.find(passName) == mRenderPassRegistry.end(),
            "Render pass ", passName.ToString(), " is already added to the graph.");

        mRenderPassRegistry.insert(passName);
    }

    void RenderPassGraph::BuildAdjacencyLists()
    {
        mAdjacencyLists.resize(mPassNodes.size());

        for (auto nodeIdx = 0; nodeIdx < mPassNodes.size(); ++nodeIdx)
        {
            Node& node = mPassNodes[nodeIdx];

            if (!node.HasAnyDependencies())
            {
                continue;
            }

            std::vector<uint64_t>& adjacentNodeIndices = mAdjacencyLists[nodeIdx];

            for (auto otherNodeIdx = 0; otherNodeIdx < mPassNodes.size(); ++otherNodeIdx)
            {
                // Do not check dependencies on itself
                if (nodeIdx == otherNodeIdx) continue;

                Node& otherNode = mPassNodes[otherNodeIdx];

                auto establishAdjacency = [&](SubresourceName otherNodeReadResource) -> bool
                {
                    // If other node reads a subresource written by the current node, then it depends on current node and is an adjacent dependency
                    bool otherNodeDependsOnCurrentNode =
                        node.WrittenSubresources().find(otherNodeReadResource) != node.WrittenSubresources().end();

                    if (otherNodeDependsOnCurrentNode)
                    {
                        adjacentNodeIndices.push_back(otherNodeIdx);

                        if (node.ExecutionQueueIndex != otherNode.ExecutionQueueIndex)
                        {
                            node.mSyncSignalRequired = true;
                            otherNode.mNodesToSyncWith.push_back(&node);
                        }

                        return true;
                    }

                    return false;
                };

                for (SubresourceName otherNodeReadResource : otherNode.ReadSubresources())
                {
                    if (establishAdjacency(otherNodeReadResource)) break;
                }

                for (SubresourceName otherNodeReadResource : otherNode.mAliasedSubresources)
                {
                    if (establishAdjacency(otherNodeReadResource)) break;
                }
            }
        }
    }

    void RenderPassGraph::DepthFirstSearch(uint64_t nodeIndex, std::vector<bool>& visited, std::vector<bool>& onStack, bool& isCyclic)
    {
        if (isCyclic) return;

        visited[nodeIndex] = true;
        onStack[nodeIndex] = true;

        uint64_t adjacencyListIndex = mPassNodes[nodeIndex].mIndexInUnorderedList;

        for (uint64_t neighbour : mAdjacencyLists[adjacencyListIndex])
        {
            if (visited[neighbour] && onStack[neighbour])
            {
                isCyclic = true;
                return;
            }

            if (!visited[neighbour]) 
            {
                DepthFirstSearch(neighbour, visited, onStack, isCyclic);
            }
        }

        onStack[nodeIndex] = false;
        mTopologicallySortedNodes.push_back(&mPassNodes[nodeIndex]);
    }

    void RenderPassGraph::TopologicalSort()
    {
        std::vector<bool> visitedNodes(mPassNodes.size(), false);
        std::vector<bool> onStackNodes(mPassNodes.size(), false);

        bool isCyclic = false;

        for (auto nodeIndex = 0; nodeIndex < mPassNodes.size(); ++nodeIndex)
        {
            const Node& node = mPassNodes[nodeIndex];

            // Visited nodes and nodes without outputs are not processed
            if (!visitedNodes[nodeIndex] && node.HasAnyDependencies())
            {
                DepthFirstSearch(nodeIndex, visitedNodes, onStackNodes, isCyclic);
                assert_format(!isCyclic, "Detected cyclic dependency in pass: ", node.PassMetadata().Name.ToString());
            }
        }

        std::reverse(mTopologicallySortedNodes.begin(), mTopologicallySortedNodes.end());
    }

    void RenderPassGraph::BuildDependencyLevels()
    {
        std::vector<int64_t> longestDistances(mPassNodes.size(), 0);

        uint64_t dependencyLevelCount = 1;

        // Perform longest node distance search
        for (auto nodeIndex = 0; nodeIndex < mTopologicallySortedNodes.size(); ++nodeIndex)
        {
            uint64_t originalIndex = mTopologicallySortedNodes[nodeIndex]->mIndexInUnorderedList;
            uint64_t adjacencyListIndex = originalIndex;

            for (uint64_t adjacentNodeIndex : mAdjacencyLists[adjacencyListIndex])
            {
                if (longestDistances[adjacentNodeIndex] < longestDistances[originalIndex] + 1)
                {
                    int64_t newLongestDistance = longestDistances[originalIndex] + 1;
                    longestDistances[adjacentNodeIndex] = newLongestDistance;
                    dependencyLevelCount = std::max(uint64_t(newLongestDistance + 1), dependencyLevelCount);
                }
            }
        }

        mDependencyLevels.resize(dependencyLevelCount);
        mDetectedQueueCount = 1;

        // Dispatch nodes to corresponding dependency levels.
        for (auto nodeIndex = 0; nodeIndex < mTopologicallySortedNodes.size(); ++nodeIndex)
        {
            Node* node = mTopologicallySortedNodes[nodeIndex];
            uint64_t levelIndex = longestDistances[node->mIndexInUnorderedList];
            DependencyLevel& dependencyLevel = mDependencyLevels[levelIndex];
            dependencyLevel.mLevelIndex = levelIndex;
            dependencyLevel.AddNode(node);
            node->mDependencyLevelIndex = levelIndex;
            mDetectedQueueCount = std::max(mDetectedQueueCount, node->ExecutionQueueIndex + 1);
        }
    }

    void RenderPassGraph::FinalizeDependencyLevels()
    {
        uint64_t globalExecutionIndex = 0;
        bool firstRayTracingUserDetected = false;

        mNodesInGlobalExecutionOrder.resize(mTopologicallySortedNodes.size(), nullptr);
        std::vector<const Node*> perQueuePreviousNodes(mDetectedQueueCount, nullptr);
        
        for (DependencyLevel& dependencyLevel : mDependencyLevels)
        {
            uint64_t localExecutionIndex = 0;

            std::unordered_map<SubresourceName, std::unordered_set<Node::QueueIndex>> resourceReadingQueueTracker;
            dependencyLevel.mNodesPerQueue.resize(mDetectedQueueCount);

            for (Node* node : dependencyLevel.mNodes)
            {
                // Track which resource is read by which queue in this dependency level
                for (SubresourceName subresourceName : node->ReadSubresources())
                {
                    resourceReadingQueueTracker[subresourceName].insert(node->ExecutionQueueIndex);
                }

                // Associate written subresource with render pass that writes to it for quick access when needed
                for (SubresourceName subresourceName : node->WrittenSubresources())
                {
                    mWrittenSubresourceToPassMap[subresourceName] = node;
                }

                node->mGlobalExecutionIndex = globalExecutionIndex;
                node->mLocalToDependencyLevelExecutionIndex = localExecutionIndex;
                node->mLocalToQueueExecutionIndex = mQueueNodeCounters[node->ExecutionQueueIndex]++;

                mNodesInGlobalExecutionOrder[globalExecutionIndex] = node;

                dependencyLevel.mNodesPerQueue[node->ExecutionQueueIndex].push_back(node);

                // Add previous node on that queue as a dependency for sync optimization later
                if (perQueuePreviousNodes[node->ExecutionQueueIndex])
                {
                    node->mNodesToSyncWith.push_back(perQueuePreviousNodes[node->ExecutionQueueIndex]);
                }

                perQueuePreviousNodes[node->ExecutionQueueIndex] = node;

                for (Foundation::Name resourceName : node->AllResources())
                {
                    auto timelineIt = mResourceUsageTimelines.find(resourceName);
                    bool timelineExists = timelineIt != mResourceUsageTimelines.end();

                    if (timelineExists) 
                    {
                        // Update "end" 
                        timelineIt->second.second = node->GlobalExecutionIndex();
                    }
                    else {
                        // Create "start"
                        auto& timeline = mResourceUsageTimelines[resourceName];
                        timeline.first = node->GlobalExecutionIndex();
                        timeline.second = node->GlobalExecutionIndex();
                    }
                }

                // Track first RT-using node to sync BVH builds with
                if (node->UsesRayTracing && !firstRayTracingUserDetected)
                {
                    mFirstNodeThatUsesRayTracing = node;
                    firstRayTracingUserDetected = true;
                }

                localExecutionIndex++;
                globalExecutionIndex++;
            }

            // Record queue indices that are detected to read common resources
            for (auto& [subresourceName, queueIndices] : resourceReadingQueueTracker)
            {
                // If resource is read by more than one queue
                if (queueIndices.size() > 1)
                {
                    for (Node::QueueIndex queueIndex : queueIndices)
                    {
                        dependencyLevel.mQueuesInvoledInCrossQueueResourceReads.insert(queueIndex);
                        dependencyLevel.mSubresourcesReadByMultipleQueues.insert(subresourceName);
                    }
                }
            }
        }
    }

    void RenderPassGraph::CullRedundantSynchronizations()
    {
        // Initialize synchronization index sets
        for (Node& node : mPassNodes)
        {
            node.mSynchronizationIndexSet.resize(mDetectedQueueCount, Node::InvalidSynchronizationIndex);
        }

        for (DependencyLevel& dependencyLevel : mDependencyLevels)
        {
            // First pass: find closest nodes to sync with, compute initial SSIS (sufficient synchronization index set)
            for (Node* node : dependencyLevel.mNodes)
            {
                // Closest node to sync with on each queue
                std::vector<const Node*> closestNodesToSyncWith{ mDetectedQueueCount, nullptr };

                // Find closest dependencies from other queues for the current node
                for (const Node* dependencyNode : node->mNodesToSyncWith)
                {
                    const Node* closestNode = closestNodesToSyncWith[dependencyNode->ExecutionQueueIndex];

                    if (!closestNode || dependencyNode->LocalToQueueExecutionIndex() > closestNode->LocalToQueueExecutionIndex())
                    {
                        closestNodesToSyncWith[dependencyNode->ExecutionQueueIndex] = dependencyNode;
                    }
                }

                // Get rid of nodes to sync that may have had redundancies
                node->mNodesToSyncWith.clear();

                for (const Node* closestNode : closestNodesToSyncWith)
                {
                    if (!closestNode)
                    {
                        continue;
                    }

                    // Update SSIS using closest nodes' indices
                    if (closestNode->ExecutionQueueIndex != node->ExecutionQueueIndex)
                    {
                        node->mSynchronizationIndexSet[closestNode->ExecutionQueueIndex] = closestNode->LocalToQueueExecutionIndex();
                    }

                    // Store only closest nodes to sync with
                    node->mNodesToSyncWith.push_back(closestNode);
                }

                // Use node's execution index as synchronization index on its own queue
                node->mSynchronizationIndexSet[node->ExecutionQueueIndex] = node->LocalToQueueExecutionIndex();
            }

            // Second pass: cull redundant dependencies by searching for indirect synchronizations
            for (Node* node : dependencyLevel.mNodes)
            {
                // Keep track of queues we still need to sync with
                std::unordered_set<uint64_t> queueToSyncWithIndices;

                // Store nodes and queue syncs they cover
                std::vector<SyncCoverage> syncCoverageArray;

                // Final optimized list of nodes without redundant dependencies
                std::vector<const Node*> optimalNodesToSyncWith;

                for (const Node* nodeToSyncWith : node->mNodesToSyncWith)
                {
                    queueToSyncWithIndices.insert(nodeToSyncWith->ExecutionQueueIndex);
                }

                while (!queueToSyncWithIndices.empty())
                {
                    uint64_t maxNumberOfSyncsCoveredBySingleNode = 0;

                    for (auto dependencyNodeIdx = 0u; dependencyNodeIdx < node->mNodesToSyncWith.size(); ++dependencyNodeIdx)
                    {
                        const Node* dependencyNode = node->mNodesToSyncWith[dependencyNodeIdx];

                        // Take a dependency node and check how many queues we would sync with 
                        // if we would only sync with this one node. We very well may encounter a case
                        // where by synchronizing with just one node we will sync with more then one queue
                        // or even all of them through indirect synchronizations, 
                        // which will make other synchronizations previously detected for this node redundant.

                        std::vector<uint64_t> syncedQueueIndices;

                        for (uint64_t queueIndex : queueToSyncWithIndices)
                        {
                            uint64_t currentNodeDesiredSyncIndex = node->mSynchronizationIndexSet[queueIndex];
                            uint64_t dependencyNodeSyncIndex = dependencyNode->mSynchronizationIndexSet[queueIndex];

                            assert_format(currentNodeDesiredSyncIndex != Node::InvalidSynchronizationIndex,
                                "Bug! Node that wants to sync with some queue must have a valid sync index for that queue.");

                            if (queueIndex == node->ExecutionQueueIndex)
                            {
                                currentNodeDesiredSyncIndex -= 1;
                            }

                            if (dependencyNodeSyncIndex != Node::InvalidSynchronizationIndex &&
                                dependencyNodeSyncIndex >= currentNodeDesiredSyncIndex)
                            {
                                syncedQueueIndices.push_back(queueIndex);
                            }
                        }

                        syncCoverageArray.emplace_back(SyncCoverage{ dependencyNode, dependencyNodeIdx, syncedQueueIndices });
                        maxNumberOfSyncsCoveredBySingleNode = std::max(maxNumberOfSyncsCoveredBySingleNode, syncedQueueIndices.size());
                    }

                    for (const SyncCoverage& syncCoverage : syncCoverageArray)
                    {
                        auto coveredSyncCount = syncCoverage.SyncedQueueIndices.size();

                        if (coveredSyncCount >= maxNumberOfSyncsCoveredBySingleNode)
                        {
                            // Optimal list of synchronizations should not contain nodes from the same queue,
                            // because work on the same queue is synchronized automatically and implicitly
                            if (syncCoverage.NodeToSyncWith->ExecutionQueueIndex != node->ExecutionQueueIndex)
                            {
                                optimalNodesToSyncWith.push_back(syncCoverage.NodeToSyncWith);

                                // Update SSIS
                                auto& index = node->mSynchronizationIndexSet[syncCoverage.NodeToSyncWith->ExecutionQueueIndex];
                                index = std::max(index, node->mSynchronizationIndexSet[syncCoverage.NodeToSyncWith->ExecutionQueueIndex]);
                            }

                            // Remove covered queues from the list of queues we need to sync with
                            for (uint64_t syncedQueueIndex : syncCoverage.SyncedQueueIndices)
                            {
                                queueToSyncWithIndices.erase(syncedQueueIndex);
                            }
                        }
                    }

                    // Remove nodes that we synced with from the original list. Reverse iterating to avoid index invalidation.
                    for (auto syncCoverageIt = syncCoverageArray.rbegin(); syncCoverageIt != syncCoverageArray.rend(); ++syncCoverageIt)
                    {
                        node->mNodesToSyncWith.erase(node->mNodesToSyncWith.begin() + syncCoverageIt->NodeToSyncWithIndex);
                    }
                }

                // Finally, assign an optimal list of nodes to sync with to the current node
                node->mNodesToSyncWith = optimalNodesToSyncWith;
            }
        }
    }

    RenderPassGraph::Node::Node(const RenderPassMetadata& passMetadata, WriteDependencyRegistry* writeDependencyRegistry)
        : mPassMetadata{ passMetadata }, mWriteDependencyRegistry{ writeDependencyRegistry } {}

    bool RenderPassGraph::Node::operator==(const Node& that) const
    {
        return mPassMetadata.Name == that.mPassMetadata.Name;
    }

    bool RenderPassGraph::Node::operator!=(const Node& that) const
    {
        return !(*this == that);
    }

    void RenderPassGraph::Node::AddReadDependency(Foundation::Name resourceName, uint32_t firstSubresourceIndex, uint32_t lastSubresourceIndex)
    {
        for (auto i = firstSubresourceIndex; i <= lastSubresourceIndex; ++i)
        {
            SubresourceName name = ConstructSubresourceName(resourceName, i);
            mReadSubresources.insert(name);
            mReadAndWrittenSubresources.insert(name);
            mAllResources.insert(resourceName);
        }
    }

    void RenderPassGraph::Node::AddReadDependency(Foundation::Name resourceName, const SubresourceList& subresources)
    {
        if (subresources.empty())
        {
            AddReadDependency(resourceName, 1);
        }
        else
        {
            for (auto subresourceIndex : subresources)
            {
                SubresourceName name = ConstructSubresourceName(resourceName, subresourceIndex);
                mReadSubresources.insert(name);
                mReadAndWrittenSubresources.insert(name);
                mAllResources.insert(resourceName);
            }
        }
    }

    void RenderPassGraph::Node::AddReadDependency(Foundation::Name resourceName, uint32_t subresourceCount)
    {
        assert_format(subresourceCount > 0, "0 subresource count");
        AddReadDependency(resourceName, 0, subresourceCount - 1);
    }

    void RenderPassGraph::Node::AddWriteDependency(Foundation::Name resourceName, std::optional<Foundation::Name> originalResourceName, uint32_t firstSubresourceIndex, uint32_t lastSubresourceIndex)
    {
        for (auto i = firstSubresourceIndex; i <= lastSubresourceIndex; ++i)
        {
            SubresourceName name = ConstructSubresourceName(resourceName, i);
            EnsureSingleWriteDependency(name);
            mWrittenSubresources.insert(name);
            mReadAndWrittenSubresources.insert(name);
            mAllResources.insert(resourceName);

            if (originalResourceName)
            {
                SubresourceName originalSubresoruce = ConstructSubresourceName(*originalResourceName, i);
                mAliasedSubresources.insert(originalSubresoruce);
                mAllResources.insert(*originalResourceName);
            }
        }
    }

    void RenderPassGraph::Node::AddWriteDependency(Foundation::Name resourceName, std::optional<Foundation::Name> originalResourceName, const SubresourceList& subresources)
    {
        if (subresources.empty())
        {
            AddWriteDependency(resourceName, originalResourceName, 1);
        }
        else
        {
            for (auto subresourceIndex : subresources)
            {
                SubresourceName name = ConstructSubresourceName(resourceName, subresourceIndex);
                EnsureSingleWriteDependency(name);
                mWrittenSubresources.insert(name);
                mReadAndWrittenSubresources.insert(name);
                mAllResources.insert(resourceName);
            }
        }
    }

    void RenderPassGraph::Node::AddWriteDependency(Foundation::Name resourceName, std::optional<Foundation::Name> originalResourceName, uint32_t subresourceCount)
    {
        assert_format(subresourceCount > 0, "0 subresource count");
        AddWriteDependency(resourceName, originalResourceName, 0, subresourceCount - 1);
    }

    bool RenderPassGraph::Node::HasDependency(Foundation::Name resourceName, uint32_t subresourceIndex) const
    {
        return HasDependency(ConstructSubresourceName(resourceName, subresourceIndex));
    }

    bool RenderPassGraph::Node::HasDependency(SubresourceName subresourceName) const
    {
        return mReadAndWrittenSubresources.find(subresourceName) != mReadAndWrittenSubresources.end();
    }

    bool RenderPassGraph::Node::HasAnyDependencies() const
    {
        return !mReadAndWrittenSubresources.empty();
    }

    void RenderPassGraph::Node::Clear()
    {
        mReadSubresources.clear();
        mWrittenSubresources.clear();
        mReadAndWrittenSubresources.clear();
        mAllResources.clear();
        mAliasedSubresources.clear();
        mNodesToSyncWith.clear();
        mSynchronizationIndexSet.clear();
        mDependencyLevelIndex = 0;
        mSyncSignalRequired = false;
        ExecutionQueueIndex = 0;
        UsesRayTracing = false;
        mGlobalExecutionIndex = 0;
        mLocalToDependencyLevelExecutionIndex = 0;
    }

    void RenderPassGraph::Node::EnsureSingleWriteDependency(SubresourceName name)
    {
        auto [resourceName, subresourceIndex] = DecodeSubresourceName(name);

        assert_format(mWriteDependencyRegistry->find(name) == mWriteDependencyRegistry->end(),
            "Resource ", resourceName.ToString(), ", subresource ", subresourceIndex, " already has a write dependency. ",
            "Use Aliases to perform multiple writes into the same resource.");

        mWriteDependencyRegistry->insert(name);
    }

    void RenderPassGraph::DependencyLevel::AddNode(Node* node)
    {
        mNodes.push_back(node);
    }

    RenderPassGraph::Node* RenderPassGraph::DependencyLevel::RemoveNode(NodeIterator it)
    {
        Node* node = *it;
        mNodes.erase(it);
        return node;
    }

}
