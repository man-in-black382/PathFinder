#pragma once

#include "ResourceFormat.hpp"

#include <vector>
#include <string>
#include <d3d12.h>

namespace HAL
{
   
    class InputAssemblerLayout {
    public:
        void AddPerVertexLayoutElement(const std::string& semanticName, uint32_t semanticIndex, ResourceFormat::Color format, uint32_t inputSlot, uint32_t alighnedByteOffset);
        void AddPerInstanceLayoutElement(const std::string& semanticName, uint32_t semanticIndex, ResourceFormat::Color format, uint32_t inputSlot, uint32_t alighnedByteOffset, uint32_t stepRate);

    private:
        std::vector<std::string> mElementSemanticNames;
        std::vector<D3D12_INPUT_ELEMENT_DESC> mInputElements;
        D3D12_INPUT_LAYOUT_DESC mDesc{};

        void SetSemanticNames();

    public:
        inline const auto& D3DLayout() const { return mDesc; }
    };

}

