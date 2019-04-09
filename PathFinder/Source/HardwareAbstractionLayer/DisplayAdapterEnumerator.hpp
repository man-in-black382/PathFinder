#pragma once

#include "DisplayAdapter.hpp"

#include <dxgi.h>
#include <array>

class DisplayAdapterEnumerator
{
public:
    DisplayAdapterEnumerator();
    ~DisplayAdapterEnumerator();

    std::array<DisplayAdapter> Enumerate() const;

private:

};

