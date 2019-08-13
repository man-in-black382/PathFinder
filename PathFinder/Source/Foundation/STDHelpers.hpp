#pragma once

#include <variant>

namespace Foundation
{

    template<typename ...Ts>
    struct Visitor : Ts ... {
        Visitor(const Ts &... args) : Ts(args)... {}
        using Ts::operator()...;
    };

    template<typename ...Ts>
    auto MakeVisitor(Ts... lambdas) {
        return Visitor<Ts...>(lambdas...);
    }

}

