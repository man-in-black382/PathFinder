#include "RenderPassGraph.hpp"

#include "RenderPass.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

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
        assert_format(timelineIt != mResourceUsageTimelines.end(), "Resource timeline doesn't exist");
        return timelineIt->second;
    }

    void RenderPassGraph::AddPass(const RenderPassMetadata& passMetadata)
    {
        EnsureRenderPassUniqueness(passMetadata.Name);
        mPassNodes.emplace_back(Node{ passMetadata, &mGlobalWriteDependencyRegistry });
        mPassNodes.back().mIndexInUnorderedList = mPassNodes.size() - 1;
    }

    void RenderPassGraph::RemovePass(NodeListIterator it)
    {
        mPassNodes.erase(it);
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

    void RenderPassGraph::IterateNodesInExecutionOrder(const std::function<void(const RenderPassGraph::Node&)>& iterator) const
    {
        for (const DependencyLevel& dependencyLevel : mDependencyLevels)
        {
            for (const Node* node : dependencyLevel.Nodes())
            {
                iterator(*node);
            }
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
            std::vector<uint64_t>& adjacentNodeIndices = mAdjacencyLists[nodeIdx];

            for (auto otherNodeIdx = 0; otherNodeIdx < mPassNodes.size(); ++otherNodeIdx)
            {
                // Do not check dependencies on itself
                if (nodeIdx == otherNodeIdx) continue;

                Node& otherNode = mPassNodes[otherNodeIdx];

                for (SubresourceName otherNodeReadResource : otherNode.ReadSubresources())
                {
                    // If other node reads a subresource written by the current node, then it depends on current node and is an adjacent dependency
                    bool otherNodeDependsOnCurrentNode = node.WrittenSubresources().find(otherNodeReadResource) != node.WrittenSubresources().end();

                    if (otherNodeDependsOnCurrentNode)
                    {
                        adjacentNodeIndices.push_back(otherNodeIdx);

                        // Signal only for cross-queue dependencies
                        if (node.ExecutionQueueIndex != otherNode.ExecutionQueueIndex)
                        {
                            otherNode.mSyncSignalRequired = true;
                        }

                        // Adding dependency even from the same queue is convenient 
                        // for dependency optimization pass later.
                        node.mNodesToSyncWith.push_back(&otherNode);

                        break;
                    }
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
            if (!visitedNodes[nodeIndex])
            {
                DepthFirstSearch(nodeIndex, visitedNodes, onStackNodes, isCyclic);
                assert_format(!isCyclic, "Detected cyclic dependency in pass: ", mPassNodes[nodeIndex].PassMetadata().Name.ToString());
            }
        }

        std::reverse(mTopologicallySortedNodes.begin(), mTopologicallySortedNodes.end());

        for (const Node* node : mTopologicallySortedNodes)
        {
            OutputDebugString(StringFormat("%s \n", node->PassMetadata().Name.ToString().c_str()).c_str());
        }
    }

    void RenderPassGraph::BuildDependencyLevels()
    {
        std::vector<int64_t> longestDistances(mTopologicallySortedNodes.size(), 0);

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
        // Iterate through unordered nodes because adjacency lists contain indices to 
        // initial unordered list of nodes and longest distances also correspond to them.
        for (auto nodeIndex = 0; nodeIndex < mPassNodes.size(); ++nodeIndex)
        {
            Node& node = mPassNodes[nodeIndex];
            uint64_t levelIndex = longestDistances[nodeIndex];
            DependencyLevel& dependencyLevel = mDependencyLevels[levelIndex];
            dependencyLevel.mLevelIndex = levelIndex;
            dependencyLevel.AddNode(&node);
            node.mDependencyLevelIndex = levelIndex;
            mDetectedQueueCount = std::max(mDetectedQueueCount, node.ExecutionQueueIndex + 1);
        }
    }

    void RenderPassGraph::FinalizeDependencyLevels()
    {
        uint64_t globalExecutionIndex = 0;
        bool firstRayTracingUserDetected = false;

        mNodesInGlobalExecutionOrder.resize(mTopologicallySortedNodes.size(), nullptr);
        
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

                node->mGlobalExecutionIndex = globalExecutionIndex;
                node->mLocalToDependencyLevelExecutionIndex = localExecutionIndex;
                node->mLocalToQueueExecutionIndex = mQueueNodeCounters[node->ExecutionQueueIndex]++;

                mNodesInGlobalExecutionOrder[globalExecutionIndex] = node;

                dependencyLevel.mNodesPerQueue[node->ExecutionQueueIndex].push_back(node);

                for (SubresourceName subresourceName : node->AllSubresources())
                {
                    // Timeline for resource is determined as an enclosing range of all of its subresource timelines
                    auto [resourceName, subresourceIndex] = DecodeSubresourceName(subresourceName);

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
            node.mSynchronizationIndexSet.resize(mDetectedQueueCount, 0);
        }

        std::vector<std::vector<const Node*>> nodesPerQueue{ mDetectedQueueCount };

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
                    uint64_t closestDependencyNodeIndexInQueue = dependencyNode->LocalToQueueExecutionIndex();

                    if (const Node* closestNode = closestNodesToSyncWith[node->ExecutionQueueIndex])
                    {
                        closestDependencyNodeIndexInQueue = std::max(closestNode->LocalToQueueExecutionIndex(), closestDependencyNodeIndexInQueue);
                    }

                    closestNodesToSyncWith[node->ExecutionQueueIndex] = dependencyNode;
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

                nodesPerQueue[node->ExecutionQueueIndex].push_back(node);
            }

            // Second pass: cull redundant dependencies by searching for indirect synchronizations
            for (Node* node : dependencyLevel.mNodes)
            {
                // Keep track of queues we still need to sync with
                std::unordered_set<uint64_t> queueToSyncWithIndices;

                // Store nodes and queue syncs they cover
                std::vector<std::pair<const Node*, std::vector<uint64_t>>> syncCoverage;

                // Final optimized list of nodes without redundant dependencies
                std::vector<const Node*> optimalNodesToSyncWith;

                for (const Node* nodeToSyncWith : node->mNodesToSyncWith)
                {
                    // Having node on the same queue as a dependency to sync with is useful 
                    // to detect indirect syncs performed through nodes on the same queue,
                    // but we don't actually want the same queue for direct synchronization,
                    // so skip it here.

                    if (nodeToSyncWith->ExecutionQueueIndex == node->ExecutionQueueIndex)
                    {
                        continue;
                    }

                    queueToSyncWithIndices.insert(nodeToSyncWith->ExecutionQueueIndex);
                }

                while (!queueToSyncWithIndices.empty())
                {
                    uint64_t maxNumberOfSyncsCoveredBySingleNode = 0;

                    for (const Node* dependencyNode : node->mNodesToSyncWith)
                    {
                        // Take a dependency node and check how many queues we would sync with 
                        // if we would only sync with this one node. We very well may encounter a case
                        // where by synchronizing with just one node we will sync with more then one queue
                        // or even all of them through indirect synchronizations, 
                        // which will make other synchronizations previously detected for this node redundant.

                        uint64_t numberOfSyncsCoveredByDependency = 0;

                        for (uint64_t queueIndex : queueToSyncWithIndices)
                        {
                            uint64_t currentNodeDesiredSyncIndex = node->mSynchronizationIndexSet[queueIndex];
                            uint64_t dependencyNodeSyncIndex = dependencyNode->mSynchronizationIndexSet[queueIndex];

                            if (dependencyNodeSyncIndex >= currentNodeDesiredSyncIndex)
                            {
                                ++numberOfSyncsCoveredByDependency;
                            }
                        }

                        syncCoverage.emplace_back(dependencyNode, numberOfSyncsCoveredByDependency);
                        maxNumberOfSyncsCoveredBySingleNode = std::max(maxNumberOfSyncsCoveredBySingleNode, numberOfSyncsCoveredByDependency);
                    }

                    for (auto& [node, syncedQueues] : syncCoverage)
                    {
                        auto coveredSyncCount = syncedQueues.size();

                        if (coveredSyncCount >= maxNumberOfSyncsCoveredBySingleNode)
                        {
                            optimalNodesToSyncWith.push_back(node);

                            // Remove covered queues from the list of queues we need to sync with
                            for (uint64_t syncedQueueIndex : syncedQueues)
                            {
                                queueToSyncWithIndices.erase(syncedQueueIndex);
                            }
                        }
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
            SubresourceName name = CreateSubresourceName(resourceName, i);
            mReadSubresources.insert(name);
            mAllSubresources.insert(name);
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
                SubresourceName name = CreateSubresourceName(resourceName, subresourceIndex);
                mReadSubresources.insert(name);
                mAllSubresources.insert(name);
                mAllResources.insert(resourceName);
            }
        }
    }

    void RenderPassGraph::Node::AddReadDependency(Foundation::Name resourceName, uint32_t subresourceCount)
    {
        assert_format(subresourceCount > 0, "0 subresource count");
        AddReadDependency(resourceName, 0, subresourceCount - 1);
    }

    void RenderPassGraph::Node::AddWriteDependency(Foundation::Name resourceName, uint32_t firstSubresourceIndex, uint32_t lastSubresourceIndex)
    {
        for (auto i = firstSubresourceIndex; i <= lastSubresourceIndex; ++i)
        {
            SubresourceName name = CreateSubresourceName(resourceName, i);
            EnsureSingleWriteDependency(name);
            mWrittenSubresources.insert(name);
            mAllSubresources.insert(name);
            mAllResources.insert(resourceName);
        }
    }

    void RenderPassGraph::Node::AddWriteDependency(Foundation::Name resourceName, const SubresourceList& subresources)
    {
        if (subresources.empty())
        {
            AddWriteDependency(resourceName, 1);
        }
        else
        {
            for (auto subresourceIndex : subresources)
            {
                SubresourceName name = CreateSubresourceName(resourceName, subresourceIndex);
                EnsureSingleWriteDependency(name);
                mWrittenSubresources.insert(name);
                mAllSubresources.insert(name);
                mAllResources.insert(resourceName);
            }
        }
    }

    void RenderPassGraph::Node::AddWriteDependency(Foundation::Name resourceName, uint32_t subresourceCount)
    {
        assert_format(subresourceCount > 0, "0 subresource count");
        AddWriteDependency(resourceName, 0, subresourceCount - 1);
    }

    bool RenderPassGraph::Node::HasDependency(Foundation::Name resourceName, uint32_t subresourceIndex) const
    {
        return mAllSubresources.find(CreateSubresourceName(resourceName, subresourceIndex)) != mAllSubresources.end();
    }

    void RenderPassGraph::Node::Clear()
    {
        mReadSubresources.clear();
        mWrittenSubresources.clear();
        mAllSubresources.clear();
        mAllResources.clear();
        mNodesToSyncWith.clear();
        mSynchronizationIndexSet.clear();
        mDependencyLevelIndex = 0;
        mSyncSignalRequired = false;
        ExecutionQueueIndex = 0;
        UsesRayTracing = false;
        mGlobalExecutionIndex = 0;
        mLocalToDependencyLevelExecutionIndex = 0;
    }

    RenderPassGraph::SubresourceName RenderPassGraph::Node::CreateSubresourceName(Foundation::Name resourceName, uint32_t subresourceIndex) const
    {
        SubresourceName name = resourceName.ToId();
        name <<= 32;
        name |= subresourceIndex;
        return name;
    }

    void RenderPassGraph::Node::EnsureSingleWriteDependency(SubresourceName name)
    {
        auto [resourceName, subresourceIndex] = DecodeSubresourceName(name);

        assert_format(mWriteDependencyRegistry->find(name) == mWriteDependencyRegistry->end(),
            "Resource ", resourceName.ToString(), ", subresource ", subresourceIndex, " already has a write dependency. ",
            "Consider refactoring render passes to write to each sub resource of any resource only once in a frame.");

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
