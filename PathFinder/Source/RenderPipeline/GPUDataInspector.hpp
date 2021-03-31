#pragma once

#include <Memory/GPUResourceProducer.hpp>
#include "RenderPassGraph.hpp"

#include <glm/vec4.hpp>
#include <robinhood/robin_hood.h>

namespace PathFinder
{

    class GPUDataInspector
    {
    public:
        using Variable = std::variant<float, glm::vec2, glm::vec3, glm::vec4>;
        using InspectionData = std::vector<Variable>;

        void PreparePerPassBuffers(const RenderPassGraph* passGraph, Memory::GPUResourceProducer* resourceProducer);
        void DecodeAvailableInspectionData();
        const InspectionData& InspectionDataForPass(const RenderPassGraph::Node& passNode) const;
        const Memory::Buffer* BufferForPass(const RenderPassGraph::Node& passNode) const;
        Memory::Buffer* BufferForPass(const RenderPassGraph::Node& passNode);

    private:
        enum class GPUDataType : uint32_t
        {
            Float = 0, Float2 = 1, Float3 = 2, Float4 = 3
        };

        std::vector<Memory::GPUResourceProducer::BufferPtr> mBuffers;
        std::vector<InspectionData> mInspectionResults;
        InspectionData mEmptyInspectionData;
    };

}
