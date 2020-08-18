#pragma once

#include <type_traits>

// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/

template<typename Enum>
struct EnableBitMaskOperators {
    static const bool enable = false;
};

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator|(Enum lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
            static_cast<underlying>(lhs) |
                    static_cast<underlying>(rhs)
    );
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator&(Enum lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
            static_cast<underlying>(lhs) &
                    static_cast<underlying>(rhs)
    );
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator^(Enum lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
            static_cast<underlying>(lhs) ^
                    static_cast<underlying>(rhs)
    );
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator~(Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
            ~static_cast<underlying>(rhs)
    );
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum &>::type
operator|=(Enum &lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum> (
            static_cast<underlying>(lhs) |
                    static_cast<underlying>(rhs)
    );
    return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum &>::type
operator&=(Enum &lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum> (
            static_cast<underlying>(lhs) &
                    static_cast<underlying>(rhs)
    );
    return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum &>::type
operator^=(Enum &lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum> (
            static_cast<underlying>(lhs) ^
                    static_cast<underlying>(rhs)
    );
    return lhs;
}

#define ENABLE_BITMASK_OPERATORS(x)  \
template<>                           \
struct EnableBitMaskOperators<x>     \
{                                    \
static const bool enable = true; \
}; \
\
inline bool EnumMaskEquals(x mask, x component) \
{ \
    return (mask & component) == component; \
} \
inline bool EnumMaskContains(x mask, x component) \
{ \
    return std::underlying_type_t<x>(mask & component) != 0; \
} 
