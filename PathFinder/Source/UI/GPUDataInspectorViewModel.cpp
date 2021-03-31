#include "GPUDataInspectorViewModel.hpp"
#include <glm/gtx/string_cast.hpp>
#include <Foundation/Visitor.hpp>

namespace PathFinder
{

    void GPUDataInspectorViewModel::Import()
    {
        mInspectionResults.clear();

        for (const RenderPassGraph::Node* node : Dependencies->RenderEngine->RenderGraph()->NodesInGlobalExecutionOrder())
        {
            auto& inspectionData = Dependencies->RenderEngine->GPUInspector()->InspectionDataForPass(*node);

            if (inspectionData.empty())
                continue;

            InspectionDisplayData& displayData = mInspectionResults.emplace_back();
            displayData.PassName = node->PassMetadata().Name.ToString();

            for (const GPUDataInspector::Variable& variable : inspectionData)
            {
                std::visit(Foundation::MakeVisitor(
                    [&displayData](const float& value) {
                        displayData.Variables.push_back(std::to_string(value));
                    },
                    [&displayData](const glm::vec2& value) {
                        displayData.Variables.push_back(std::to_string(value.x) + " " + std::to_string(value.y));
                    },
                        [&displayData](const glm::vec3& value) {
                        displayData.Variables.push_back(std::to_string(value.x) + " " + std::to_string(value.y) + " " + std::to_string(value.z));
                    },
                        [&displayData](const glm::vec4& value) {
                        displayData.Variables.push_back(
                            std::to_string(value.x) + " " + std::to_string(value.y) + " " +
                            std::to_string(value.z) + " " + std::to_string(value.w));
                    }),
                    variable);
            }
        }
    }

}
