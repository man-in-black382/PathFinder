#pragma once

#include <variant>

namespace Foundation {

    template<typename ...Ts>
    struct Visitor : Ts ... {
        Visitor(const Ts &... args) : Ts(args)... {}
        using Ts::operator()...;
    };

    template<typename ...Ts>
    auto MakeVisitor(Ts... lambdas) {
        return Visitor<Ts...>(lambdas...);
    }

 /*   template<
        template<class...> class AssociativeContainer,
        class Key,
        class Value
    >
        decltype(auto) Find(const AssocitiveContainer<Key, Value>& container, Key&& key)
    {
        auto iterator = container.find(std::forward<Key>(key));

        if (iterator == container.end())
        {
            return nullptr;
        }

        return &iterator->second;
    }

    template<
        template<class...> class AssociativeContainer,
        class Value,
        class Key,
        class... Keys
    >
        decltype(auto) Find(const AssociativeContainer<Key, Value>& container, Key&& key, Keys&&... keys)
    {
        auto valuePtr = Find(container, std::forward<Key>(keys));

        if (!valuePtr)
        {
            return nullptr;
        }


    }*/

}

