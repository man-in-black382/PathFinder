#ifndef _GIProbeIndirectionTableUpdate__
#define _GIProbeIndirectionTableUpdate__

#include "GIProbeHelpers.hlsl"

struct PassData
{
    IrradianceField ProbeField;
    bool ShouldInitialize;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

[numthreads(64, 1, 1)]
void CSMain(uint3 dtID : SV_DispatchThreadID)  
{
    RWTexture2D<uint4> indirectionTable = RW_UInt4_Textures2D[PassDataCB.ProbeField.IndirectionTableTexIdx];

    // If probe grid moved, then offset 3D index by the number of probe planes spawned on each world axis

    uint localProbeIndex = dtID.x;
    uint previousIndirectionIndex = indirectionTable[uint2(localProbeIndex, 0)].r;
    int3 probe3DIndex = Probe3DIndexFrom1D(previousIndirectionIndex, PassDataCB.ProbeField);
    int3 new3DIndex = probe3DIndex + PassDataCB.ProbeField.SpawnedProbePlanesCount;

    // Wrap indices around
    for (uint i = 0; i < 3; ++i) 
    {
        if (new3DIndex[i] < 0)
            new3DIndex[i] = PassDataCB.ProbeField.GridSize[i] - (abs(new3DIndex[i]) % PassDataCB.ProbeField.GridSize[i]);

        if (new3DIndex[i] >= PassDataCB.ProbeField.GridSize[i])
            new3DIndex[i] = new3DIndex[i] % PassDataCB.ProbeField.GridSize[i];
    }

    // If initialization is requested, just store local probe indices
    uint newIndirectionIndex = PassDataCB.ShouldInitialize ? localProbeIndex : Probe1DIndexFrom3D(new3DIndex, PassDataCB.ProbeField);
    indirectionTable[uint2(localProbeIndex, 0)].r = newIndirectionIndex;
}

#endif