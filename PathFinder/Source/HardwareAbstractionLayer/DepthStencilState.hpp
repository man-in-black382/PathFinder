#pragma once

#include <d3d12.h>

namespace HAL
{

	class DepthStencilState {
	public:
		enum class ComparisonFunction {
			Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always
		};

		DepthStencilState(bool depthTestEnabled, bool depthWriteEnabled, ComparisonFunction depthComparisonFunction);

	private:
		D3D12_DEPTH_STENCIL_DESC mDesc;

	public:
		inline const auto& D3DState() { return mDesc; }
	};

}

