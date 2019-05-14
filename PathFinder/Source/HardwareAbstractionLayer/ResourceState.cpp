#include "ResourceState.hpp"
#include "Utils.h"

namespace HAL
{

    CommonResourceStateT CommonResourceState;
    PixelShaderAccessResourceStateT PixelShaderAccessResourceState;
    NonPixelShaderAccessResourceStateT NonPixelShaderAccessResourceState;
    UnorderedAccessResourceStateT UnorderedAccessResourceState;
    GenericReadResourceStateT GenericReadResourceState;
    CopySourceResourceStateT CopySourceResourceState;
    CopyDesctinationResourceStateT CopyDesctinationResourceState;
    VertexAndConstantResourceStateT VertexAndConstantResourceState;
    IndexResourceStateT IndexResourceState;
    PresentTextureResourceStateT PresentResourceState;
    RenderTargetTextureResourceStateT RenderTargetResourceState;
    DepthReadResourceStateT DepthReadResourceState;
    DepthWriteResourceStateT DepthWriteResourceState;

}

