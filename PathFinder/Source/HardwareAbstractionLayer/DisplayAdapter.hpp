#pragma once

#include <dxgi.h>
#include <wrl.h>

class DisplayAdapter
{
public:
    DisplayAdapter(const DXGI_ADAPTER_DESC& desc);
    ~DisplayAdapter();

private:
    DXGI_ADAPTER_DESC mDescription;
};

