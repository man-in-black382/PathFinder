#include "GPUDataInspector.hpp"

namespace PathFinder
{

    void GPUDataInspector::PreparePerPassBuffers(const RenderPassGraph* passGraph, Memory::GPUResourceProducer* resourceProducer)
    {
        auto debugBufferProperties = HAL::BufferProperties::Create(512, 1, HAL::ResourceState::UnorderedAccess, HAL::ResourceState::CopySource);

        int32_t buffersToAllocate = passGraph->NodesInGlobalExecutionOrder().size() - mBuffers.size();
        buffersToAllocate = std::max(0, buffersToAllocate);

        for (auto i = 0; i < buffersToAllocate; ++i)
        {
            mBuffers.emplace_back(resourceProducer->NewBuffer(debugBufferProperties));
            mBuffers.back()->SetDebugName(StringFormat("GPUDataInspector Buffer [%d]", mBuffers.size() - 1));
        }

        for (auto& buffer : mBuffers)
        {
            buffer->RequestWrite();
            float zero = 0.0f;
            buffer->Write<float>(&zero, 0, 1);
        }
    }

    void GPUDataInspector::DecodeAvailableInspectionData()
    {
        mInspectionResults.clear();

        for (auto& buffer : mBuffers)
        {
            buffer->Read<float>([&](const float* rawData)
            {
                if (!rawData)
                    return;

                InspectionData& inspectionData = mInspectionResults.emplace_back();

                uint32_t totalVariableCount = *rawData;
                rawData++;

                for (auto varIdx = 0u; varIdx < totalVariableCount; ++varIdx)
                {
                    Variable& variable = inspectionData.emplace_back();
                    GPUDataType dataType{ (uint32_t)(*rawData) };
                    rawData++;

                    switch (dataType)
                    {
                    case GPUDataType::Float:
                        variable = *rawData;
                        rawData += 1;
                        break;
                    case GPUDataType::Float2:
                        variable = glm::vec2{ *(rawData + 0), *(rawData + 1) };
                        rawData += 2;
                        break;
                    case GPUDataType::Float3:
                        variable = glm::vec3{ *(rawData + 0), *(rawData + 1), *(rawData + 2) };
                        rawData += 3;
                        break;
                    case GPUDataType::Float4:
                        variable = glm::vec4{ *(rawData + 0), *(rawData + 1), *(rawData + 2), *(rawData + 3) };
                        rawData += 4;
                        break;
                    default:
                        break;
                    }
                }
            });
        }
    }

    const GPUDataInspector::InspectionData& GPUDataInspector::InspectionDataForPass(const RenderPassGraph::Node& passNode) const
    {
        return passNode.GlobalExecutionIndex() >= mInspectionResults.size() ?
            mEmptyInspectionData : mInspectionResults[passNode.GlobalExecutionIndex()];
    }

    const Memory::Buffer* GPUDataInspector::BufferForPass(const RenderPassGraph::Node& passNode) const
    {
        return passNode.GlobalExecutionIndex() >= mBuffers.size() ?
            nullptr : mBuffers[passNode.GlobalExecutionIndex()].get();
    }

    Memory::Buffer* GPUDataInspector::BufferForPass(const RenderPassGraph::Node& passNode)
    {
        return passNode.GlobalExecutionIndex() >= mBuffers.size() ?
            nullptr : mBuffers[passNode.GlobalExecutionIndex()].get();
    }

}
