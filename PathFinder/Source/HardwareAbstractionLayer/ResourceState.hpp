#pragma once

#include <wrl.h>
#include <d3d12.h>

namespace HAL
{
	enum class TextureResourceState { RenderTarget, ResolveDesctination, ResolveSource };

	enum class DepthStencilTextureResourceState { Read, Write };

	enum class BufferResourceState { VertexAndConstant, Index };

	enum class ResourceState { 
		Common, UnorderedAccess, PixelShaderAccess, NonPixelShaderAccess, 
		StreamOut, IndirectArgument, CopyDestination, CopySource,
		GenericRead, RaytracingAccelerationStructure, ShadingRateSource, Predication 
	};
}

