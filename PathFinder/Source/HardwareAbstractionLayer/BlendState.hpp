#pragma once

#include <cstdint>
#include <d3d12.h>

namespace HAL
{
   
	class BlendState {
	public:
		enum class Value { Original, Inverse, Zero, One };

		enum class Function { Addition, Substraction, ReverseSubstraction, Min, Max };
		
		enum class RenderTarget {
			RT0 = 0, RT1 = 1, RT2 = 2, RT3 = 3,
			RT4 = 4, RT5 = 5, RT6 = 6, RT7 = 7
		};

		void SetSourceValues(Value color, Value alpha, RenderTarget renderTarget = RenderTarget::RT0);
		void SetDestinationValues(Value color, Value alpha, RenderTarget renderTarget = RenderTarget::RT0);
		void SetFunctions(Function colorFunction, Function alphaFunction, RenderTarget renderTarget = RenderTarget::RT0);
		void SetBlendingEnabled(bool enabled, RenderTarget renderTarget = RenderTarget::RT0);

	private:
		D3D12_BLEND_DESC mDesc;

	public:
		inline const auto& D3DState() const { return mDesc; }
	};

}

