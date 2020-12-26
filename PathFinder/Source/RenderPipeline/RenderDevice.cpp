#include "RenderDevice.hpp"



namespace PathFinder
{

    RenderDevice::RenderDevice(
        const HAL::Device& device, 
        Memory::PoolDescriptorAllocator* descriptorAllocator,
        Memory::PoolCommandListAllocator* commandListAllocator,
        Memory::ResourceStateTracker* resourceStateTracker,
        Memory::CopyRequestManager* copyRequestManager,
        PipelineResourceStorage* resourceStorage,
        PipelineStateManager* pipelineStateManager,
        GPUProfiler* gpuProfiler,
        const RenderPassGraph* renderPassGraph,
        const RenderSurfaceDescription& defaultRenderSurface)
        :
        mGraphicsQueue{ device },
        mComputeQueue{ device },
        mDescriptorAllocator{ descriptorAllocator },
        mCommandListAllocator{ commandListAllocator },
        mResourceStateTracker{ resourceStateTracker },
        mCopyRequestManager{ copyRequestManager },
        mResourceStorage{ resourceStorage },
        mPipelineStateManager{ pipelineStateManager },
        mGPUProfiler{ gpuProfiler },
        mRenderPassGraph{ renderPassGraph },
        mDefaultRenderSurface{ defaultRenderSurface },
        mGraphicsQueueFence{ device },
        mComputeQueueFence{ device },
        mBVHFence{ device }
    {
        mGraphicsQueue.SetDebugName("Graphics Queue");
        mComputeQueue.SetDebugName("Async Compute Queue");
    }

    RenderDevice::PassCommandLists& RenderDevice::CommandListsForNode(const RenderPassGraph::Node& node)
    {
        return mPassCommandLists[node.GlobalExecutionIndex()];
    }

    RenderDevice::PassHelpers& RenderDevice::PassHelpersForNode(const RenderPassGraph::Node& node)
    {
        return mPassHelpers[node.GlobalExecutionIndex()];
    }

    void RenderDevice::SetBackBuffer(Memory::Texture* backBuffer)
    {
        mBackBuffer = backBuffer;
    }

    const Memory::Texture* RenderDevice::BackBuffer() const
    {
        return mBackBuffer;
    }

    void RenderDevice::AllocateUploadCommandList()
    {
        mPreRenderUploadsCommandList = mCommandListAllocator->AllocateGraphicsCommandList();
        mPreRenderUploadsCommandList->Reset();
        mPreRenderUploadsCommandList->SetDebugName("Prerender Data Upload Cmd List");
        mEventTracker.StartGPUEvent("Prerender Data Upload", *mPreRenderUploadsCommandList);
    }

    void RenderDevice::AllocateRTASBuildsCommandList()
    {
        mRTASBuildsCommandList = mCommandListAllocator->AllocateComputeCommandList();
        mRTASBuildsCommandList->Reset();
        mPreRenderUploadsCommandList->SetDebugName("Ray Tracing BVH Build Cmd List");
        mEventTracker.StartGPUEvent("Ray Tracing BVH Build", *mRTASBuildsCommandList);
    }

    void RenderDevice::AllocateWorkerCommandLists()
    {
        mPassHelpers.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());

        for (const RenderPassGraph::Node* node : mRenderPassGraph->NodesInGlobalExecutionOrder())
        {
            // Zero out helpers on new frame
            mPassHelpers[node->GlobalExecutionIndex()] = PassHelpers{};
            PassHelpers& helpers = mPassHelpers[node->GlobalExecutionIndex()];
            helpers.ResourceStoragePassData = mResourceStorage->GetPerPassData(node->PassMetadata().Name);
            helpers.ResourceStoragePassData->LastSetConstantBufferDataSize = 0;
            helpers.ResourceStoragePassData->PassConstantBufferMemoryOffset = 0;
            helpers.ResourceStoragePassData->IsAllowedToAdvanceConstantBufferOffset = false;
        }

        mPassCommandLists.clear();
        mPassCommandLists.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());

        mMeasurements.clear();
        mMeasurements.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());

        // If memory layout did not change we reuse aliasing barriers from previous frame.
        // Otherwise we start from scratch.
        if (mResourceStorage->HasMemoryLayoutChange())
        {
            mPerNodeAliasingBarriers.clear();
            mPerNodeAliasingBarriers.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());
        }

        for (const RenderPassGraph::Node* node : mRenderPassGraph->NodesInGlobalExecutionOrder())
        {
            CommandListPtrVariant cmdListVariant = AllocateCommandListForQueue(node->ExecutionQueueIndex);
            GetComputeCommandListBase(cmdListVariant)->SetDebugName(node->PassMetadata().Name.ToString() + " Worker Cmd List");
            mPassCommandLists[node->GlobalExecutionIndex()].WorkCommandList = std::move(cmdListVariant);

            // UAV barriers must be ready before command list recording.
            // Process aliasing barriers while we're at it too.
            for (Foundation::Name resourceName : node->AllResources())
            {
                // Special case of back buffer
                if (resourceName == RenderPassGraph::Node::BackBufferName)
                {
                    continue;
                }

                const PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceName);
                const PipelineResourceSchedulingInfo::PassInfo* passInfo = resourceData->SchedulingInfo.GetInfoForPass(node->PassMetadata().Name);

                if (passInfo->NeedsAliasingBarrier)
                {
                    mPerNodeAliasingBarriers[node->GlobalExecutionIndex()].AddBarrier(HAL::ResourceAliasingBarrier{ nullptr, resourceData->GetGPUResource()->HALResource() });
                }

                if (passInfo->NeedsUnorderedAccessBarrier)
                {
                    mPassHelpers[node->GlobalExecutionIndex()].UAVBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ resourceData->GetGPUResource()->HALResource() });
                }
            }
        }
    }

    void RenderDevice::ExecuteRenderGraph()
    {
        // https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
        //
        // Execute fixed workloads early to save correct fence values
        ExecuteUploadCommands();
        ExecuteBVHBuildCommands();

        BatchCommandLists();
        UploadPassConstants();
        ExetuteCommandLists();
    }

    void RenderDevice::GatherMeasurements()
    {
        for (PipelineMeasurement& measurement : mMeasurements)
        {
            const GPUProfiler::Event& event = mGPUProfiler->GetCompletedEvent(measurement.ProfilerEventID);
            measurement.DurationSeconds = event.DurationSeconds;
        }
    }

    void RenderDevice::BatchCommandLists()
    {
        mCommandListBatches.clear();
        mCommandListBatches.resize(mQueueCount);

        mReroutedTransitionsCommandLists.clear();
        mReroutedTransitionsCommandLists.resize(mRenderPassGraph->DependencyLevels().size());

        mPerNodeBeginBarriers.clear();
        mPerNodeBeginBarriers.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());

        mPerNodeReadbackInfo.clear();
        mPerNodeReadbackInfo.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());

        mSubresourcesPreviousUsageInfo.clear();

        for (const RenderPassGraph::DependencyLevel& dependencyLevel : mRenderPassGraph->DependencyLevels())
        {
            mDependencyLevelTransitionBarriers.clear();
            mDependencyLevelTransitionBarriers.resize(dependencyLevel.Nodes().size());
            mDependencyLevelInterpassUAVBarriers.clear();
            mDependencyLevelInterpassUAVBarriers.resize(dependencyLevel.Nodes().size());

            GatherResourceTransitionKnowledge(dependencyLevel);
            CreateBatchesWithTransitionRerouting(dependencyLevel);
            CreateBatchesWithoutTransitionRerouting(dependencyLevel);
        }

        RecordPostWorkCommandLists();
        InsertCommandListsIntoCorrespondingBatches();
    }

    void RenderDevice::GatherResourceTransitionKnowledge(const RenderPassGraph::DependencyLevel& dependencyLevel)
    {
        mDependencyLevelQueuesThatRequireTransitionRerouting = dependencyLevel.QueuesInvoledInCrossQueueResourceReads();

        bool backBufferTransitioned = false;

        for (const RenderPassGraph::Node* node : dependencyLevel.Nodes())
        {
            // Collect resources that need to be read back after the render pass
            robin_hood::unordered_flat_set<Memory::GPUResource*> resourcesToReadback;

            auto requestTransition = [&](RenderPassGraph::SubresourceName subresourceName, bool isReadDependency)
            {
                auto [resourceName, subresourceIndex] = mRenderPassGraph->DecodeSubresourceName(subresourceName);

                // Back buffer does not participate in normal transition handling,
                // its state is changed at the beginning and at the end of a frame
                if (resourceName == RenderPassGraph::Node::BackBufferName)
                {
                    return;
                }

                PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceName);
                const PipelineResourceSchedulingInfo::PassInfo* passInfo = resourceData->SchedulingInfo.GetInfoForPass(node->PassMetadata().Name);
                
                // When dealing with reading use combined read state to make one transition instead of 
                // several separate consequential transitions when neighboring render passes require resource in different read states
                HAL::ResourceState newState = isReadDependency ?
                    resourceData->SchedulingInfo.GetSubresourceCombinedReadStates(subresourceIndex) :
                    newState = passInfo->SubresourceInfos[subresourceIndex]->RequestedState;

                std::optional<HAL::ResourceTransitionBarrier> barrier =
                    mResourceStateTracker->TransitionToStateImmediately(resourceData->GetGPUResource()->HALResource(), newState, subresourceIndex, false);

                // First pass on graphic queue needs to transition back buffer to RenderTarget state
                if (node->ExecutionQueueIndex == 0 && !backBufferTransitioned)
                {
                    std::optional<HAL::ResourceTransitionBarrier> backBufferBarrier =
                        mResourceStateTracker->TransitionToStateImmediately(mBackBuffer->HALResource(), HAL::ResourceState::RenderTarget, 0, false);

                    if (backBufferBarrier)
                    {
                        mDependencyLevelTransitionBarriers[node->LocalToDependencyLevelExecutionIndex()].push_back({ 0, *backBufferBarrier, mBackBuffer->HALResource() });
                    }

                    backBufferTransitioned = true;
                }
                
                // Render graph works with resource name aliases, so we need to track transitions for the resource using its original name,
                // otherwise we would lose transition history and place incorrect Begin/End barriers
                RenderPassGraph::SubresourceName originalSubresourceName = RenderPassGraph::ConstructSubresourceName(resourceData->ResourceName(), subresourceIndex);
                SubresourceTransitionInfo transitionInfo{ originalSubresourceName, barrier, resourceData->GetGPUResource()->HALResource() };

                // Keep track even of redundant transitions for later stage to correctly keep track of resource usage history
                mDependencyLevelTransitionBarriers[node->LocalToDependencyLevelExecutionIndex()].push_back(transitionInfo);

                // Redundant transition
                if (!barrier)
                {
                    // If barrier is redundant but new state contains UnorderedAccess, we have a case of UAV->UAV usage between render passes
                    if (EnumMaskContains(newState, HAL::ResourceState::UnorderedAccess))
                    {
                        mDependencyLevelInterpassUAVBarriers[node->LocalToDependencyLevelExecutionIndex()].AddBarrier(
                            HAL::UnorderedAccessResourceBarrier{ resourceData->GetGPUResource()->HALResource() });
                    }
                }
                else
                {
                    // Another reason to reroute resource transitions into another queue is incompatibility 
                    // of resource state transitions with receiving queue
                    if (!IsStateTransitionSupportedOnQueue(node->ExecutionQueueIndex, barrier->BeforeStates(), barrier->AfterStates()))
                    {
                        mDependencyLevelQueuesThatRequireTransitionRerouting.insert(node->ExecutionQueueIndex);
                        // If queue doesn't support the transition then we need to also involve queue that does
                        mDependencyLevelQueuesThatRequireTransitionRerouting.insert(FindQueueSupportingTransition(barrier->BeforeStates(), barrier->AfterStates()));
                    }
                }

                // Prepare list of resources that need to be read back after render pass work is completed
                if (passInfo->IsReadbackRequested)
                {
                    resourcesToReadback.insert(resourceData->GetGPUResource());
                }
            };

            for (RenderPassGraph::SubresourceName subresourceName : node->ReadSubresources())
            {
                requestTransition(subresourceName, true);
            }

            for (RenderPassGraph::SubresourceName subresourceName : node->WrittenSubresources())
            {
                requestTransition(subresourceName, false);
            }

            // Now that we know resources that need to be read back we gather 
            // and then batch transitions and copy commands
            for (Memory::GPUResource* resourceToReadback : resourcesToReadback)
            {
                resourceToReadback->RequestRead();
            }

            ResourceReadbackInfo& readbackInfo = mPerNodeReadbackInfo[node->GlobalExecutionIndex()];

            for (const Memory::CopyRequestManager::CopyRequest& request : mCopyRequestManager->ReadbackRequests())
            {
                HAL::ResourceBarrierCollection toCopyBarriers = mResourceStateTracker->TransitionToStateImmediately(request.Resource, HAL::ResourceState::CopySource);
                readbackInfo.CopyCommands.push_back(request.Command);
                readbackInfo.ToCopyStateTransitions.AddBarriers(toCopyBarriers);
            }

            mCopyRequestManager->FlushReadbackRequests();
        }
    }

    void RenderDevice::CollectNodeTransitions(const RenderPassGraph::Node* node, uint64_t currentCommandListBatchIndex, HAL::ResourceBarrierCollection& collection)
    {
        const std::vector<SubresourceTransitionInfo>& nodeTransitionBarriers = mDependencyLevelTransitionBarriers[node->LocalToDependencyLevelExecutionIndex()];
        const HAL::ResourceBarrierCollection& nodeInterpassUAVBarriers = mDependencyLevelInterpassUAVBarriers[node->LocalToDependencyLevelExecutionIndex()];
        const HAL::ResourceBarrierCollection& nodeAliasingBarriers = mPerNodeAliasingBarriers[node->GlobalExecutionIndex()];

        collection.AddBarriers(nodeAliasingBarriers);
        collection.AddBarriers(nodeInterpassUAVBarriers);

        for (const SubresourceTransitionInfo& transitionInfo : nodeTransitionBarriers)
        {
            // Transition is implicit, we need to only keep track of previous resource usage 
            // to correctly place split barriers later, if needed, graphic API transition for this pass is not required
            if (!transitionInfo.TransitionBarrier)
            {
                mSubresourcesPreviousUsageInfo[transitionInfo.SubresourceName] = { node, currentCommandListBatchIndex };
                continue;
            }

            // Transition is explicit. Look for previous transition info to see whether current explicit transition can be made
            // implicit due to automatic promotion/decay.
            auto previousTransitionInfoIt = mSubresourcesPreviousUsageInfo.find(transitionInfo.SubresourceName);
            bool foundPreviousTransition = previousTransitionInfoIt != mSubresourcesPreviousUsageInfo.end();
            bool subresourceTransitionedAtLeastOnce = foundPreviousTransition && previousTransitionInfoIt->second.CommandListBatchIndex == currentCommandListBatchIndex;

            if (!subresourceTransitionedAtLeastOnce)
            {
                bool implicitTransitionPossible = Memory::ResourceStateTracker::CanResourceBeImplicitlyTransitioned(
                    *transitionInfo.Resource, transitionInfo.TransitionBarrier->BeforeStates(), transitionInfo.TransitionBarrier->AfterStates());

                if (implicitTransitionPossible)
                {
                    continue;
                }
            }

            // When previous transition is found we can try to split the barrier
            if (foundPreviousTransition)
            {
                const SubresourcePreviousUsageInfo& previousTransitionInfo = previousTransitionInfoIt->second;

                // Split barrier is only possible when transmitting queue supports transitions for both before and after states
                bool isSplitBarrierPossible = IsStateTransitionSupportedOnQueue(
                    previousTransitionInfo.Node->ExecutionQueueIndex, transitionInfo.TransitionBarrier->BeforeStates(), transitionInfo.TransitionBarrier->AfterStates()
                );

                // There is no sense in splitting barriers between two adjacent render passes. 
                // That will only double the amount of barriers without any performance gain.
                bool currentNodeIsNextToPrevious = node->LocalToQueueExecutionIndex() - previousTransitionInfo.Node->LocalToQueueExecutionIndex() <= 1;

                if (isSplitBarrierPossible && !currentNodeIsNextToPrevious)
                {
                    auto [beginBarrier, endBarrier] = transitionInfo.TransitionBarrier->Split();
                    collection.AddBarrier(endBarrier);
                    mPerNodeBeginBarriers[previousTransitionInfo.Node->GlobalExecutionIndex()].AddBarrier(beginBarrier);
                }
                else
                {
                    collection.AddBarrier(*transitionInfo.TransitionBarrier);
                }
            }
            else
            {
                collection.AddBarrier(*transitionInfo.TransitionBarrier);
            }

            mSubresourcesPreviousUsageInfo[transitionInfo.SubresourceName] = { node, currentCommandListBatchIndex };
        }
    }

    void RenderDevice::CreateBatchesWithTransitionRerouting(const RenderPassGraph::DependencyLevel& dependencyLevel)
    {
        if (mDependencyLevelQueuesThatRequireTransitionRerouting.empty())
        {
            return;
        }

        uint64_t mostCompetentQueueIndex = FindMostCompetentQueueIndex(mDependencyLevelQueuesThatRequireTransitionRerouting);
        mReroutedTransitionsCommandLists[dependencyLevel.LevelIndex()] = AllocateCommandListForQueue(mostCompetentQueueIndex);
        CommandListPtrVariant& commandListVariant = mReroutedTransitionsCommandLists[dependencyLevel.LevelIndex()];
        HAL::ComputeCommandListBase* transitionsCommandList = GetComputeCommandListBase(commandListVariant);
        transitionsCommandList->SetDebugName(StringFormat("Dependency Level %d Rerouted Transitions Cmd List", dependencyLevel.LevelIndex()));

        std::vector<CommandListBatch>& mostCompetentQueueBatches = mCommandListBatches[mostCompetentQueueIndex];
        CommandListBatch* reroutedTransitionsBatch = &mostCompetentQueueBatches.emplace_back();

        HAL::Fence* fence = &FenceForQueueIndex(mostCompetentQueueIndex);
        reroutedTransitionsBatch->FenceToSignal = { fence, fence->IncrementExpectedValue() };
        reroutedTransitionsBatch->SignalName = StringFormat("Dependency Level %d Rerouted Transitions Signal", dependencyLevel.LevelIndex());
        reroutedTransitionsBatch->CommandLists.emplace_back(GetHALCommandListVariant(commandListVariant));
        reroutedTransitionsBatch->IsEmpty = false;

        uint64_t reroutedTransitionsBatchIndex = mostCompetentQueueBatches.size() - 1;

        HAL::ResourceBarrierCollection reroutedTransitionBarrires;

        std::vector<CommandListBatch*> dependencyLevelPerQueueBatches{ mQueueCount, nullptr };

        for (RenderPassGraph::Node::QueueIndex queueIndex : mDependencyLevelQueuesThatRequireTransitionRerouting)
        {
            // Make rerouted transitions wait for fences from involved queues,
            // but only if there is actually any work to wait for in the current frame,
            // otherwise we would establish a dependency between frames, which we don't really want
            if (queueIndex != mostCompetentQueueIndex && mCommandListBatches[queueIndex].size() > 1)
            {
                const HAL::Fence* fence = &FenceForQueueIndex(queueIndex);
                mostCompetentQueueBatches[reroutedTransitionsBatchIndex].FencesToWait.push_back({ fence, fence->ExpectedValue() });
                mostCompetentQueueBatches[reroutedTransitionsBatchIndex].EventNamesToWait.emplace_back(StringFormat("Waiting Queue %d (Rerouting Transitions)", queueIndex));
            }

            for (const RenderPassGraph::Node* node : dependencyLevel.NodesForQueue(queueIndex))
            {
                // A special case of waiting for BVH build fence, if of course pass is not executed on the same queue as BVH build
                if (mRenderPassGraph->FirstNodeThatUsesRayTracing() == node && mBVHBuildsQueueIndex != mostCompetentQueueIndex)
                {
                    mostCompetentQueueBatches[reroutedTransitionsBatchIndex].FencesToWait.push_back({ &mBVHFence, mBVHFence.ExpectedValue() });
                    mostCompetentQueueBatches[reroutedTransitionsBatchIndex].EventNamesToWait.emplace_back(StringFormat("Waiting Queue %d (BVH Build)", mBVHBuildsQueueIndex));
                }

                CommandListBatch* latestBatchAfterRerouting = dependencyLevelPerQueueBatches[node->ExecutionQueueIndex];

                if (!latestBatchAfterRerouting)
                {
                    latestBatchAfterRerouting = &mCommandListBatches[node->ExecutionQueueIndex].emplace_back();
                    dependencyLevelPerQueueBatches[node->ExecutionQueueIndex] = latestBatchAfterRerouting;

                    // Insert fence before first pass after rerouted transitions
                    if (node->ExecutionQueueIndex != mostCompetentQueueIndex)
                    {
                        auto& fenceAndValue = mostCompetentQueueBatches[reroutedTransitionsBatchIndex].FenceToSignal;
                        latestBatchAfterRerouting->FencesToWait.push_back(fenceAndValue);
                        latestBatchAfterRerouting->EventNamesToWait.push_back(StringFormat("Pass %s Waiting For Rerouted Transitions", node->PassMetadata().Name.ToString().c_str()));
                    }
                }

                // Make command lists in a batch wait for rerouted transitions
                latestBatchAfterRerouting->CommandListNames.emplace_back(node->PassMetadata().Name.ToString());
                latestBatchAfterRerouting->IsEmpty = false;

                // Associate batch index with pass command lists so we could insert them later when all split barriers are collected
                uint64_t currentCommandListBatchIndex = mCommandListBatches[queueIndex].size() - 1;
                mPassCommandLists[node->GlobalExecutionIndex()].CommandListBatchIndex = currentCommandListBatchIndex;

                CollectNodeTransitions(node, currentCommandListBatchIndex, reroutedTransitionBarrires);

                if (node->IsSyncSignalRequired())
                {
                    HAL::Fence* fence = &FenceForQueueIndex(node->ExecutionQueueIndex);
                    latestBatchAfterRerouting->FenceToSignal = { fence, fence->IncrementExpectedValue() };
                    latestBatchAfterRerouting->SignalName = StringFormat("Pass %s Signals", node->PassMetadata().Name.ToString().c_str());
                    dependencyLevelPerQueueBatches[node->ExecutionQueueIndex] = &mCommandListBatches[node->ExecutionQueueIndex].emplace_back();
                }
            }

            // Do not leave empty batches 
            if (mCommandListBatches[queueIndex].back().IsEmpty)
            {
                mCommandListBatches[queueIndex].pop_back();
            }
        }

        transitionsCommandList->Reset();
        transitionsCommandList->InsertBarriers(reroutedTransitionBarrires);
        transitionsCommandList->Close();
    }

    void RenderDevice::CreateBatchesWithoutTransitionRerouting(const RenderPassGraph::DependencyLevel& dependencyLevel)
    {
        for (auto queueIdx = 0u; queueIdx < mRenderPassGraph->DetectedQueueCount(); ++queueIdx)
        {
            if (mDependencyLevelQueuesThatRequireTransitionRerouting.find(queueIdx) != mDependencyLevelQueuesThatRequireTransitionRerouting.end())
            {
                continue;
            }

            auto& nodesForQueue = dependencyLevel.NodesForQueue(queueIdx);

            if (nodesForQueue.empty())
            {
                continue;
            }

            for (const RenderPassGraph::Node* node : nodesForQueue)
            {
                if (mCommandListBatches[queueIdx].empty())
                {
                    mCommandListBatches[queueIdx].emplace_back();
                }

                CommandListBatch* currentBatch = &mCommandListBatches[queueIdx].back();

                bool usesRT = mRenderPassGraph->FirstNodeThatUsesRayTracing() == node;

                if (!node->NodesToSyncWith().empty() || usesRT)
                {
                    if (!currentBatch->IsEmpty)
                    {
                        currentBatch = &mCommandListBatches[queueIdx].emplace_back();
                    }

                    for (const RenderPassGraph::Node* nodeToWait : node->NodesToSyncWith())
                    {
                        const HAL::Fence* fence = &FenceForQueueIndex(nodeToWait->ExecutionQueueIndex);
                        currentBatch->FencesToWait.push_back({ fence, fence->ExpectedValue() });
                        currentBatch->EventNamesToWait.push_back(StringFormat("Waiting %s Pass", nodeToWait->PassMetadata().Name.ToString().c_str()));
                    }

                    if (usesRT && node->ExecutionQueueIndex != mBVHBuildsQueueIndex)
                    {
                        currentBatch->FencesToWait.push_back({ &mBVHFence, mBVHFence.ExpectedValue() });
                        currentBatch->EventNamesToWait.emplace_back(StringFormat("Waiting Queue %d (BVH Build)", mBVHBuildsQueueIndex));
                    }
                }

                // On queues that do not require transition rerouting each node will have its own transition collection
                HAL::ResourceBarrierCollection nodeBarriers{};

                // Associate batch index with pass command lists so we could insert them later when all split barriers are collected
                uint64_t currentCommandListBatchIndex = mCommandListBatches[queueIdx].size() - 1;
                mPassCommandLists[node->GlobalExecutionIndex()].CommandListBatchIndex = currentCommandListBatchIndex;

                CollectNodeTransitions(node, currentCommandListBatchIndex, nodeBarriers);

                // Mark first command list of render pass with it's debug name
                currentBatch->CommandListNames.emplace_back(node->PassMetadata().Name.ToString());
                currentBatch->IsEmpty = false;

                if (nodeBarriers.BarrierCount() > 0)
                {
                    mPassCommandLists[node->GlobalExecutionIndex()].TransitionsCommandList = AllocateCommandListForQueue(queueIdx);
                    CommandListPtrVariant& cmdListVariant = mPassCommandLists[node->GlobalExecutionIndex()].TransitionsCommandList;
                    HAL::ComputeCommandListBase* transitionsCommandList = GetComputeCommandListBase(cmdListVariant);
                    transitionsCommandList->SetDebugName(node->PassMetadata().Name.ToString() + " Transitions Cmd List");
                    transitionsCommandList->Reset();
                    mEventTracker.StartGPUEvent(node->PassMetadata().Name.ToString() + " Pre Work (Transitions)", *transitionsCommandList);
                    transitionsCommandList->InsertBarriers(nodeBarriers);
                    mEventTracker.EndGPUEvent(*transitionsCommandList);
                    transitionsCommandList->Close();

                    // Do not mark second cmd list 
                    currentBatch->CommandListNames.emplace_back(std::nullopt);
                }

                if (node->IsSyncSignalRequired())
                {
                    HAL::Fence* fence = &FenceForQueueIndex(queueIdx);
                    currentBatch->SignalName = StringFormat("Pass %s Signals", node->PassMetadata().Name.ToString().c_str());
                    currentBatch->FenceToSignal = { fence, fence->IncrementExpectedValue() };
                    mCommandListBatches[queueIdx].emplace_back();
                }
            }

            // Do not leave empty batches 
            if (mCommandListBatches[queueIdx].back().IsEmpty)
            {
                mCommandListBatches[queueIdx].pop_back();
            }
        }
    }

    void RenderDevice::RecordPostWorkCommandLists()
    {
        auto graphicNodesCount = mRenderPassGraph->NodeCountForQueue(0);

        for (const RenderPassGraph::Node* node : mRenderPassGraph->NodesInGlobalExecutionOrder())
        {
            const HAL::ResourceBarrierCollection& beginBarriers = mPerNodeBeginBarriers[node->GlobalExecutionIndex()];
            const ResourceReadbackInfo& readbackInfo = mPerNodeReadbackInfo[node->GlobalExecutionIndex()];

            bool lastGraphicNode = node->LocalToQueueExecutionIndex() == graphicNodesCount - 1;
            bool beginBarriersExist = beginBarriers.BarrierCount() > 0;
            bool readbackRequestsExist = readbackInfo.CopyCommands.size() > 0;

            bool postWorkExists = lastGraphicNode || beginBarriersExist || readbackRequestsExist;

            if (!postWorkExists)
            {
                continue;
            }

            HAL::ResourceBarrierCollection barriers{};

            if (beginBarriersExist)
            {
                barriers.AddBarriers(beginBarriers);
            }

            // Transition back buffer after last graphic render pass
            if (lastGraphicNode)
            {
                barriers.AddBarriers(mResourceStateTracker->TransitionToStateImmediately(mBackBuffer->HALResource(), HAL::ResourceState::Present));
            }

            mPassCommandLists[node->GlobalExecutionIndex()].PostWorkCommandList = AllocateCommandListForQueue(node->ExecutionQueueIndex);
            HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[node->GlobalExecutionIndex()].PostWorkCommandList);
            cmdList->SetDebugName(node->PassMetadata().Name.ToString() + " Post-Work Cmd List");
            cmdList->Reset();

            mEventTracker.StartGPUEvent(node->PassMetadata().Name.ToString() + " Post Work", *cmdList);

            // Read back resources
            if (readbackRequestsExist)
            {
                cmdList->InsertBarriers(readbackInfo.ToCopyStateTransitions);
                
                for (const Memory::CopyRequestManager::CopyCommand& command : readbackInfo.CopyCommands)
                {
                    command(*cmdList);
                }
            }

            // Handle timestamp query readback
            if (lastGraphicNode)
            {
                mGPUProfiler->ReadbackEvents(*cmdList);
            }

            // Then apply begin and back buffer barriers
            cmdList->InsertBarriers(barriers);

            mEventTracker.EndGPUEvent(*cmdList);
            cmdList->Close();
        }
    }

    void RenderDevice::InsertCommandListsIntoCorrespondingBatches()
    {
        for (const RenderPassGraph::Node* node : mRenderPassGraph->NodesInGlobalExecutionOrder())
        {
            auto batchIndex = mPassCommandLists[node->GlobalExecutionIndex()].CommandListBatchIndex;
            CommandListBatch& batch = mCommandListBatches[node->ExecutionQueueIndex][batchIndex];

            HALCommandListPtrVariant transitionsCommandList = GetHALCommandListVariant(mPassCommandLists[node->GlobalExecutionIndex()].TransitionsCommandList);
            HALCommandListPtrVariant workCommandList = GetHALCommandListVariant(mPassCommandLists[node->GlobalExecutionIndex()].WorkCommandList);
            HALCommandListPtrVariant postWorkCommandList = GetHALCommandListVariant(mPassCommandLists[node->GlobalExecutionIndex()].PostWorkCommandList);

            if (!IsNullCommandList(transitionsCommandList)) batch.CommandLists.push_back(transitionsCommandList);
            if (!IsNullCommandList(workCommandList)) batch.CommandLists.push_back(workCommandList);
            if (!IsNullCommandList(postWorkCommandList)) batch.CommandLists.push_back(postWorkCommandList);
        }
    }

    void RenderDevice::ExecuteUploadCommands()
    {
        // Run initial upload commands
        mGraphicsQueueFence.IncrementExpectedValue();
        // Transition uploaded resources to readable states
        mPreRenderUploadsCommandList->InsertBarriers(mResourceStateTracker->ApplyRequestedTransitions());
        mGraphicsQueue.ExecuteCommandList(*mPreRenderUploadsCommandList);
        mEventTracker.EndGPUEvent(mGraphicsQueue);

        mEventTracker.StartGPUEvent("Uploads Done Signal", mGraphicsQueue);
        mGraphicsQueue.SignalFence(mGraphicsQueueFence);
        mEventTracker.EndGPUEvent(mGraphicsQueue);
    }

    void RenderDevice::ExecuteBVHBuildCommands()
    {
        // Wait for uploads, run RT AS builds
        mBVHFence.IncrementExpectedValue();

        mEventTracker.StartGPUEvent("Waiting Data Upload on Graphic Queue", mComputeQueue);
        mComputeQueue.WaitFence(mGraphicsQueueFence);
        mEventTracker.EndGPUEvent(mComputeQueue);

        mComputeQueue.ExecuteCommandList(*mRTASBuildsCommandList);
        mEventTracker.EndGPUEvent(mComputeQueue);

        mEventTracker.StartGPUEvent("BVH Builds Done Signal", mComputeQueue);
        mComputeQueue.SignalFence(mBVHFence);
        mEventTracker.EndGPUEvent(mComputeQueue);
    }

    void RenderDevice::ExetuteCommandLists()
    {
        for (auto queueIdx = 0; queueIdx < mQueueCount; ++queueIdx)
        {
            std::vector<CommandListBatch>& batches = mCommandListBatches[queueIdx];
            HAL::CommandQueue& queue = GetCommandQueue(queueIdx);

            for (auto batchIdx = 0; batchIdx < batches.size(); ++batchIdx)
            {
                CommandListBatch& batch = batches[batchIdx];

                for (auto fenceIdx = 0; fenceIdx < batch.FencesToWait.size(); ++fenceIdx)
                {
                    auto& [fence, value] = batch.FencesToWait[fenceIdx];
                    mEventTracker.StartGPUEvent(batch.EventNamesToWait[fenceIdx], queue);
                    queue.WaitFence(*fence, value);
                    mEventTracker.EndGPUEvent(queue);
                }
                
                if (RenderPassExecutionQueue{ queueIdx } == RenderPassExecutionQueue::Graphics)
                {
                    ExecuteCommandListBatch<HAL::GraphicsCommandQueue, HAL::GraphicsCommandList>(batch, queue);
                }
                else {
                    ExecuteCommandListBatch<HAL::ComputeCommandQueue, HAL::ComputeCommandList>(batch, queue);
                }

                if (const HAL::Fence* fence = batch.FenceToSignal.first)
                {
                    mEventTracker.StartGPUEvent(batch.SignalName, queue);
                    queue.SignalFence(*fence, batch.FenceToSignal.second);
                    mEventTracker.EndGPUEvent(queue);
                }
            }
        }
    }

    void RenderDevice::UploadPassConstants()
    {
        for (PassHelpers& passHelpers : mPassHelpers)
        {
            if (Memory::Buffer* passConstantBuffer = passHelpers.ResourceStoragePassData->PassConstantBuffer.get())
            {
                const auto& bytes = passHelpers.ResourceStoragePassData->PassConstantData;
                passConstantBuffer->Write(bytes.data(), 0, bytes.size());
            }
        }
    }

    bool RenderDevice::IsStateTransitionSupportedOnQueue(uint64_t queueIndex, HAL::ResourceState beforeState, HAL::ResourceState afterState) const
    {
        return IsStateTransitionSupportedOnQueue(queueIndex, beforeState) && IsStateTransitionSupportedOnQueue(queueIndex, afterState);
    }

    bool RenderDevice::IsStateTransitionSupportedOnQueue(uint64_t queueIndex, HAL::ResourceState state) const
    {
        assert_format(queueIndex < mQueueCount, "Queue index is out of bounds");

        if (queueIndex > 0)
        {
            return IsResourceStateTransitionSupportedOnComputeQueue(state);
        }

        // Graphics queue supports all states
        return true;
    }

    HAL::CommandQueue& RenderDevice::GetCommandQueue(uint64_t queueIndex)
    {
        if (queueIndex == 0)
        {
            return mGraphicsQueue;
        }
        
        return mComputeQueue;
    }

    uint64_t RenderDevice::FindMostCompetentQueueIndex(const robin_hood::unordered_flat_set<RenderPassGraph::Node::QueueIndex>& queueIndices) const
    {
        RenderPassGraph::Node::QueueIndex mostCompetentQueueIndex = std::numeric_limits<RenderPassGraph::Node::QueueIndex>::max();

        // Engine is designed to have graphics queue at index 0 and compute at indices 1 and onward hence 
        // most competent queue will always have minimum index out of the index set
        for (auto index : queueIndices)
        {
            mostCompetentQueueIndex = std::min(index, mostCompetentQueueIndex);
        }

        return mostCompetentQueueIndex;
    }

    uint64_t RenderDevice::FindQueueSupportingTransition(HAL::ResourceState beforeStates, HAL::ResourceState afterStates) const
    {
        // At the moment engine only supports 1 graphics and 1 compute queue,
        // so if we're searching for a queue that supports a transition that means
        // that compute queue can't do it and we're left with graphics only.
        // If more queues are introduced we should search for a queue, intelligently.
        return 0;
    }

    RenderDevice::CommandListPtrVariant RenderDevice::AllocateCommandListForQueue(uint64_t queueIndex) const
    {
        return queueIndex == 0 ? 
            CommandListPtrVariant{ mCommandListAllocator->AllocateGraphicsCommandList() } :
            CommandListPtrVariant{ mCommandListAllocator->AllocateComputeCommandList() };
    }

    HAL::ComputeCommandListBase* RenderDevice::GetComputeCommandListBase(CommandListPtrVariant& variant) const
    {
        HAL::ComputeCommandListBase* cmdList = nullptr;
        std::visit([&cmdList](auto&& v) { cmdList = v.get(); }, variant);
        return cmdList;
    }

    HAL::ComputeCommandListBase* RenderDevice::GetComputeCommandListBase(HALCommandListPtrVariant& variant) const
    {
        HAL::ComputeCommandListBase* cmdList = nullptr;
        std::visit([&cmdList](auto&& v) { cmdList = v; }, variant);
        return cmdList;
    }

    RenderDevice::HALCommandListPtrVariant RenderDevice::GetHALCommandListVariant(CommandListPtrVariant& variant) const
    {
        HALCommandListPtrVariant halv;
        std::visit([&halv](auto&& v) { halv = v.get(); }, variant);
        return halv;
    }

    bool RenderDevice::IsNullCommandList(HALCommandListPtrVariant& variant) const
    {
        return GetComputeCommandListBase(variant) == nullptr;
    }

    HAL::Fence& RenderDevice::FenceForQueueIndex(uint64_t index)
    {
        assert_format(index < 2, "There are currently only 2 queues and 2 respective fences");
        return index == 0 ? mGraphicsQueueFence : mComputeQueueFence;
    }

}
