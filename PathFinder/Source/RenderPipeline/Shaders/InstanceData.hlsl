struct MeshInstanceData
{
    float4x4 ModelMatrix;
    float4x4 NormalMatrix;
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint MetalnessMapIndex;
    uint AOMapIndex;
    uint DisplacementMapIndex;
    uint DistanceFieldIndex;
    uint UnifiedVertexBufferOffset;
    uint UnifiedIndexBufferOffset;
    uint IndexCount;
};

static const uint LightTypeDisk = 0;
static const uint LightTypeSphere = 1;
static const uint LightTypeLine = 2;
static const uint LightTypePolygon = 3;

struct LightInstanceData
{
    float LuminousIntensity;
    float Width;
    float Height;
    float Radius;
    float4 Orientation;
    float4 Position;
    float4 Color;
    uint LightType;
};