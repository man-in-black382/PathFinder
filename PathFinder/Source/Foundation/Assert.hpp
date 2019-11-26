#pragma once

#include <sstream>
#include <cassert>

#include <windows.h>

template< typename... Args >
inline void print_assertion(Args&&... args)
{
    std::stringstream ss;
    ss.precision(10);
    ss << std::endl;
    //if constexpr (debug_mode)
    //{
    (ss << ... << args) << std::endl;
    //}
    OutputDebugString(ss.str().c_str()); 
    abort();
}

#ifdef assert_format
#undef assert_format
#endif
#define assert_format(EXPRESSION, ... ) ((EXPRESSION) ? (void)0 : print_assertion(\
        "Error: ", \
        #EXPRESSION, \
        " in File: ", \
        __FILE__, \
        " in Line: ", \
        __LINE__, \
        " \n",\
        __VA_ARGS__))

