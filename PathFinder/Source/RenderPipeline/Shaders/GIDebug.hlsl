#ifndef _GIDebug__
#define _GIDebug__

#include "Matrix.hlsl"
#include "GIProbeHelpers.hlsl"
#include "Vertices.hlsl"

struct PassData
{
    IrradianceField ProbeField;
    int ExplicitProbeIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

//--------------------  Probe Rendering  -------------------------//
//------------------------  Vertex  ------------------------------//

struct ProbeVertexOut
{
    float4 Position : SV_POSITION;
    float3x3 NormalMatrix : NORMAL_MATRIX;
    float2 NonTransformedPosition : NON_TRANSFORMED_POSITION;
    uint ProbeIndex : PROBE_INDEX;
};

ProbeVertexOut ProbeVSMain(uint vertexId : SV_VertexID)
{
    uint vertexIndex = vertexId % 6;
    uint probeIndex = PassDataCB.ExplicitProbeIndex >= 0 ? PassDataCB.ExplicitProbeIndex : vertexId / 6;

    float probeRadius = PassDataCB.ProbeField.DebugProbeRadius;
    float3 probePosition = ProbePositionFrom3DIndex(Probe3DIndexFrom1D(probeIndex, PassDataCB.ProbeField), PassDataCB.ProbeField);
    float3 billboardToCamera = normalize(FrameDataCB.CurrentFrameCamera.Position.xyz - probePosition);
    float4x4 billboardRotation = RotationMatrix4x4(billboardToCamera, GetUpVectorForOrientaion(billboardToCamera));

    float4x4 modelMat = Matrix4x4Identity;

    // Scale first
    modelMat[0][0] = probeRadius;
    modelMat[1][1] = probeRadius;

    // Then rotate
    modelMat = mul(billboardRotation, modelMat);

    // Then translate
    float4x4 translationMat = Matrix4x4Identity;
    translationMat[0][3] = probePosition.x;
    translationMat[1][3] = probePosition.y;
    translationMat[2][3] = probePosition.z;

    modelMat = mul(translationMat, modelMat);

    float2 normVertex = UnitQuadVertices[UnitQuadIndices[vertexIndex]] * 2.0; // Expand to -1 to 1 range
    float4 vertex = float4(normVertex, 0, 1);

    vertex = mul(modelMat, vertex);
    vertex = mul(FrameDataCB.CurrentFrameCamera.ViewProjection, vertex);

    ProbeVertexOut output;
    output.Position = vertex;
    output.NormalMatrix = (float3x3)billboardRotation;
    output.NonTransformedPosition = normVertex;
    output.ProbeIndex = probeIndex;

    return output;
}

//------------------------  Pixel  ------------------------------//

float4 ProbePSMain(ProbeVertexOut pin) : SV_Target0
{
    float2 dirOnBillboardPlane = pin.NonTransformedPosition;
    float normDistFromCenter = length(dirOnBillboardPlane);

    // Make a nice round circle out of a quad
    if (normDistFromCenter > 1.0)
        discard;

    // Knowing that our billboard represents a sphere we can reconstruct the z coordinate
    // Also negate Z because the equation gives positive values for Z pointing "from the screen to the viewer"
    // which is the opposite of the positive Z direction in NDC space
    float z = sqrt(1.0 - normDistFromCenter * normDistFromCenter);

    // Calculate the normal for the sphere
    float3 normal = normalize(float3(dirOnBillboardPlane, z));

    // Rotate to world space
    normal = mul(pin.NormalMatrix, normal);

    uint adjustedProbeIndex = UInt4_Textures2D[PassDataCB.ProbeField.IndirectionTableTexIdx][uint2(pin.ProbeIndex, 0)].r;
    float2 atlasUV = IrradianceProbeAtlasUV(adjustedProbeIndex, normal, PassDataCB.ProbeField);
    Texture2D atlas = Textures2D[PassDataCB.ProbeField.IrradianceProbeAtlasTexIdx];
    float3 irradiance = atlas.SampleLevel(LinearClampSampler(), atlasUV, 0).rgb;
    irradiance = DecodeProbeIrradiance(irradiance);

    return float4(irradiance, 1.0);
}

//-------------------  Probe Rays Rendering  ---------------------//
//------------------------  Vertex  ------------------------------//

struct RayVertexOut
{
    float4 Position : SV_POSITION;
    float3 Color : COLOR0;
};

RayVertexOut RaysVSMain(uint vertexId : SV_VertexID)
{
    uint rayIndex = vertexId / 36;
    uint vertexIndex = vertexId % 36;
    uint probeIndex = PassDataCB.ExplicitProbeIndex;
    float probeRadius = PassDataCB.ProbeField.DebugProbeRadius;
    float3 probePosition = ProbePositionFrom3DIndex(Probe3DIndexFrom1D(probeIndex, PassDataCB.ProbeField), PassDataCB.ProbeField);
    float3 rayDirection = ProbeSamplingVector(rayIndex, PassDataCB.ProbeField);
    float4x4 vertexRotation = RotationMatrix4x4(rayDirection, GetUpVectorForOrientaion(rayDirection));

    Texture2D rayInfoTexture = Textures2D[PassDataCB.ProbeField.RayHitInfoTextureIdx];
    float4 rayInfo = rayInfoTexture[RayHitTexelIndex(rayIndex, probeIndex, PassDataCB.ProbeField)];
    float3 radiance = rayInfo.rgb;
    float hitDistance = rayInfo.w;

    float4x4 modelMat = Matrix4x4Identity;

    // Scale first. We're making thin long boxes here to represent rays.
    float2 xyScale = probeRadius * 0.04;
    modelMat[0][0] = xyScale.x;
    modelMat[1][1] = xyScale.y;
    // Make the box as long as the hit distance on the Z axis
    modelMat[2][2] = hitDistance < 0.0 ? 0.0 : hitDistance;

    // Then rotate to match the ray direction
    modelMat = mul(vertexRotation, modelMat);

    // Then translate to start from the probe position
    float4x4 translationMat = Matrix4x4Identity;
    translationMat[0][3] = probePosition.x;
    translationMat[1][3] = probePosition.y;
    translationMat[2][3] = probePosition.z;

    modelMat = mul(translationMat, modelMat);

    float4 vertex = float4(UnitCubeVertices[UnitCubeIndices[vertexIndex]] + float3(0.0, 0.0, 0.5), 1); // Make cube's Z plane be at 0

    vertex = mul(modelMat, vertex);
    vertex = mul(FrameDataCB.CurrentFrameCamera.ViewProjection, vertex);

    RayVertexOut output;
    output.Position = vertex;
    output.Color = radiance;

    return output;
}

//------------------------  Pixel  ------------------------------//

float4 RaysPSMain(RayVertexOut pin) : SV_Target0
{
    return float4(pin.Color, 1.0);
}

#endif