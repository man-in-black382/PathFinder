#ifndef _GIProbeIndirectionTableUpdate__
#define _GIProbeIndirectionTableUpdate__

#include "GIProbeHelpers.hlsl"

struct PassData
{
    IrradianceField ProbeField;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

[numthreads(64, 1, 1)]
void CSMain(uint3 dtID : SV_DispatchThreadID)  
{
    uint probeIndex = dtID.x;
    uint3 probe3DIndex = Probe3DIndexFrom1D(probeIndex, PassDataCB.ProbeField);
    float3 probePosition = ProbePositionFrom3DIndex(probe3DIndex, PassDataCB.ProbeField);

    
}

#endif