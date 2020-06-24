#include "RenderDevice.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    RenderDevice::RenderDevice(
        const HAL::Device& device, 
        const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
        Memory::PoolCommandListAllocator* commandListAllocator,
        Memory::ResourceStateTracker* resourceStateTracker,
        PipelineResourceStorage* resourceStorage,
        PipelineStateManager* pipelineStateManager,
        const RenderPassGraph* renderPassGraph,
        const RenderSurfaceDescription& defaultRenderSurface)
        :
        mGraphicsQueue{ device },
        mComputeQueue{ device },
        mUniversalGPUDescriptorHeap{ universalGPUDescriptorHeap },
        mCommandListAllocator{ commandListAllocator },
        mResourceStateTracker{ resourceStateTracker },
        mResourceStorage{ resourceStorage },
        mPipelineStateManager{ pipelineStateManager },
        mRenderPassGraph{ renderPassGraph },
        mDefaultRenderSurface{ defaultRenderSurface },
        mGraphicsQueueFence{ device },
        mComputeQueueFence{ device }
    {
        mGraphicsQueue.SetDebugName("Graphics Command Queue");
        mComputeQueue.SetDebugName("Async Compute Command Queue");
    }

    void RenderDevice::SetRenderTarget(const RenderPassGraph::Node* passNode, Foundation::Name rtName, std::optional<Foundation::Name> dsName)
    {
        assert_format(RenderPassExecutionQueue{ passNode->ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Set command is unsupported on asynchronous compute queue");

        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();
        const HAL::DSDescriptor* dsDescriptor = dsName ? mResourceStorage->GetDepthStencilDescriptor(*dsName, passNode->PassMetadata().Name) : nullptr;
        cmdList->SetRenderTarget(*mResourceStorage->GetRenderTargetDescriptor(rtName, passNode->PassMetadata().Name), dsDescriptor);
    }

    void RenderDevice::SetBackBufferAsRenderTarget(const RenderPassGraph::Node* passNode, std::optional<Foundation::Name> dsName)
    {
        assert_format(RenderPassExecutionQueue{ passNode->ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Set command is unsupported on asynchronous compute queue");

        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();
        const HAL::DSDescriptor* dsDescriptor = dsName ? mResourceStorage->GetDepthStencilDescriptor(*dsName, passNode->PassMetadata().Name) : nullptr;
        cmdList->SetRenderTarget(*mBackBuffer->GetRTDescriptor(), dsDescriptor);
    }

    void RenderDevice::ClearRenderTarget(const RenderPassGraph::Node* passNode, Foundation::Name rtName)
    {
        assert_format(RenderPassExecutionQueue{ passNode->ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Clear command is unsupported on asynchronous compute queue");

        const Memory::Texture* renderTarget = mResourceStorage->GetPerResourceData(rtName)->Texture.get();
        assert_format(renderTarget, "Render target doesn't exist");

        auto clearValue = std::get_if<HAL::ColorClearValue>(&renderTarget->Properties().OptimizedClearValue);
        assert_format(clearValue, "Texture does not contain optimized color clear value");

        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();
        cmdList->ClearRenderTarget(*mResourceStorage->GetRenderTargetDescriptor(rtName, passNode->PassMetadata().Name), *clearValue);
    }

    void RenderDevice::ClearDepth(const RenderPassGraph::Node* passNode, Foundation::Name dsName)
    {
        assert_format(RenderPassExecutionQueue{ passNode->ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Depth Clear command is unsupported on asynchronous compute queue");

        const Memory::Texture* depthAttachment = mResourceStorage->GetPerResourceData(dsName)->Texture.get();
        assert_format(depthAttachment, "Depth/stencil attachment doesn't exist");

        auto clearValue = std::get_if<HAL::DepthStencilClearValue>(&depthAttachment->Properties().OptimizedClearValue);
        assert_format(clearValue, "Texture does not contain optimized depth/stencil clear value");

        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();
        cmdList->CleadDepthStencil(*mResourceStorage->GetDepthStencilDescriptor(dsName, passNode->PassMetadata().Name), clearValue->Depth);
    }

    void RenderDevice::SetViewport(const RenderPassGraph::Node* passNode, const HAL::Viewport& viewport)
    {
        assert_format(RenderPassExecutionQueue{ passNode->ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Viewport Set command is unsupported on asynchronous compute queue");

        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();
        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        passHelpers.LastAppliedViewport = viewport;
        cmdList->SetViewport(viewport);
    }

    void RenderDevice::Draw(const RenderPassGraph::Node* passNode, uint32_t vertexCount, uint32_t instanceCount)
    {
        assert_format(RenderPassExecutionQueue{ passNode->ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Draw command is unsupported on asynchronous compute queue");

        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();
        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        // Apply default viewport if none were provided by the render pass yet
        if (!passHelpers.LastAppliedViewport)
        {
            passHelpers.LastAppliedViewport = HAL::Viewport(mDefaultRenderSurface.Dimensions().Width, mDefaultRenderSurface.Dimensions().Height);
            cmdList->SetViewport(*passHelpers.LastAppliedViewport);
        }

        // Inset UAV barriers between draws
        if (passHelpers.ExecutedRenderCommandsCount > 0)
        {
            cmdList->InsertBarriers(passHelpers.UAVBarriers);
        }

        BindGraphicsPassRootConstantBuffer(passNode, cmdList);
        cmdList->Draw(vertexCount, 0);

        passHelpers.ResourceStoragePassData->IsAllowedToAdvanceConstantBufferOffset = true;
        passHelpers.ExecutedRenderCommandsCount++;
    }
    
    void RenderDevice::Draw(const RenderPassGraph::Node* passNode, const DrawablePrimitive& primitive)
    {
        Draw(passNode, primitive.VertexCount());
    }

    void RenderDevice::Dispatch(const RenderPassGraph::Node* passNode, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList);
        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        if (passHelpers.ExecutedRenderCommandsCount > 0)
        {
            cmdList->InsertBarriers(passHelpers.UAVBarriers);
        }

        BindComputePassRootConstantBuffer(passNode, cmdList);
        cmdList->Dispatch(groupCountX, groupCountY, groupCountZ);

        passHelpers.ResourceStoragePassData->IsAllowedToAdvanceConstantBufferOffset = true;
        passHelpers.ExecutedRenderCommandsCount++;
    }

    void RenderDevice::DispatchRays(const RenderPassGraph::Node* passNode, uint32_t width, uint32_t height, uint32_t depth)
    {
        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        assert_format(passHelpers.LastAppliedRTStateDispatchInfo, "No Ray Tracing state / dispatch info were applied before Ray Dispatch");

        HAL::RayDispatchInfo dispatchInfo = *passHelpers.LastAppliedRTStateDispatchInfo;
        dispatchInfo.SetWidth(width);
        dispatchInfo.SetHeight(height);
        dispatchInfo.SetDepth(depth);

        HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList);

        if (passHelpers.ExecutedRenderCommandsCount > 0)
        {
            cmdList->InsertBarriers(passHelpers.UAVBarriers);
        }

        BindComputePassRootConstantBuffer(passNode, cmdList);
        cmdList->DispatchRays(dispatchInfo);
        passHelpers.ResourceStoragePassData->IsAllowedToAdvanceConstantBufferOffset = true;
        passHelpers.ExecutedRenderCommandsCount++;
    }

    void RenderDevice::BindBuffer(const RenderPassGraph::Node* passNode, Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        const PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceName);
        assert_format(resourceData->Buffer, "Buffer ' ", resourceName.ToString(), "' doesn't exist");

        BindExternalBuffer(passNode, *resourceData->Buffer, shaderRegister, registerSpace, registerType);
    }

    void RenderDevice::AllocateUploadCommandList()
    {
        mPreRenderUploadsCommandList = mCommandListAllocator->AllocateGraphicsCommandList();
    }

    void RenderDevice::AllocateRTASBuildsCommandList()
    {
        mRTASBuildsCommandList = mCommandListAllocator->AllocateComputeCommandList();
    }

    void RenderDevice::AllocateWorkerCommandLists()
    {
        CreatePassHelpers();

        mPassCommandLists.clear();

        for (const RenderPassGraph::Node& node : mRenderPassGraph->Nodes())
        {
            CommandListPtrVariant cmdListVariant = AllocateCommandListForQueue(node.ExecutionQueueIndex);
            std::visit([this](auto&& cmdList) { cmdList->SetDescriptorHeap(*mUniversalGPUDescriptorHeap); }, cmdListVariant);
            mPassCommandLists.emplace_back(PassCommandLists{ GraphicsCommandListPtr{nullptr}, std::move(cmdListVariant) });
        }
    }

    void RenderDevice::BindGraphicsCommonResources(const RenderPassGraph::Node* passNode, const HAL::RootSignature* rootSignature, HAL::GraphicsCommandListBase* cmdList)
    {   
        auto commonParametersIndexOffset = rootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        // Look at PipelineStateManager for base root signature parameter ordering
        HAL::DescriptorAddress SRRangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::ShaderResource);
        HAL::DescriptorAddress UARangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::UnorderedAccess);

        const PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        // Alias different registers to one GPU address
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 3 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 4 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 5 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 6 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 7 + commonParametersIndexOffset);

        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 8 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 9 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 10 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 11 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 12 + commonParametersIndexOffset);

        cmdList->SetGraphicsRootConstantBuffer(*mResourceStorage->GlobalRootConstantsBuffer()->HALBuffer(), 0 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootConstantBuffer(*mResourceStorage->PerFrameRootConstantsBuffer()->HALBuffer(), 1 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootUnorderedAccessResource(*passHelpers.ResourceStoragePassData->PassDebugBuffer->HALBuffer(), 13 + commonParametersIndexOffset);
    }

    void RenderDevice::BindComputeCommonResources(const RenderPassGraph::Node* passNode, const HAL::RootSignature* rootSignature, HAL::ComputeCommandListBase* cmdList)
    {
        auto commonParametersIndexOffset = rootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        // Look at PipelineStateManager for base root signature parameter ordering
        HAL::DescriptorAddress SRRangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::ShaderResource);
        HAL::DescriptorAddress UARangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::UnorderedAccess);

        const PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        // Alias different registers to one GPU address
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 3 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 4 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 5 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 6 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 7 + commonParametersIndexOffset);

        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 8 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 9 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 10 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 11 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 12 + commonParametersIndexOffset);

        cmdList->SetComputeRootConstantBuffer(*mResourceStorage->GlobalRootConstantsBuffer()->HALBuffer(), 0 + commonParametersIndexOffset);
        cmdList->SetComputeRootConstantBuffer(*mResourceStorage->PerFrameRootConstantsBuffer()->HALBuffer(), 1 + commonParametersIndexOffset);
        cmdList->SetComputeRootUnorderedAccessResource(*passHelpers.ResourceStoragePassData->PassDebugBuffer->HALBuffer(), 13 + commonParametersIndexOffset);
    }

    void RenderDevice::BindGraphicsPassRootConstantBuffer(const RenderPassGraph::Node* passNode, HAL::GraphicsCommandListBase* cmdList)
    {
        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        auto commonParametersIndexOffset = passHelpers.LastSetRootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        if (!passHelpers.ResourceStoragePassData->PassConstantBuffer)
        {
            return;
        }

        HAL::GPUAddress address = 
            passHelpers.ResourceStoragePassData->PassConstantBuffer->HALBuffer()->GPUVirtualAddress() + 
            passHelpers.ResourceStoragePassData->PassConstantBufferMemoryOffset;

        // Already bound
        if (passHelpers.LastBoundRootConstantBufferAddress == address)
        {
            return;
        }

        passHelpers.LastBoundRootConstantBufferAddress = address;
        cmdList->SetGraphicsRootConstantBuffer(address, 2 + commonParametersIndexOffset);
    }

    void RenderDevice::BindComputePassRootConstantBuffer(const RenderPassGraph::Node* passNode, HAL::ComputeCommandListBase* cmdList)
    {
        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        auto commonParametersIndexOffset = passHelpers.LastSetRootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        if (!passHelpers.ResourceStoragePassData->PassConstantBuffer)
        {
            return;
        }

        HAL::GPUAddress address =
            passHelpers.ResourceStoragePassData->PassConstantBuffer->HALBuffer()->GPUVirtualAddress() +
            passHelpers.ResourceStoragePassData->PassConstantBufferMemoryOffset;

        // Already bound
        if (passHelpers.LastBoundRootConstantBufferAddress == address)
        {
            return;
        }

        passHelpers.LastBoundRootConstantBufferAddress = address;
        cmdList->SetComputeRootConstantBuffer(address, 2 + commonParametersIndexOffset);
    }

    void RenderDevice::CreatePassHelpers()
    {
        mPassHelpers.resize(mRenderPassGraph->Nodes().size());

        for (const RenderPassGraph::Node& node : mRenderPassGraph->Nodes())
        {
            // Zero out helpers on new frame
            mPassHelpers[node.GlobalExecutionIndex()] = PassHelpers{};
            PassHelpers& helpers = mPassHelpers[node.GlobalExecutionIndex()];
            helpers.ResourceStoragePassData = mResourceStorage->GetPerPassData(node.PassMetadata().Name);
            helpers.ResourceStoragePassData->LastSetConstantBufferDataSize = 0;
            helpers.ResourceStoragePassData->PassConstantBufferMemoryOffset = 0;
            helpers.ResourceStoragePassData->IsAllowedToAdvanceConstantBufferOffset = false;
        }
    }

    void RenderDevice::BatchCommandLists()
    {
        mCommandListBatches.clear();
        mCommandListBatches.resize(mQueueCount);

        mReroutedTransitionsCommandLists.clear();
        mReroutedTransitionsCommandLists.resize(mRenderPassGraph->DependencyLevels().size());

        mPerNodeBeginBarriers.clear();
        mPerNodeBeginBarriers.resize(mRenderPassGraph->Nodes().size());

        // If memory layout did not change we reuse aliasing barriers from previous frame.
        // Otherwise we start from scratch.
        if (mResourceStorage->HasMemoryLayoutChange())
        {
            mPerNodeAliasingBarriers.clear();
            mPerNodeAliasingBarriers.resize(mRenderPassGraph->Nodes().size());
        }

        mSubresourcesPreviousTransitionInfo.clear();

        for (const RenderPassGraph::DependencyLevel& dependencyLevel : mRenderPassGraph->DependencyLevels())
        {
            mDependencyLevelTransitionBarriers.clear();
            mDependencyLevelTransitionBarriers.resize(dependencyLevel.Nodes().size());

            GatherResourceTransitionKnowledge(dependencyLevel);
            BatchCommandListsWithTransitionRerouting(dependencyLevel);
            BatchCommandListsWithoutTransitionRerouting(dependencyLevel);
        }

        RecordBeginBarriers();
    }

    void RenderDevice::GatherResourceTransitionKnowledge(const RenderPassGraph::DependencyLevel& dependencyLevel)
    {
        mDependencyLevelQueuesThatRequireTransitionRerouting = dependencyLevel.QueuesInvoledInCrossQueueResourceReads();

        bool backBufferTransitioned = false;

        for (const RenderPassGraph::Node* node : dependencyLevel.Nodes())
        {
            auto requestTransition = [&](RenderPassGraph::SubresourceName subresourceName, bool isReadDependency)
            {
                auto [resourceName, subresourceIndex] = mRenderPassGraph->DecodeSubresourceName(subresourceName);
                PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceName);

                HAL::ResourceState newState = isReadDependency ?
                    resourceData->SchedulingInfo.GetSubresourceCombinedReadStates(subresourceIndex) :
                    resourceData->SchedulingInfo.GetSubresourceWriteState(subresourceIndex);

                std::optional<HAL::ResourceTransitionBarrier> barrier =
                    mResourceStateTracker->TransitionToStateImmediately(resourceData->GetGPUResource()->HALResource(), newState, subresourceIndex, false);

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

                // Redundant transition
                if (!barrier)
                {
                    return;
                }

                mDependencyLevelTransitionBarriers[node->LocalToDependencyLevelExecutionIndex()].push_back({ subresourceName, *barrier, resourceData->GetGPUResource()->HALResource() });

                // Another reason to reroute resource transitions into another queue is incompatibility 
                // of resource state transitions with receiving queue
                if (!IsStateTransitionSupportedOnQueue(node->ExecutionQueueIndex, barrier->BeforeStates(), barrier->AfterStates()))
                {
                    mDependencyLevelQueuesThatRequireTransitionRerouting.insert(node->ExecutionQueueIndex);
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

            for (Foundation::Name resourceName : node->AllResources())
            {
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

    void RenderDevice::CollectNodeTransitions(const RenderPassGraph::Node* node, uint64_t currentCommandListBatchIndex, HAL::ResourceBarrierCollection& collection)
    {
        const std::vector<SubresourceTransitionInfo>& nodeTransitionBarriers = mDependencyLevelTransitionBarriers[node->LocalToDependencyLevelExecutionIndex()];
        const HAL::ResourceBarrierCollection& nodeAliasingBarriers = mPerNodeAliasingBarriers[node->GlobalExecutionIndex()];

        collection.AddBarriers(nodeAliasingBarriers);

        for (const SubresourceTransitionInfo& transitionInfo : nodeTransitionBarriers)
        {
            auto previousTransitionInfoIt = mSubresourcesPreviousTransitionInfo.find(transitionInfo.SubresourceName);
            bool foundPreviousTransition = previousTransitionInfoIt != mSubresourcesPreviousTransitionInfo.end();
            bool subresourceTransitionedAtLeastOnce = foundPreviousTransition && previousTransitionInfoIt->second.CommandListBatchIndex == currentCommandListBatchIndex;

            if (!subresourceTransitionedAtLeastOnce)
            {
                bool implicitTransitionPossible = Memory::ResourceStateTracker::CanResourceBeImplicitlyTransitioned(
                    *transitionInfo.Resource, transitionInfo.TransitionBarrier.BeforeStates(), transitionInfo.TransitionBarrier.AfterStates());

                if (implicitTransitionPossible)
                {
                    continue;
                }
            }

            if (foundPreviousTransition)
            {
                const SubresourcePreviousTransitionInfo& previousTransitionInfo = previousTransitionInfoIt->second;

                // Split barrier is only possible when transmitting queue supports transitions for both before and after states
                bool isSplitBarrierPossible = IsStateTransitionSupportedOnQueue(
                    previousTransitionInfo.Node->ExecutionQueueIndex, transitionInfo.TransitionBarrier.BeforeStates(), transitionInfo.TransitionBarrier.AfterStates()
                );

                // There is no sense in splitting barriers between two adjacent render passes. 
                // That will only double the amount of barriers without any performance gain.
                bool currentNodeIsNextToPrevious = node->LocalToQueueExecutionIndex() - previousTransitionInfo.Node->LocalToQueueExecutionIndex() <= 1;

                if (isSplitBarrierPossible && !currentNodeIsNextToPrevious)
                {
                    auto [beginBarrier, endBarrier] = transitionInfo.TransitionBarrier.Split();
                    collection.AddBarrier(endBarrier);
                    mPerNodeBeginBarriers[previousTransitionInfo.Node->GlobalExecutionIndex()].AddBarrier(beginBarrier);
                }
                else
                {
                    collection.AddBarrier(transitionInfo.TransitionBarrier);
                }
            }
            else
            {
                collection.AddBarrier(transitionInfo.TransitionBarrier);
            }

            mSubresourcesPreviousTransitionInfo[transitionInfo.SubresourceName] = { node, currentCommandListBatchIndex };
        }
    }

    void RenderDevice::BatchCommandListsWithTransitionRerouting(const RenderPassGraph::DependencyLevel& dependencyLevel)
    {
        if (mDependencyLevelQueuesThatRequireTransitionRerouting.empty())
        {
            return;
        }

        uint64_t mostCompetentQueueIndex = FindMostCompetentQueueIndex(mDependencyLevelQueuesThatRequireTransitionRerouting);
        mReroutedTransitionsCommandLists[dependencyLevel.LevelIndex()] = AllocateCommandListForQueue(mostCompetentQueueIndex);
        CommandListPtrVariant& commandListVariant = mReroutedTransitionsCommandLists[dependencyLevel.LevelIndex()];
        HAL::ComputeCommandListBase* transitionsCommandList = GetComputeCommandListBase(commandListVariant);

        std::vector<CommandListBatch>& mostCompetentQueueBatches = mCommandListBatches[mostCompetentQueueIndex];
        CommandListBatch& reroutedTransitionsBatch = mostCompetentQueueBatches.emplace_back();
        reroutedTransitionsBatch.FenceToSignal = &FenceForQueueIndex(mostCompetentQueueIndex);
        reroutedTransitionsBatch.CommandLists.emplace_back(GetHALCommandListVariant(commandListVariant));

        HAL::ResourceBarrierCollection reroutedTransitionBarrires;

        std::vector<CommandListBatch*> dependencyLevelPerQueueBatches{ mQueueCount, nullptr };

        for (RenderPassGraph::Node::QueueIndex queueIndex : mDependencyLevelQueuesThatRequireTransitionRerouting)
        {
            for (const RenderPassGraph::Node* node : dependencyLevel.NodesForQueue(queueIndex))
            {
                for (const RenderPassGraph::Node* nodeToWait : node->NodesToSyncWith())
                {
                    reroutedTransitionsBatch.FencesToWait.insert(&FenceForQueueIndex(nodeToWait->ExecutionQueueIndex));
                }

                if (mRenderPassGraph->FirstNodeThatUsesRayTracing() == node)
                {
                    reroutedTransitionsBatch.FencesToWait.insert(&FenceForQueueIndex(mBVHBuildsQueueIndex));
                }

                if (!dependencyLevelPerQueueBatches[node->ExecutionQueueIndex])
                {
                    dependencyLevelPerQueueBatches[node->ExecutionQueueIndex] = &mCommandListBatches[node->ExecutionQueueIndex].emplace_back();
                }

                CommandListBatch* currentBatchInCurrentDependencyLevel = dependencyLevelPerQueueBatches[node->ExecutionQueueIndex];

                currentBatchInCurrentDependencyLevel->FencesToWait.insert(reroutedTransitionsBatch.FenceToSignal);
                currentBatchInCurrentDependencyLevel->CommandLists.push_back(GetHALCommandListVariant(mPassCommandLists[node->GlobalExecutionIndex()].WorkCommandList));

                uint64_t currentCommandListBatchIndex = mCommandListBatches[queueIndex].size() - 1;
                CollectNodeTransitions(node, currentCommandListBatchIndex, reroutedTransitionBarrires);

                if (node->IsSyncSignalRequired())
                {
                    currentBatchInCurrentDependencyLevel->FenceToSignal = &FenceForQueueIndex(node->ExecutionQueueIndex);
                    dependencyLevelPerQueueBatches[node->ExecutionQueueIndex] = &mCommandListBatches[node->ExecutionQueueIndex].emplace_back();
                }
            }
        }

        transitionsCommandList->InsertBarriers(reroutedTransitionBarrires);
    }

    void RenderDevice::BatchCommandListsWithoutTransitionRerouting(const RenderPassGraph::DependencyLevel& dependencyLevel)
    {
        for (auto queueIdx = 0u; queueIdx < mRenderPassGraph->DetectedQueueCount(); ++queueIdx)
        {
            if (mDependencyLevelQueuesThatRequireTransitionRerouting.find(queueIdx) != mDependencyLevelQueuesThatRequireTransitionRerouting.end())
            {
                continue;
            }

            auto& nodesForQueue = dependencyLevel.NodesForQueue(queueIdx);

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
                    if (!currentBatch->CommandLists.empty())
                    {
                        currentBatch = &mCommandListBatches[queueIdx].emplace_back();
                    }

                    for (const RenderPassGraph::Node* nodeToWait : node->NodesToSyncWith())
                    {
                        currentBatch->FencesToWait.insert(&FenceForQueueIndex(nodeToWait->ExecutionQueueIndex));
                    }

                    if (usesRT)
                    {
                        currentBatch->FencesToWait.insert(&FenceForQueueIndex(mBVHBuildsQueueIndex));
                    }
                }

                // On queues that do not require transition rerouting each node will have its own transition collection
                HAL::ResourceBarrierCollection nodeBarriers{};
                uint64_t currentCommandListBatchIndex = mCommandListBatches[queueIdx].size() - 1;

                CollectNodeTransitions(node, currentCommandListBatchIndex, nodeBarriers);

                if (nodeBarriers.BarrierCount() > 0)
                {
                    mPassCommandLists[node->GlobalExecutionIndex()].TransitionsCommandList = AllocateCommandListForQueue(queueIdx);
                    CommandListPtrVariant& cmdListVariant = mPassCommandLists[node->GlobalExecutionIndex()].TransitionsCommandList;
                    HAL::ComputeCommandListBase* transitionsCommandList = GetComputeCommandListBase(cmdListVariant);
                    transitionsCommandList->InsertBarriers(nodeBarriers);
                    currentBatch->CommandLists.push_back(GetHALCommandListVariant(cmdListVariant));
                }

                currentBatch->CommandLists.push_back(GetHALCommandListVariant(mPassCommandLists[node->GlobalExecutionIndex()].WorkCommandList));

                if (node->IsSyncSignalRequired())
                {
                    currentBatch->FenceToSignal = &FenceForQueueIndex(queueIdx);
                    mCommandListBatches[queueIdx].emplace_back();
                }
            }
        }
    }

    void RenderDevice::RecordBeginBarriers()
    {
        for (const RenderPassGraph::Node& node : mRenderPassGraph->Nodes())
        {
            const HAL::ResourceBarrierCollection& beginBarriers = mPerNodeBeginBarriers[node.GlobalExecutionIndex()];
            
            if (beginBarriers.BarrierCount() > 0)
            {
                HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[node.GlobalExecutionIndex()].WorkCommandList);
                cmdList->InsertBarriers(beginBarriers);
            }
        }
    }

    void RenderDevice::ExetuteCommandLists()
    {
        // Run initial upload commands
        mGraphicsQueueFence.IncrementExpectedValue();
        // Transition uploaded resources to readable states
        mPreRenderUploadsCommandList->InsertBarriers(mResourceStateTracker->ApplyRequestedTransitions());
        mPreRenderUploadsCommandList->Close();
        mGraphicsQueue.ExecuteCommandList(*mPreRenderUploadsCommandList);
        mGraphicsQueue.SignalFence(mGraphicsQueueFence);

        // Wait for uploads, run RT AS builds
        mComputeQueueFence.IncrementExpectedValue();
        mComputeQueue.WaitFence(mGraphicsQueueFence);
        mRTASBuildsCommandList->Close();
        mComputeQueue.ExecuteCommandList(*mRTASBuildsCommandList);
        mComputeQueue.SignalFence(mComputeQueueFence);

        for (auto queueIdx = 0; queueIdx < mQueueCount; ++queueIdx)
        {
            std::vector<CommandListBatch>& batches = mCommandListBatches[queueIdx];

            for (auto batchIdx = 0; batchIdx < batches.size(); ++batchIdx)
            {
                CommandListBatch& batch = batches[batchIdx];
                HAL::CommandQueue& queue = GetCommandQueue(queueIdx);

                for (const HAL::Fence* fenceToWait : batch.FencesToWait)
                {
                    queue.WaitFence(*fenceToWait);
                }

                if (RenderPassExecutionQueue{ queueIdx } == RenderPassExecutionQueue::Graphics)
                {
                    std::vector<HAL::GraphicsCommandList*> graphicsCommands;
                    HAL::GraphicsCommandQueue* graphicsQueue = dynamic_cast<HAL::GraphicsCommandQueue*>(&queue);

                    for (auto cmdListIdx = 0; cmdListIdx < batch.CommandLists.size(); ++cmdListIdx)
                    {
                        HALCommandListPtrVariant& cmdListVariant = batch.CommandLists[cmdListIdx];
                        auto cmdList = std::get<HAL::GraphicsCommandList*>(cmdListVariant);

                        bool isLastGraphicsCmdList = batchIdx == batches.size() - 1 && cmdListIdx == batch.CommandLists.size() - 1;

                        if (isLastGraphicsCmdList)
                        {
                            cmdList->InsertBarriers(mResourceStateTracker->TransitionToStateImmediately(mBackBuffer->HALResource(), HAL::ResourceState::Present));
                        }

                        cmdList->Close();
                        graphicsCommands.push_back(cmdList);
                    }

                    graphicsQueue->ExecuteCommandLists(graphicsCommands.data(), graphicsCommands.size());
                }
                else
                {
                    std::vector<HAL::ComputeCommandList*> computeCommands;
                    HAL::ComputeCommandQueue* computeQueue = dynamic_cast<HAL::ComputeCommandQueue*>(&queue);

                    for (HALCommandListPtrVariant& cmdListVariant : batch.CommandLists)
                    {
                        auto cmdList = std::get<HAL::ComputeCommandList*>(cmdListVariant);
                        cmdList->Close();
                        computeCommands.push_back(cmdList);
                    }

                    computeQueue->ExecuteCommandLists(computeCommands.data(), computeCommands.size());
                }

                if (batch.FenceToSignal)
                {
                    batch.FenceToSignal->IncrementExpectedValue();
                    queue.SignalFence(*batch.FenceToSignal);
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

    uint64_t RenderDevice::FindMostCompetentQueueIndex(const std::unordered_set<RenderPassGraph::Node::QueueIndex>& queueIndices) const
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

    RenderDevice::HALCommandListPtrVariant RenderDevice::GetHALCommandListVariant(CommandListPtrVariant& variant) const
    {
        HALCommandListPtrVariant halv;
        std::visit([&halv](auto&& v) { halv = v.get(); }, variant);
        return halv;
    }

    HAL::Fence& RenderDevice::FenceForQueueIndex(uint64_t index)
    {
        assert_format(index < 2, "There are currently only 2 queues and 2 respective fences");
        return index == 0 ? mGraphicsQueueFence : mComputeQueueFence;
    }

    void RenderDevice::ApplyPipelineState(const RenderPassGraph::Node* passNode, Foundation::Name psoName)
    {
        std::optional<PipelineStateManager::PipelineStateVariant> state = mPipelineStateManager->GetPipelineState(psoName);
        assert_format(state, "Pipeline state doesn't exist");

        if (state->ComputePSO) ApplyState(passNode, state->ComputePSO);
        else if (state->GraphicPSO) ApplyState(passNode, state->GraphicPSO);
        else if (state->RayTracingPSO) ApplyState(passNode, state->RayTracingPSO, state->BaseRayDispatchInfo);

        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];
        passHelpers.LastSetPipelineState = state;
    }

    void RenderDevice::ApplyState(const RenderPassGraph::Node* passNode, const HAL::GraphicsPipelineState* state)
    {
        RenderPassExecutionQueue queueType{ passNode->ExecutionQueueIndex };
        assert_format(queueType != RenderPassExecutionQueue::AsyncCompute, "Cannot apply Graphics State on Async Compute queue");
        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();

        cmdList->SetPipelineState(*state);
        cmdList->SetPrimitiveTopology(state->GetPrimitiveTopology());
        cmdList->SetGraphicsRootSignature(*state->GetRootSignature());

        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];
        passHelpers.LastSetRootSignature = state->GetRootSignature();

        BindGraphicsCommonResources(passNode, state->GetRootSignature(), cmdList);
    }
    
    void RenderDevice::ApplyState(const RenderPassGraph::Node* passNode, const HAL::ComputePipelineState* state)
    {
        HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList);

        cmdList->SetPipelineState(*state);
        cmdList->SetComputeRootSignature(*state->GetRootSignature());

        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];
        passHelpers.LastSetRootSignature = state->GetRootSignature();

        BindComputeCommonResources(passNode, state->GetRootSignature(), cmdList);
    }

    void RenderDevice::ApplyState(const RenderPassGraph::Node* passNode, const HAL::RayTracingPipelineState* state, const HAL::RayDispatchInfo* dispatchInfo)
    {
        assert_format(passNode->UsesRayTracing, "Render pass ", passNode->PassMetadata().Name.ToString(), " didn't schedule Ray Tracing usage");

        HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList);

        cmdList->SetPipelineState(*state);
        cmdList->SetComputeRootSignature(*state->GetGlobalRootSignature());

        PassHelpers& passHelpers = mPassHelpers[passNode->GlobalExecutionIndex()];
        passHelpers.LastSetRootSignature = state->GetGlobalRootSignature();
        passHelpers.LastAppliedRTStateDispatchInfo = dispatchInfo;

        BindComputeCommonResources(passNode, state->GetGlobalRootSignature(), cmdList);
    }

    void RenderDevice::BindExternalBuffer(const RenderPassGraph::Node* passNode, const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        PassHelpers& helpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        assert_format(helpers.LastSetPipelineState, "No pipeline state applied before binding a buffer in ", passNode->PassMetadata().Name.ToString(), " render pass");

        // Ray Tracing bindings go to compute 
        if (helpers.LastSetPipelineState->ComputePSO || helpers.LastSetPipelineState->RayTracingPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->ComputePSO ?
                helpers.LastSetPipelineState->ComputePSO->GetRootSignature() : 
                helpers.LastSetPipelineState->RayTracingPSO->GetGlobalRootSignature();

            HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList);

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, registerType });

            assert_format(index, "Root signature parameter doesn't exist. It either wasn't created or register/space/type aren't correctly specified.");

            switch (registerType)
            {
            case HAL::ShaderRegister::ConstantBuffer: cmdList->SetComputeRootConstantBuffer(*buffer.HALBuffer(), index->IndexInSignature); break;
            case HAL::ShaderRegister::ShaderResource: cmdList->SetComputeRootDescriptorTable(buffer.GetSRDescriptor()->GPUAddress(), index->IndexInSignature); break;
            case HAL::ShaderRegister::UnorderedAccess: cmdList->SetComputeRootDescriptorTable(buffer.GetUADescriptor()->GPUAddress(), index->IndexInSignature); break;
            case HAL::ShaderRegister::Sampler: assert_format(false, "Incompatible register type");
            }
        }
        else if (helpers.LastSetPipelineState->GraphicPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->GraphicPSO->GetRootSignature();
            auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();
            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, registerType });

            assert_format(index, "Root signature parameter doesn't exist. It either wasn't created or register/space/type aren't correctly specified.");

            switch (registerType)
            {
            case HAL::ShaderRegister::ConstantBuffer: cmdList->SetGraphicsRootConstantBuffer(*buffer.HALBuffer(), index->IndexInSignature); break;
            case HAL::ShaderRegister::ShaderResource: cmdList->SetGraphicsRootDescriptorTable(buffer.GetSRDescriptor()->GPUAddress(), index->IndexInSignature); break;
            case HAL::ShaderRegister::UnorderedAccess: cmdList->SetGraphicsRootDescriptorTable(buffer.GetUADescriptor()->GPUAddress(), index->IndexInSignature); break;
            case HAL::ShaderRegister::Sampler: assert_format(false, "Incompatible register type");
            }
        }
    }

    void RenderDevice::SetBackBuffer(Memory::Texture* backBuffer)
    {
        mBackBuffer = backBuffer;
    }

}
