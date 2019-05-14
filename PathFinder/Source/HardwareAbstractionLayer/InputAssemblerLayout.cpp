#include "InputAssemblerLayout.hpp"

namespace HAL
{

	void InputAssemblerLayout::AddPerVertexLayoutElement(const std::string& semanticName, uint32_t semanticIndex, ResourceFormat::Color format, uint32_t inputSlot, uint32_t alighnedByteOffset)
	{
        D3D12_INPUT_ELEMENT_DESC desc{};
		desc.Format = ResourceFormat::D3DFormat(format);
		desc.AlignedByteOffset = alighnedByteOffset;
		desc.InputSlot = inputSlot;
		desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		desc.SemanticName = semanticName.c_str();
		desc.SemanticIndex = semanticIndex;
		
		mInputElements.push_back(desc);

		mDesc.NumElements = mInputElements.size();
		mDesc.pInputElementDescs = &mInputElements[0];
	}

	void InputAssemblerLayout::AddPerInstanceLayoutElement(const std::string& semanticName, uint32_t semanticIndex, ResourceFormat::Color format, uint32_t inputSlot, uint32_t alighnedByteOffset, uint32_t stepRate)
	{
        D3D12_INPUT_ELEMENT_DESC desc{};
		desc.Format = ResourceFormat::D3DFormat(format);
		desc.AlignedByteOffset = alighnedByteOffset;
		desc.InputSlot = inputSlot;
		desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		desc.SemanticName = semanticName.c_str();
		desc.SemanticIndex = semanticIndex;
		desc.InstanceDataStepRate = stepRate;

		mInputElements.push_back(desc);

		mDesc.NumElements = mInputElements.size();
		mDesc.pInputElementDescs = &mInputElements[0];
	}

}
