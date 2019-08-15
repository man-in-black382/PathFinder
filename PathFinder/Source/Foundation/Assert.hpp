#pragma once

#include <iostream>
#include <cassert>

template< typename... Args >
inline void print_assertion(std::ostream& out, Args&&... args)
{
    out.precision(10);
    //if constexpr (debug_mode)
    //{
    (out << ... << args) << std::endl;
    //}
    abort();
}

#ifdef assert_format
#undef assert_format
#endif
#define assert_format(EXPRESSION, ... ) ((EXPRESSION) ? (void)0 : print_assertion(std::cerr, \
        "Assertion failure: ", \
        #EXPRESSION, \
        " in File: ", \
        __FILE__, \
        " in Line: ", \
        __LINE__, __VA_ARGS__))

