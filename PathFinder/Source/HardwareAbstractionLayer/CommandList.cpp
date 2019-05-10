#include "CommandList.hpp"
#include "Utils.h"

namespace HAL
{

    CommandList::CommandList(const Device& device, const DirectCommandAllocator& allocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ThrowIfFailed(device.D3DPtr()->CreateCommandList(0, type, allocator.D3DPtr(), nullptr, IID_PPV_ARGS(&mList)));
        mList->Close();
    }

    CommandList::~CommandList() {}

    void CommandList::SetViewport(const Viewport& viewport)
    {
        auto d3dViewport = viewport.D3DViewport();
        mList->RSSetViewports(1, &d3dViewport);
    }

    void CommandList::SetPipelineState(const GraphicsPipelineState& state)
    {
        mList->SetPipelineState(state.D3DState());
        mList->SetGraphicsRootSignature(state.AssosiatedRootSignature().D3DSignature());
    }

    DirectCommandList::DirectCommandList(const Device& device, const DirectCommandAllocator& allocator)
        : CommandList(device, allocator, D3D12_COMMAND_LIST_TYPE_DIRECT)
    {

    }

}