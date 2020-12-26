#pragma once

#include <Memory/GPUResourceProducer.hpp>
#include <HardwareAbstractionLayer/QueryHeap.hpp>
#include <HardwareAbstractionLayer/CommandList.hpp>

#include <mutex>

namespace PathFinder
{

    class GPUProfiler
    {
    public:
        using EventID = uint64_t;

        struct Event
        {
            float DurationSeconds;
        };

        GPUProfiler(const HAL::Device& device, uint64_t maxEventsPerFrame, uint64_t simultaneousFramesInFlight, Memory::GPUResourceProducer* resourceProducer);

        EventID RecordEventStart(HAL::CommandList& cmdList, uint64_t tickFrequency);
        void RecordEventEnd(HAL::CommandList& cmdList, const GPUProfiler::EventID& eventId);
        void ReadbackEvents(HAL::CommandList& cmdList);
        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);
        const Event& GetCompletedEvent(const GPUProfiler::EventID& eventId) const;

    private:
        struct EventInfo
        {
            bool IsStarted = false;
            bool IsCompleted = false;
            uint64_t TickFrequency = 1;
        };

        std::pair<uint64_t, uint64_t> GetEventIndicesInHeap(EventID id) const;
        uint64_t GetHeapStartIndexForFrameIndex(uint64_t frameIndex) const;
        uint64_t HeapEventsPerFrameCount() const;

        Memory::GPUResourceProducer* mResourceProducer;
        HAL::QueryHeap mQueryHeap;
        Memory::GPUResourceProducer::BufferPtr mReadbackBuffer;
        std::vector<Event> mCompletedEvents;
        std::vector<EventInfo> mEventInfos;
        uint64_t mSimultaneousFramesInFlight = 1;
        uint64_t mCurrentFrameIndex = 0;
        EventID mCurrentFrameEventID = 0;
        std::mutex mAccessMutex;
    };

}
