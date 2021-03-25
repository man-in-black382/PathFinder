#include "RenderDevice.hpp"

#include <Foundation/Visitor.hpp>

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
        const RenderSurfaceDescription& defaultRenderSurface,
        const PipelineSettings* settings)
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
        mBVHFence{ device },
        mFrameBlueprint{ renderPassGraph, mBVHBuildsQueueIndex, &mBVHFence, {&mGraphicsQueueFence, &mComputeQueueFence} },
        mPipelinesSettings{ settings }
    {
        mFrameMeasurement.Name = "Total Frame Time";
        mGraphicsQueue.SetDebugName("Graphics Queue");
        mComputeQueue.SetDebugName("Async Compute Queue");
    }

    RenderDevice::PassCommandLists& RenderDevice::CommandListsForPass(const RenderPassGraph::Node& node)
    {
        return mFrameBlueprint.GetRenderPassEvent(node).CommandLists;
    }

    RenderDevice::PassHelpers& RenderDevice::PassHelpersForPass(const RenderPassGraph::Node& node)
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

    void RenderDevice::PrepareForGraphExecution()
    {
        mFrameBlueprint.Build();

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

        mPassWorkMeasurements.clear();
        mPassWorkMeasurements.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());

        mPassBarrierMeasurements.clear();

        mGPUProfiler->SetPerQueueTimestampFrequencies(GetQueueTimestampFrequencies());

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
            mFrameBlueprint.GetRenderPassEvent(*node).CommandLists.WorkCommandList = std::move(cmdListVariant);

            // UAV barriers must be ready before command list recording.
            // Process aliasing barriers while we're at it too.
            for (Foundation::Name resourceName : node->AllResources())
            {
                // Special case of back buffer
                if (resourceName == RenderPassGraph::Node::BackBufferName)
                    continue;

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

        RecordNonWorkerCommandLists();
        TraverseAndExecuteFrameBlueprint();
    }

    void RenderDevice::GatherMeasurements()
    {
        for (PipelineMeasurement& measurement : mPassWorkMeasurements)
        {
            const GPUProfiler::Event& event = mGPUProfiler->GetCompletedEvent(measurement.ProfilerEventID);
            measurement.DurationSeconds = event.DurationSeconds;
        }

        for (PipelineMeasurement& measurement : mPassBarrierMeasurements)
        {
            const GPUProfiler::Event& event = mGPUProfiler->GetCompletedEvent(measurement.ProfilerEventID);
            measurement.DurationSeconds = event.DurationSeconds;
        }

        mFrameMeasurement.DurationSeconds = mGPUProfiler->GetCompletedEvent(mFrameMeasurement.ProfilerEventID).DurationSeconds;
    }

    void RenderDevice::RecordNonWorkerCommandLists()
    {
        mPerNodeBeginBarriers.clear();
        mPerNodeBeginBarriers.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());

        mPerNodeReadbackInfo.clear();
        mPerNodeReadbackInfo.resize(mRenderPassGraph->NodesInGlobalExecutionOrder().size());

        mSubresourcesPreviousUsageInfo.clear();

        for (const RenderPassGraph::DependencyLevel& dependencyLevel : mRenderPassGraph->DependencyLevels())
        {
            mDependencyLevelStandardTransitions.clear();
    
            mDependencyLevelStandardTransitions.resize(dependencyLevel.Nodes().size());

            mDependencyLevelTransitionsToReroute.clear();

            mDependencyLevelInterpassUAVBarriers.clear();
            mDependencyLevelInterpassUAVBarriers.resize(dependencyLevel.Nodes().size());

            mDependencyLevelQueuesThatRequireTransitionRerouting.clear();

            GatherResourceTransitionKnowledge(dependencyLevel);
            RecordResourceTransitions(dependencyLevel);
        }

        RecordPostWorkCommandLists();
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
                        mDependencyLevelStandardTransitions[node->LocalToDependencyLevelExecutionIndex()].push_back({ 0, *backBufferBarrier, mBackBuffer->HALResource() });
                    }

                    backBufferTransitioned = true; 
                }
                
                // Render graph works with resource name aliases, so we need to track transitions for the resource using its original name,
                // otherwise we would lose transition history and place incorrect Begin/End barriers
                RenderPassGraph::SubresourceName originalSubresourceName = RenderPassGraph::ConstructSubresourceName(resourceData->ResourceName(), subresourceIndex);
                SubresourceTransitionInfo transitionInfo{ originalSubresourceName, barrier, resourceData->GetGPUResource()->HALResource() };

                bool doesTransitionNeedRerouting = false;

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
                    // Reasons to reroute resource transitions into another queue are:
                    // 1) Incompatibility of resource state transitions with receiving queue.
                    // 2) Resource is read by multiple queues and explicit transition is required.

                    doesTransitionNeedRerouting =
                        !IsStateTransitionSupportedOnQueue(node->ExecutionQueueIndex, barrier->BeforeStates(), barrier->AfterStates()) ||
                        dependencyLevel.SubresourcesReadByMultipleQueues().contains(subresourceName);

                    if (doesTransitionNeedRerouting)
                    {
                        mDependencyLevelQueuesThatRequireTransitionRerouting.insert(node->ExecutionQueueIndex);
                        // If queue doesn't support the transition then we need to also involve queue that does
                        mDependencyLevelQueuesThatRequireTransitionRerouting.insert(FindQueueSupportingTransition(barrier->BeforeStates(), barrier->AfterStates()));
                    }
                }

                // Keep track even of redundant transitions for later stage to correctly keep track of resource usage history
                doesTransitionNeedRerouting ?
                    mDependencyLevelTransitionsToReroute.push_back(transitionInfo) :
                    mDependencyLevelStandardTransitions[node->LocalToDependencyLevelExecutionIndex()].push_back(transitionInfo);

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

    void RenderDevice::AllocateAndRecordPreWorkCommandList(const RenderPassGraph::Node& node, const HAL::ResourceBarrierCollection& barriers, const std::string& cmdListName)
    {
        if (barriers.BarrierCount() == 0)
            return;

        PassCommandLists& commandLists = CommandListsForPass(node);

        commandLists.PreWorkCommandList = AllocateCommandListForQueue(node.ExecutionQueueIndex);
        HAL::ComputeCommandListBase* transitionsCommandList = GetComputeCommandListBase(commandLists.PreWorkCommandList);
        transitionsCommandList->SetDebugName(node.PassMetadata().Name.ToString() + " " + cmdListName + " Cmd List");
        transitionsCommandList->Reset();
        
        mEventTracker.StartGPUEvent(node.PassMetadata().Name.ToString() + " " + cmdListName, *transitionsCommandList);
        GPUProfiler::EventID profilerEventID = mGPUProfiler->RecordEventStart(*transitionsCommandList, node.ExecutionQueueIndex);
        mPassBarrierMeasurements.emplace_back(PipelineMeasurement{ "", profilerEventID, 0 });

        transitionsCommandList->InsertBarriers(barriers);

        mGPUProfiler->RecordEventEnd(*transitionsCommandList, profilerEventID);
        mEventTracker.EndGPUEvent(*transitionsCommandList);
        
        transitionsCommandList->Close();
    }

    void RenderDevice::AllocateAndRecordReroutedTransitionsCommandList(std::optional<uint64_t> reroutingDependencyLevelIndex, uint64_t currentDependencyLevelIndex, const HAL::ResourceBarrierCollection& barriers)
    {
        uint64_t mostCompetentQueueIndex = FindMostCompetentQueueIndex(mDependencyLevelQueuesThatRequireTransitionRerouting);

        std::vector<uint64_t> queuesToSync;

        for (uint64_t queueIndex : mDependencyLevelQueuesThatRequireTransitionRerouting)
        {
            if (queueIndex != mostCompetentQueueIndex)
                queuesToSync.push_back(queueIndex);
        }

        FrameBlueprint::ReroutedTransitionsEvent& transitionsEvent = mFrameBlueprint.InsertReroutedTransitionsEvent(
            reroutingDependencyLevelIndex, currentDependencyLevelIndex, mostCompetentQueueIndex, queuesToSync
        );

        transitionsEvent.CommandList = AllocateCommandListForQueue(mostCompetentQueueIndex);

        HAL::ComputeCommandListBase* transitionsCommandList = GetComputeCommandListBase(transitionsEvent.CommandList);
        transitionsCommandList->SetDebugName(StringFormat("Rerouted Transitions for Dependency Level %d Cmd List", currentDependencyLevelIndex));
        transitionsCommandList->Reset();
        
        mEventTracker.StartGPUEvent(StringFormat("Rerouting Transitions for Dependency Level %d", currentDependencyLevelIndex), *transitionsCommandList);
        GPUProfiler::EventID profilerEventID = mGPUProfiler->RecordEventStart(*transitionsCommandList, mostCompetentQueueIndex);
        mPassBarrierMeasurements.emplace_back(PipelineMeasurement{ "", profilerEventID, 0 });

        transitionsCommandList->InsertBarriers(barriers);

        mGPUProfiler->RecordEventEnd(*transitionsCommandList, profilerEventID);
        mEventTracker.EndGPUEvent(*transitionsCommandList);

        transitionsCommandList->Close();
    }

    void RenderDevice::CollectNodeStandardTransitions(const RenderPassGraph::Node* node, uint64_t currentCommandListBatchIndex, HAL::ResourceBarrierCollection& collection)
    {
        const std::vector<SubresourceTransitionInfo>& nodeTransitionBarriers = mDependencyLevelStandardTransitions[node->LocalToDependencyLevelExecutionIndex()];

        for (const SubresourceTransitionInfo& transitionInfo : nodeTransitionBarriers)
        {
            // Transition is implicit, we need to only keep track of previous resource usage 
            // to correctly place split barriers later, if needed, graphic API transition for this pass is not required
            if (!transitionInfo.TransitionBarrier)
            {
                mSubresourcesPreviousUsageInfo[transitionInfo.SubresourceName] = { SubresourcePreviousUsageInfo::ResourceUser{node}, currentCommandListBatchIndex };
                continue;
            }

            // Transition is explicit. Look for previous transition info to see whether current explicit transition can be made
            // implicit due to automatic promotion/decay.
            auto previousTransitionInfoIt = mSubresourcesPreviousUsageInfo.find(transitionInfo.SubresourceName);
            bool foundPreviousTransition = previousTransitionInfoIt != mSubresourcesPreviousUsageInfo.end();
            bool subresourceTransitionedAtLeastOnce = foundPreviousTransition && previousTransitionInfoIt->second.EstimatedCommandListBatchIndex == currentCommandListBatchIndex;

            if (!subresourceTransitionedAtLeastOnce)
            {
                bool implicitTransitionPossible = Memory::ResourceStateTracker::CanResourceBeImplicitlyTransitioned(
                    *transitionInfo.Resource, transitionInfo.TransitionBarrier->BeforeStates(), transitionInfo.TransitionBarrier->AfterStates());

                if (implicitTransitionPossible)
                    continue;
            }

            // When previous transition is found and(!) previous usage was on render pass and not in rerouted transitions, we can try to split the barrier
            if (foundPreviousTransition && std::holds_alternative<const RenderPassGraph::Node*>(previousTransitionInfoIt->second.User))
            {
                const RenderPassGraph::Node* previousTransitionNode = std::get<const RenderPassGraph::Node*>(previousTransitionInfoIt->second.User);

                // Split barrier is only possible when transmitting queue supports transitions for both before and after states
                bool isSplitBarrierPossible = IsStateTransitionSupportedOnQueue(
                    previousTransitionNode->ExecutionQueueIndex, transitionInfo.TransitionBarrier->BeforeStates(), transitionInfo.TransitionBarrier->AfterStates()
                );

                // Respect settings
                isSplitBarrierPossible = isSplitBarrierPossible && mPipelinesSettings->IsSplitBarriersEnabled;

                // There is no sense in splitting barriers between two adjacent render passes. 
                // That will only double the amount of barriers without any performance gain.
                bool currentNodeIsNextToPrevious = node->LocalToQueueExecutionIndex() - previousTransitionNode->LocalToQueueExecutionIndex() <= 1;

                if (isSplitBarrierPossible && !currentNodeIsNextToPrevious)
                {
                    auto [beginBarrier, endBarrier] = transitionInfo.TransitionBarrier->Split();
                    collection.AddBarrier(endBarrier);
                    mPerNodeBeginBarriers[previousTransitionNode->GlobalExecutionIndex()].AddBarrier(beginBarrier);
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

            mSubresourcesPreviousUsageInfo[transitionInfo.SubresourceName] = { SubresourcePreviousUsageInfo::ResourceUser{node}, currentCommandListBatchIndex };
        }
    }

    void RenderDevice::CollectNodeTransitionsToReroute(std::optional<uint64_t>& reroutingDependencyLevelIndex, HAL::ResourceBarrierCollection& collection, const RenderPassGraph::DependencyLevel& currentDL)
    {
        // We want to execute rerouted transitions as early in the frame as we can, 
        // but only after closest dependency level that uses any of the resources,
        // transitions for which need to be rerouted. 

        for (const SubresourceTransitionInfo& transitionInfo : mDependencyLevelTransitionsToReroute)
        {
            collection.AddBarrier(*transitionInfo.TransitionBarrier);

            auto previousUsageInfoIt = mSubresourcesPreviousUsageInfo.find(transitionInfo.SubresourceName);
            bool previousUsageExists = previousUsageInfoIt != mSubresourcesPreviousUsageInfo.end();

            if (previousUsageExists)
            {
                uint64_t dlIndex = 0;
                
                // Previous usage was either on a render pass OR on a rerouted transitions event
                std::visit(Foundation::MakeVisitor(
                    [&dlIndex](const RenderPassGraph::Node* previousUsageNode) { dlIndex = previousUsageNode->DependencyLevelIndex(); },
                    [&dlIndex](SubresourcePreviousUsageInfo::DependencyLevelIndex previousUsageDLIndex) { dlIndex = previousUsageDLIndex; }),
                    previousUsageInfoIt->second.User);

                if (!reroutingDependencyLevelIndex || dlIndex > (*reroutingDependencyLevelIndex))
                    reroutingDependencyLevelIndex = dlIndex;
            }

            // We update index of dependency level the resource was last used in
            mSubresourcesPreviousUsageInfo[transitionInfo.SubresourceName] = { SubresourcePreviousUsageInfo::ResourceUser{ currentDL.LevelIndex() } };
        }
    }

    void RenderDevice::CollectNodeUAVAndAliasingBarriers(const RenderPassGraph::Node& node, HAL::ResourceBarrierCollection& collection)
    {
        const HAL::ResourceBarrierCollection& nodeInterpassUAVBarriers = mDependencyLevelInterpassUAVBarriers[node.LocalToDependencyLevelExecutionIndex()];
        const HAL::ResourceBarrierCollection& nodeAliasingBarriers = mPerNodeAliasingBarriers[node.GlobalExecutionIndex()];

        collection.AddBarriers(nodeAliasingBarriers);
        collection.AddBarriers(nodeInterpassUAVBarriers);
    }

    void RenderDevice::RecordResourceTransitions(const RenderPassGraph::DependencyLevel& dependencyLevel)
    {
        bool willRerouteTransitions = !mDependencyLevelQueuesThatRequireTransitionRerouting.empty();
        
        std::optional<uint64_t> reroutingDependencyLevelIndex = std::nullopt;

        for (const RenderPassGraph::Node* node : dependencyLevel.Nodes())
        {
            HAL::ResourceBarrierCollection nodeBarriers{};

            uint64_t currentCommandListBatchIndex = mFrameBlueprint.GetRenderPassEvent(*node).EstimatedBatchIndex;
            
            CollectNodeStandardTransitions(node, currentCommandListBatchIndex, nodeBarriers);
            CollectNodeUAVAndAliasingBarriers(*node, nodeBarriers);
            AllocateAndRecordPreWorkCommandList(*node, nodeBarriers, "Pre Work (Transitions | UAV | Aliasing)");
        }

        if (willRerouteTransitions)
        {
            HAL::ResourceBarrierCollection transitionsToReroute{};
            CollectNodeTransitionsToReroute(reroutingDependencyLevelIndex, transitionsToReroute, dependencyLevel);
            AllocateAndRecordReroutedTransitionsCommandList(reroutingDependencyLevelIndex, dependencyLevel.LevelIndex(), transitionsToReroute);
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

            PassCommandLists& cmdLists = CommandListsForPass(*node);
            cmdLists.PostWorkCommandList = AllocateCommandListForQueue(node->ExecutionQueueIndex);

            HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(cmdLists.PostWorkCommandList);
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

            GPUProfiler::EventID profilerEventID = mGPUProfiler->RecordEventStart(*cmdList, node->ExecutionQueueIndex);
            mPassBarrierMeasurements.emplace_back(PipelineMeasurement{ "", profilerEventID, 0 });

            // Then apply begin and back buffer barriers
            cmdList->InsertBarriers(barriers);

            mGPUProfiler->RecordEventEnd(*cmdList, profilerEventID);
            mEventTracker.EndGPUEvent(*cmdList);
            cmdList->Close();
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

    void RenderDevice::SyncNonGraphicQueuesIntoGraphicQueue()
    {
        // For queues that are "dangling" i.e. results of which are not used by graphics queue for some reason
        // we manually sync them with graphics so that all frame work on all queues ends before call to Present()
        HAL::CommandQueue& graphicQueue = GetCommandQueue(0);

        for (auto queueIdx = 1; queueIdx < mQueueCount; ++queueIdx)
        {
            if (queueIdx >= mRenderPassGraph->DetectedQueueCount())
                break;

            if (!mRenderPassGraph->NodeCountForQueue(queueIdx))
                continue;

            const RenderPassGraph::Node* lastNode = mRenderPassGraph->NodesForQueue(queueIdx).back();

            HAL::Fence& fence = FenceForQueueIndex(queueIdx);
            fence.IncrementExpectedValue();
            GetCommandQueue(queueIdx).SignalFence(fence);
            graphicQueue.WaitFence(fence);
        }
    }

    void RenderDevice::TraverseAndExecuteFrameBlueprint()
    {
        mFrameBlueprint.PropagateFenceUpdates();

        std::vector<std::vector<CommandListPtrVariant>> commandLists;
        commandLists.resize(mRenderPassGraph->DetectedQueueCount());

        // Measure start of the frame
        Memory::PoolCommandListAllocator::GraphicsCommandListPtr frameMeasurementsStartCmdList = mCommandListAllocator->AllocateGraphicsCommandList();
        auto graphicQueueIndex = std::underlying_type_t<RenderPassExecutionQueue>(RenderPassExecutionQueue::Graphics);
        frameMeasurementsStartCmdList->Reset();
        mFrameMeasurement.ProfilerEventID = mGPUProfiler->RecordEventStart(*frameMeasurementsStartCmdList, graphicQueueIndex);
        frameMeasurementsStartCmdList->Close();
        commandLists[graphicQueueIndex].push_back(std::move(frameMeasurementsStartCmdList));

        auto flushBatch = [this, &commandLists](HAL::CommandQueue& queue, uint64_t queueIndex)
        {
            if (!commandLists.empty())
            {
                HAL::CommandQueue& queue = GetCommandQueue(queueIndex);

                if (RenderPassExecutionQueue{ queueIndex } == RenderPassExecutionQueue::Graphics)
                    ExecuteCommandListBatch<HAL::GraphicsCommandQueue, HAL::GraphicsCommandList>(commandLists[queueIndex], queue);
                else
                    ExecuteCommandListBatch<HAL::ComputeCommandQueue, HAL::ComputeCommandList>(commandLists[queueIndex], queue);
                    
                commandLists[queueIndex].clear();
            }
        };

        auto signal = [this](const FrameBlueprint::Signal& signal, HAL::CommandQueue& queue)
        {
            mEventTracker.StartGPUEvent(signal.SignalName, queue);
            queue.SignalFence(*signal.Fence, signal.FenceValue);
            mEventTracker.EndGPUEvent(queue);
        };

        auto wait = [this](const FrameBlueprint::Wait& wait, HAL::CommandQueue& queue)
        {
            for (auto signalIdx = 0; signalIdx < wait.SignalsToWait.size(); ++signalIdx)
            {
                const FrameBlueprint::Signal* signalToWait = wait.SignalsToWait[signalIdx];
                const std::string& signalName = wait.EventNamesToWait[signalIdx];

                mEventTracker.StartGPUEvent(signalName, queue);
                queue.WaitFence(*signalToWait->Fence, signalToWait->FenceValue);
                mEventTracker.EndGPUEvent(queue);
            }
        };

        mFrameBlueprint.Traverse([&](uint64_t queueIndex, FrameBlueprint::Event& event)
        {
            HAL::CommandQueue& queue = GetCommandQueue(queueIndex);

            std::visit(Foundation::MakeVisitor(
            [&](FrameBlueprint::RenderPassEvent& e)
            {
                if (e.WaitEvent)
                {
                    flushBatch(queue, queueIndex);
                    wait(*e.WaitEvent, queue);
                }

                if (!IsNullCommandList(e.CommandLists.PreWorkCommandList)) 
                    commandLists[queueIndex].push_back(std::move(e.CommandLists.PreWorkCommandList));

                if (!IsNullCommandList(e.CommandLists.WorkCommandList)) 
                    commandLists[queueIndex].push_back(std::move(e.CommandLists.WorkCommandList));

                if (!IsNullCommandList(e.CommandLists.PostWorkCommandList))
                    commandLists[queueIndex].push_back(std::move(e.CommandLists.PostWorkCommandList));

                if (e.SignalEvent)
                {
                    flushBatch(queue, queueIndex);
                    signal(*e.SignalEvent, queue);
                }
            },
            [&](FrameBlueprint::ReroutedTransitionsEvent& e)
            {
                flushBatch(queue, queueIndex);
                wait(e.WaitEvent, queue);

                commandLists[queueIndex].emplace_back(std::move(e.CommandList));
                flushBatch(queue, queueIndex);

                signal(e.SignalEvent, queue);
            }),
            event);
        });

        // Flush last batches on each queue
        for (auto queueIdx = 0; queueIdx < mRenderPassGraph->DetectedQueueCount(); ++queueIdx)
        {
            flushBatch(GetCommandQueue(queueIdx), queueIdx);
        }

        SyncNonGraphicQueuesIntoGraphicQueue();

        // Measure frame end
        Memory::PoolCommandListAllocator::GraphicsCommandListPtr frameMeasurementsEndCmdList = mCommandListAllocator->AllocateGraphicsCommandList();
        frameMeasurementsEndCmdList->Reset();
        mGPUProfiler->RecordEventEnd(*frameMeasurementsEndCmdList, mFrameMeasurement.ProfilerEventID);
        mGPUProfiler->ReadbackEvents(*frameMeasurementsEndCmdList);
        frameMeasurementsEndCmdList->Close();
        commandLists[graphicQueueIndex].push_back(std::move(frameMeasurementsEndCmdList));
        flushBatch(GetCommandQueue(graphicQueueIndex), graphicQueueIndex);
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
        auto mostCompetentQueueIndex = std::numeric_limits<RenderPassGraph::Node::QueueIndex>::max();

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

    bool RenderDevice::IsNullCommandList(CommandListPtrVariant& variant) const
    {
        return GetComputeCommandListBase(variant) == nullptr;
    }

    HAL::Fence& RenderDevice::FenceForQueueIndex(uint64_t index)
    {
        assert_format(index < 2, "There are currently only 2 queues and 2 respective fences");
        return index == 0 ? mGraphicsQueueFence : mComputeQueueFence;
    }

    std::vector<uint64_t> RenderDevice::GetQueueTimestampFrequencies()
    {
        std::vector<uint64_t> frequencies;

        for (auto queueIdx = 0; queueIdx < mQueueCount; ++queueIdx)
            frequencies.push_back(GetCommandQueue(queueIdx).GetTimestampFrequency());

        return frequencies;
    }

    RenderDevice::FrameBlueprint::FrameBlueprint(const RenderPassGraph* graph, uint64_t bvhBuildQueueIndex, HAL::Fence* bvhFence, const std::vector<HAL::Fence*>& queueFences)
        : mPassGraph{ graph }, mBVHBuildQueueIndex{ bvhBuildQueueIndex }, mQueueFences{ queueFences }
    {
        // A special case of BVH build signal that's managed outside of the blueprint
        mBVHBuildSignal.Fence = bvhFence;
    }

    void RenderDevice::FrameBlueprint::Build()
    {
        mEventsPerQueue.clear();
        mRenderPassEventRefs.clear();
        mReroutedTransitionEventRefs.clear();
        mCurrentBatchIndices.clear();

        for (auto queueIdx = 0; queueIdx < mPassGraph->DetectedQueueCount(); ++queueIdx)
            mEventsPerQueue.emplace_back(std::make_unique<EventList>());

        mRenderPassEventRefs.resize(mPassGraph->DetectedQueueCount());
        mReroutedTransitionEventRefs.resize(mPassGraph->DetectedQueueCount());
        mCurrentBatchIndices.resize(mPassGraph->DetectedQueueCount(), 0);

        for (const RenderPassGraph::DependencyLevel& dl : mPassGraph->DependencyLevels())
        {
            for (const RenderPassGraph::Node* node : dl.Nodes())
            {
                EventList* eventList = mEventsPerQueue[node->ExecutionQueueIndex].get();
                RenderPassEvent& passEvent = std::get<RenderPassEvent>(eventList->emplace_back());
                mRenderPassEventRefs[node->ExecutionQueueIndex].push_back(std::prev(eventList->end()));

                uint64_t currentBatchIndex = mCurrentBatchIndices[node->ExecutionQueueIndex];

                bool usesRT = mPassGraph->FirstNodeThatUsesRayTracingOnQueue(node->ExecutionQueueIndex) == node && node->ExecutionQueueIndex != mBVHBuildQueueIndex;

                if (!node->NodesToSyncWith().empty() || usesRT)
                {
                    // Since we're waiting on something, we have to make a new batch
                    currentBatchIndex = ++mCurrentBatchIndices[node->ExecutionQueueIndex];

                    passEvent.WaitEvent = Wait{};

                    for (const RenderPassGraph::Node* nodeToWait : node->NodesToSyncWith())
                    {
                        const EventIt& eventToWaitIt = mRenderPassEventRefs[nodeToWait->ExecutionQueueIndex][nodeToWait->LocalToQueueExecutionIndex()];
                        const Signal* signal = &(*std::get<RenderPassEvent>(*eventToWaitIt).SignalEvent);

                        passEvent.WaitEvent->SignalsToWait.push_back(signal);
                        passEvent.WaitEvent->EventNamesToWait.push_back(StringFormat("Waiting %s Pass", nodeToWait->PassMetadata().Name.ToString().c_str()));
                    }

                    if (usesRT)
                    {
                        passEvent.WaitEvent->SignalsToWait.push_back(&mBVHBuildSignal);
                        passEvent.WaitEvent->EventNamesToWait.push_back(StringFormat("Waiting Queue %d (BVH Build)", mBVHBuildQueueIndex));
                    }
                }

                if (node->IsSyncSignalRequired())
                {
                    passEvent.SignalEvent = Signal{};
                    passEvent.SignalEvent->Fence = mQueueFences[node->ExecutionQueueIndex];
                    passEvent.SignalEvent->SignalName = StringFormat("Pass %s Signals", node->PassMetadata().Name.ToString().c_str());

                    ++mCurrentBatchIndices[node->ExecutionQueueIndex];
                }

                passEvent.EstimatedBatchIndex = currentBatchIndex;
            }
        }
    }

    void RenderDevice::FrameBlueprint::PropagateFenceUpdates()
    {
        // When blueprint is completed and all events are placed we can finally 
        // generate consecutive fence values that events will rely on
        //
        for (auto& events : mEventsPerQueue)
        {
            for (Event& event : *events)
            {
                std::visit(Foundation::MakeVisitor(
                [&](RenderPassEvent& e)
                {
                    if (e.SignalEvent)
                        e.SignalEvent->FenceValue = e.SignalEvent->Fence->IncrementExpectedValue();
                },
                [&](ReroutedTransitionsEvent& e)
                {
                    e.SignalEvent.FenceValue = e.SignalEvent.Fence->IncrementExpectedValue();
                }),
                event);
            }
        }

        // BVH Fence Value has been increased outside of the blueprint
        mBVHBuildSignal.FenceValue = mBVHBuildSignal.Fence->ExpectedValue();
    }

    RenderDevice::FrameBlueprint::ReroutedTransitionsEvent& RenderDevice::FrameBlueprint::InsertReroutedTransitionsEvent(
        std::optional<uint64_t> afterDependencyLevel,
        uint64_t waitingDependencyLevel,
        uint64_t queueIndex,
        const std::vector<uint64_t>& queuesToSyncWith)
    {
        EventList& events = *mEventsPerQueue[queueIndex];
        ReroutedTransitionsEvent newTransitionsEvent{};
        EventIt transitionsEventIt{};

        newTransitionsEvent.SignalEvent.Fence = mQueueFences[queueIndex];
        newTransitionsEvent.SignalEvent.SignalName = StringFormat("Rerouted Transitions for Dependency Level %d", waitingDependencyLevel);

        if (afterDependencyLevel)
        {
            assert_format(*afterDependencyLevel < waitingDependencyLevel, "Synchronization order is wrong");

            const RenderPassGraph::DependencyLevel& dl = mPassGraph->DependencyLevels()[*afterDependencyLevel];

            // Make rerouted transitions wait for passes in dependency level we're inserting them after
            for (uint64_t queueToWait : queuesToSyncWith)
            {   
                // We may have encounter a case when there is nothing to sync with on a particular queue.
                // If dependency level doesn't contain render passes on a particular queue, we search
                // in previous dependency levels until we hit the start of the frame or we find a suitable render pass to sync with.
                //
                int64_t dlIndex = *afterDependencyLevel;
                while (dlIndex >= 0 && mPassGraph->DependencyLevels()[dlIndex].NodesForQueue(queueToWait).empty())
                {
                    --dlIndex;
                }

                if (dlIndex < 0)
                    continue;

                const RenderPassGraph::Node* nodeToWait = mPassGraph->DependencyLevels()[dlIndex].NodesForQueue(queueToWait).front();
                RenderPassEvent& passEvent = GetRenderPassEvent(*nodeToWait);
                
                if (!passEvent.SignalEvent)
                {
                    passEvent.SignalEvent = Signal{};
                    passEvent.SignalEvent->Fence = mQueueFences[queueToWait];
                    passEvent.SignalEvent->SignalName = StringFormat("Pass %s Signals", nodeToWait->PassMetadata().Name.ToString().c_str());
                }

                newTransitionsEvent.WaitEvent.SignalsToWait.push_back(&(*passEvent.SignalEvent));
                newTransitionsEvent.WaitEvent.EventNamesToWait.push_back(StringFormat("Waiting %s Pass", nodeToWait->PassMetadata().Name.ToString().c_str()));
            }

            const RenderPassGraph::Node* lastNodeInDL = dl.NodesForQueue(queueIndex).back();
            EventIt passEventIt = mRenderPassEventRefs[queueIndex][lastNodeInDL->LocalToQueueExecutionIndex()];
            transitionsEventIt = events.emplace(++passEventIt, std::move(newTransitionsEvent));
        }
        else
        {
            // Insert rerouted transitions before any render pass on the queue
            transitionsEventIt = events.emplace(events.begin(), std::move(newTransitionsEvent));
        }

        mReroutedTransitionEventRefs[queueIndex].push_back(transitionsEventIt);

        ReroutedTransitionsEvent& transitionsEvent = std::get<ReroutedTransitionsEvent>(*transitionsEventIt);

        // Now make first passes in waiting dependency level wait for rerouted transitions
        const RenderPassGraph::DependencyLevel& waitingDL = mPassGraph->DependencyLevels()[waitingDependencyLevel];

        // Make rerouted transitions wait for passes in dependency level we're inserting them after
        for (uint64_t waitingQueue : queuesToSyncWith)
        {
            const RenderPassGraph::Node* firstNodeInDL = waitingDL.NodesForQueue(waitingQueue).front();
            RenderPassEvent& passEvent = GetRenderPassEvent(*firstNodeInDL);
            
            if (!passEvent.WaitEvent)
                passEvent.WaitEvent = Wait{};

            passEvent.WaitEvent->SignalsToWait.push_back(&transitionsEvent.SignalEvent);
            passEvent.WaitEvent->EventNamesToWait.push_back(StringFormat("Waiting for Rerouted Transitions on Queue %d", queueIndex));
        }

        return transitionsEvent;
    }

    void RenderDevice::FrameBlueprint::Traverse(const BlueprintTraverser& traverser)
    {
        for (auto queueIdx = 0; queueIdx < mPassGraph->DetectedQueueCount(); ++queueIdx)
        {
            for (Event& event : *mEventsPerQueue[queueIdx])
            {
                traverser(queueIdx, event);
            }
        }
    }

    const RenderDevice::FrameBlueprint::RenderPassEvent& RenderDevice::FrameBlueprint::GetRenderPassEvent(const RenderPassGraph::Node& node) const
    {
        return std::get<RenderPassEvent>(*mRenderPassEventRefs[node.ExecutionQueueIndex][node.LocalToQueueExecutionIndex()]);
    }

    RenderDevice::FrameBlueprint::RenderPassEvent& RenderDevice::FrameBlueprint::GetRenderPassEvent(const RenderPassGraph::Node& node)
    {
        return std::get<RenderPassEvent>(*mRenderPassEventRefs[node.ExecutionQueueIndex][node.LocalToQueueExecutionIndex()]);
    }

}
