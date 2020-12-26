#include <aftermath/AftermathHelpers.hpp>

namespace PathFinder
{

    template <class Lambda>
    void RenderDevice::RecordWorkerCommandList(const RenderPassGraph::Node& passNode, const Lambda& action)
    {
        HAL::ComputeCommandListBase* worker = GetComputeCommandListBase(mPassCommandLists[passNode.GlobalExecutionIndex()].WorkCommandList);
        worker->Reset();

        const std::string& passName = passNode.PassMetadata().Name.ToString();
        mEventTracker.StartGPUEvent(passName, *worker);

        uint64_t tickFrequency = GetCommandQueue(passNode.ExecutionQueueIndex).GetTimestampFrequency();
        GPUProfiler::EventID profilerEventID = mGPUProfiler->RecordEventStart(*worker, tickFrequency);
        PipelineMeasurement& measurement = mMeasurements[passNode.GlobalExecutionIndex()];
        measurement.Name = passName;
        measurement.ProfilerEventID = profilerEventID;

        if (worker->AftermathHandle())
        {
            AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_SetEventMarker(*worker->AftermathHandle(), passName.c_str(), passName.size() + 1));
        }

        worker->SetDescriptorHeaps(mDescriptorAllocator->CBSRUADescriptorHeap(), mDescriptorAllocator->SamplerDescriptorHeap());
        action();
        mEventTracker.EndGPUEvent(*worker);
        mGPUProfiler->RecordEventEnd(*worker, profilerEventID);
        worker->Close();
    }

    template <class CommandQueueT, class CommandListT>
    void RenderDevice::ExecuteCommandListBatch(CommandListBatch& batch, HAL::CommandQueue& queue)
    {
        std::vector<CommandListT*> commandLists;
        CommandQueueT* concreteQueue = static_cast<CommandQueueT*>(&queue);

        for (auto cmdListIdx = 0; cmdListIdx < batch.CommandLists.size(); ++cmdListIdx)
        {
            HALCommandListPtrVariant& cmdListVariant = batch.CommandLists[cmdListIdx];
            commandLists.push_back(std::get<CommandListT*>(cmdListVariant));
        }

        concreteQueue->ExecuteCommandLists(commandLists.data(), commandLists.size());
    }

}

