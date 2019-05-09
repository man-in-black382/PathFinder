#pragma once

#include <cstdint>
#include <d3d12.h>

namespace HAL
{
   
	class RasterizerState {
	public:
		RasterizerState() = default;

	private:
		D3D12_RASTERIZER_DESC mDesc;

	public:
		inline const auto& D3DState() const { return mDesc; }
	};

}

