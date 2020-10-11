#pragma once

#include <HardwareAbstractionLayer/BlendState.hpp>

namespace PathFinder
{

    inline HAL::BlendState AlphaBlendingState()
    {
        HAL::BlendState state{};
        state.SetBlendingEnabled(true);
        state.SetSourceValues(HAL::BlendState::Value::SourceAlpha, HAL::BlendState::Value::InverseSourceAlpha);
        state.SetDestinationValues(HAL::BlendState::Value::InverseSourceAlpha, HAL::BlendState::Value::Zero);
        return state;
    }

}
