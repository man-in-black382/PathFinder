#include "InputAssemblerLayout.hpp"

namespace HAL
{

    void InputAssemblerLayout::AddPerVertexLayoutElement(const std::string& semanticName, uint32_t semanticIndex, ColorFormat format, uint32_t inputSlot, uint32_t alighnedByteOffset)
    {
        D3D12_INPUT_ELEMENT_DESC desc{};
        desc.Format = D3DFormat(format);
        desc.AlignedByteOffset = alighnedByteOffset;
        desc.InputSlot = inputSlot;
        desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        desc.SemanticIndex = semanticIndex;
        
        mInputElements.push_back(desc);
        mElementSemanticNames.push_back(semanticName);

        mDesc.NumElements = (UINT)mInputElements.size();
        mDesc.pInputElementDescs = &mInputElements[0];

        SetSemanticNames();
    }

    void InputAssemblerLayout::AddPerInstanceLayoutElement(const std::string& semanticName, uint32_t semanticIndex, ColorFormat format, uint32_t inputSlot, uint32_t alighnedByteOffset, uint32_t stepRate)
    {
        D3D12_INPUT_ELEMENT_DESC desc{};
        desc.Format = D3DFormat(format);
        desc.AlignedByteOffset = alighnedByteOffset;
        desc.InputSlot = inputSlot;
        desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        desc.SemanticIndex = semanticIndex;
        desc.InstanceDataStepRate = stepRate;

        mInputElements.push_back(desc);
        mElementSemanticNames.push_back(semanticName);

        mDesc.NumElements = (UINT)mInputElements.size();
        mDesc.pInputElementDescs = &mInputElements[0];

        SetSemanticNames();
    }

    void InputAssemblerLayout::SetSemanticNames()
    {
        for (auto i = 0; i < mInputElements.size(); i++)
        {
            mInputElements[i].SemanticName = mElementSemanticNames[i].c_str();
        }
    }

}
