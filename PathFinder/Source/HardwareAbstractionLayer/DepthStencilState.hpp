#pragma once

#include <d3d12.h>

namespace HAL
{

    class DepthStencilState {
    public:
        enum class ComparisonFunction {
            Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always
        };

        void SetDepthTestEnabled(bool enabled);
        void SetDepthWriteEnabled(bool enabled);
        void SetComparisonFunction(ComparisonFunction function);

    private:
        D3D12_DEPTH_STENCIL_DESC mDesc{};

    public:
        inline const auto& D3DState() const { return mDesc; }
    };

}

