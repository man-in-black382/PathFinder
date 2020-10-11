#pragma once

#include <string>
#include <comdef.h>


#include <Foundation/StringUtils.hpp>

namespace HAL 
{

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    _com_error comError{ hr__ };                                      \
    assert_format(!FAILED(hr__), comError.ErrorMessage()); \
}
#endif

}