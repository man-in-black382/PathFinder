#ifndef _GIProbeBorderUpdate__
#define _GIProbeBorderUpdate__

#include "GIProbeHelpers.hlsl"

struct PassData
{
    IlluminanceField ProbeField;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

// Group processes 16 probes, 4 corners each
static const uint CornerUpdateGroupSizeX = 16;
static const uint CornerUpdateGroupSizeY = 4;

void GetLocalTexelIndexForCorner(uint cornerIndex, uint probeSize, inout int2 localTexelIndex, inout int2 cornerOffset)
{
    bool isTopRow = cornerIndex == 0 || cornerIndex == 1;
    bool isLeftColumn = cornerIndex == 0 || cornerIndex == 2;

    localTexelIndex = int2(isLeftColumn ? 0 : probeSize - 1, isTopRow ? 0 : probeSize - 1);
    cornerOffset = int2(isLeftColumn ? probeSize : -probeSize, isTopRow ? probeSize : -probeSize); // Propagate to *opposite* side
}

[numthreads(CornerUpdateGroupSizeX, CornerUpdateGroupSizeY, 1)]
void IlluminanceCornerUpdateCSMain(uint3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    uint probeIndex = dtID.x;
    uint cornerIndex = dtID.y;

    if (probeIndex >= PassDataCB.ProbeField.TotalProbeCount)
        return;

    int2 localTexelIndex, cornerOffset;
    GetLocalTexelIndexForCorner(cornerIndex, PassDataCB.ProbeField.IlluminanceProbeSize, localTexelIndex, cornerOffset);

    uint2 atlasTexelIndex = IlluminanceProbeAtlasTexelIndex(probeIndex, localTexelIndex, PassDataCB.ProbeField);
    RWTexture2D<float4> atlas = RW_Float4_Textures2D[PassDataCB.ProbeField.CurrentIlluminanceProbeAtlasTexIdx];

    atlas[atlasTexelIndex + cornerOffset] = atlas[atlasTexelIndex];
}

[numthreads(CornerUpdateGroupSizeX, CornerUpdateGroupSizeY, 1)]
void DepthCornerUpdateCSMain(uint3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    uint probeIndex = dtID.x;
    uint cornerIndex = dtID.y;

    if (probeIndex >= PassDataCB.ProbeField.TotalProbeCount)
        return;

    int2 localTexelIndex, cornerOffset;
    GetLocalTexelIndexForCorner(cornerIndex, PassDataCB.ProbeField.DepthProbeSize, localTexelIndex, cornerOffset);

    uint2 atlasTexelIndex = DepthProbeAtlasTexelIndex(probeIndex, localTexelIndex, PassDataCB.ProbeField);
    RWTexture2D<float4> atlas = RW_Float4_Textures2D[PassDataCB.ProbeField.CurrentDepthProbeAtlasTexIdx];

    atlas[atlasTexelIndex + cornerOffset] = atlas[atlasTexelIndex];
}

static const uint IlluminanceBorderUpdateGroupSizeX = 8; // *Must* match irradiance probe dimension size
static const uint DepthBorderUpdateGroupSizeX = 16; // *Must* match depth probe dimension size
static const uint BorderUpdateGroupSizeY = 4; // 4 borders in each case

void GetLocalTexelIndexForBorder(uint borderIndex, int borderTexelIndex, uint probeSize, inout int2 localTexelIndex, inout int2 offsetToBorder)
{
    // Borders are indexed as follows: 0 - left border, 1 - top, 2 - right, 3 - bottom
    // We now swizzle coordinates depending on whether border is horizontal or vertical
    // Swizzling pattern is described in Nvidia's paper.

    uint halfProbeSize = probeSize / 2;

    switch (borderIndex)
    {
    case 0:
        localTexelIndex = int2(0, borderTexelIndex);
        offsetToBorder = int2(-1, (probeSize - 1 - borderTexelIndex) - borderTexelIndex);
        break;

    case 1:
        localTexelIndex = int2(borderTexelIndex, 0);
        offsetToBorder = int2((probeSize - 1 - borderTexelIndex) - borderTexelIndex, -1);
        break;

    case 2:
        localTexelIndex = int2(probeSize - 1, borderTexelIndex);
        offsetToBorder = int2(1, (probeSize - 1 - borderTexelIndex) - borderTexelIndex);
        break;

    case 3:
        localTexelIndex = int2(borderTexelIndex, probeSize - 1);
        offsetToBorder = int2((probeSize - 1 - borderTexelIndex) - borderTexelIndex, 1);
        break;

    default:
        localTexelIndex = 0;
        offsetToBorder = 0;
    }
}

[numthreads(IlluminanceBorderUpdateGroupSizeX, BorderUpdateGroupSizeY, 1)]
void IlluminanceBorderUpdateCSMain(int3 gtID : SV_GroupThreadID, int3 dtID : SV_DispatchThreadID)
{
    int probeIndex = dtID.x / IlluminanceBorderUpdateGroupSizeX;

    if (probeIndex >= PassDataCB.ProbeField.TotalProbeCount)
        return;

    int2 localTexelIndex = 0, texelOffsetToBorder = 0;
    GetLocalTexelIndexForBorder(dtID.y, gtID.x, PassDataCB.ProbeField.IlluminanceProbeSize, localTexelIndex, texelOffsetToBorder);

    int2 atlasTexelIndex = IlluminanceProbeAtlasTexelIndex(probeIndex, localTexelIndex, PassDataCB.ProbeField);
    RWTexture2D<float4> atlas = RW_Float4_Textures2D[PassDataCB.ProbeField.CurrentIlluminanceProbeAtlasTexIdx];

    atlas[atlasTexelIndex + texelOffsetToBorder] = atlas[atlasTexelIndex];
}

[numthreads(DepthBorderUpdateGroupSizeX, BorderUpdateGroupSizeY, 1)]
void DepthBorderUpdateCSMain(int3 gtID : SV_GroupThreadID, int3 dtID : SV_DispatchThreadID)
{
    int probeIndex = dtID.x / DepthBorderUpdateGroupSizeX;

    if (probeIndex >= PassDataCB.ProbeField.TotalProbeCount)
        return;

    int2 localTexelIndex = 0, texelOffsetToBorder = 0;
    GetLocalTexelIndexForBorder(dtID.y, gtID.x, PassDataCB.ProbeField.DepthProbeSize, localTexelIndex, texelOffsetToBorder);

    int2 atlasTexelIndex = DepthProbeAtlasTexelIndex(probeIndex, localTexelIndex, PassDataCB.ProbeField);
    RWTexture2D<float4> atlas = RW_Float4_Textures2D[PassDataCB.ProbeField.CurrentDepthProbeAtlasTexIdx];

    atlas[atlasTexelIndex + texelOffsetToBorder] = atlas[atlasTexelIndex];
}

#endif