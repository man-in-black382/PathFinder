#include "GPUProfiler.hpp"

namespace PathFinder
{

    GPUProfiler::GPUProfiler(const HAL::Device& device, uint64_t maxEventsPerFrame, uint64_t simultaneousFramesInFlight, Memory::GPUResourceProducer* resourceProducer)
        : mQueryHeap{ device, maxEventsPerFrame * simultaneousFramesInFlight * 2, HAL::QueryHeap::QueryType::Timestamp },
        mSimultaneousFramesInFlight{ simultaneousFramesInFlight },
        mResourceProducer{ resourceProducer }
    {
        mCompletedEvents.resize(maxEventsPerFrame);
        mEventInfos.resize(maxEventsPerFrame);
    }

    GPUProfiler::EventID GPUProfiler::RecordEventStart(HAL::CommandList& cmdList, uint64_t tickFrequency)
    {
        std::lock_guard lock{ mAccessMutex };

        GPUProfiler::EventID index = mCurrentFrameEventID;

        assert_format(index < mEventInfos.size(), "Exceeded maximum per-frame event count");

        mEventInfos[index].IsStarted = true;
        mEventInfos[index].TickFrequency = tickFrequency;

        auto [start, end] = GetEventIndicesInHeap(index);
        cmdList.EndQuery(mQueryHeap, start);

        ++mCurrentFrameEventID;

        return index;
    }

    void GPUProfiler::RecordEventEnd(HAL::CommandList& cmdList, const GPUProfiler::EventID& eventId)
    {
        std::lock_guard lock{ mAccessMutex };

        assert_format(eventId < mEventInfos.size(), "Invalid event ID");
        assert_format(mEventInfos[eventId].IsStarted, "Event was not started");
        mEventInfos[eventId].IsCompleted = true;

        auto [start, end] = GetEventIndicesInHeap(eventId);
        cmdList.EndQuery(mQueryHeap, end);
    }

    void GPUProfiler::ReadbackEvents(HAL::CommandList& cmdList)
    {
        uint64_t rangeStartIdx = GetHeapStartIndexForFrameIndex(mCurrentFrameIndex);
        cmdList.ExtractQueryData(mQueryHeap, rangeStartIdx, HeapEventsPerFrameCount(), *mReadbackBuffer->HALBuffer());
    }

    void GPUProfiler::BeginFrame(uint64_t frameNumber)
    {
        if (!mReadbackBuffer)
        {
            auto properties = HAL::BufferProperties::Create<uint64_t>(HeapEventsPerFrameCount());
            mReadbackBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::AccessStrategy::DirectReadback);
            mReadbackBuffer->SetDebugName("Timestamp Queries");
        }
        
        mCurrentFrameIndex = frameNumber % mSimultaneousFramesInFlight;
    }

    void GPUProfiler::EndFrame(uint64_t frameNumber)
    {
        mReadbackBuffer->Read<uint64_t>([&](const uint64_t* ticks)
        {
            std::lock_guard lock{ mAccessMutex };

            auto requestedEventCount = mCurrentFrameEventID;
            mCurrentFrameEventID = 0;

            if (!ticks)
                return;

            for (uint64_t eventIdx = 0; eventIdx < requestedEventCount; ++eventIdx)
            {
                uint64_t eventStartIndexInHeap = eventIdx * 2;
                uint64_t eventEndIndexInHeap = eventStartIndexInHeap + 1;

                EventInfo& eventInfo = mEventInfos[eventIdx];
                assert_format(!eventInfo.IsStarted || (eventInfo.IsStarted && eventInfo.IsCompleted), "Started event was not completed");
                eventInfo.IsStarted = false;
                eventInfo.IsCompleted = false;

                Event& event = mCompletedEvents[eventIdx];
                uint64_t endTick = ticks[eventEndIndexInHeap];
                uint64_t startTick = ticks[eventStartIndexInHeap];
                event.DurationSeconds = float(endTick - startTick) / eventInfo.TickFrequency;
            }
        });
    }

    const GPUProfiler::Event& GPUProfiler::GetCompletedEvent(const GPUProfiler::EventID& eventId) const
    {
        return mCompletedEvents[eventId];
    }

    std::pair<uint64_t, uint64_t> GPUProfiler::GetEventIndicesInHeap(EventID id) const
    {
        uint64_t startEventIdxInHeap = id * 2 + GetHeapStartIndexForFrameIndex(mCurrentFrameIndex);
        return { startEventIdxInHeap, startEventIdxInHeap + 1 };
    }

    uint64_t GPUProfiler::GetHeapStartIndexForFrameIndex(uint64_t frameIndex) const
    {
        uint64_t rangeStartIdx = frameIndex * HeapEventsPerFrameCount();
        return rangeStartIdx;
    }

    uint64_t GPUProfiler::HeapEventsPerFrameCount() const
    {
        return mQueryHeap.Size() / mSimultaneousFramesInFlight;
    }

}
