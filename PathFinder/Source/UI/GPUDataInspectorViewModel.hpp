#pragma once

#include "ViewModel.hpp"

namespace PathFinder
{
   
    class GPUDataInspectorViewModel : public ViewModel
    {
    public:
        struct InspectionDisplayData
        {
            std::string PassName;
            std::vector<std::string> Variables;
        };

        void Import() override;

    private:
        std::vector<InspectionDisplayData> mInspectionResults;

    public:
        inline const auto& InspectionResults() const { return mInspectionResults; }
    };

}
