#pragma once

#include <string>
#include <comdef.h>

#include "../Foundation/Assert.hpp"
#include "../Foundation/StringUtils.hpp"

namespace HAL 
{

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    assert_format(!FAILED(hr__), _com_error{ hr__ }.ErrorMessage()); \
}
#endif

}