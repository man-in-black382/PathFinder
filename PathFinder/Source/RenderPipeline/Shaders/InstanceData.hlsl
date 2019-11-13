struct InstanceData
{
    float4x4 ModelMatrix;
    float4x4 NormalMatrix;
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint MetalnessMapIndex;
    uint AOMapIndex;
    uint DisplacementMapIndex;
    uint UnifiedVertexBufferOffset;
    uint UnifiedIndexBufferOffset;
    uint IndexCount;
};