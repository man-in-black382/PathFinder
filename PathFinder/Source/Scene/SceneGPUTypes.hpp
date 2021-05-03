#pragma once

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct GPUMeshInstanceTableEntry
    {
        glm::mat4 InstanceWorldMatrix;
        // 16 byte boundary
        glm::mat4 InstancePrevWorldMatrix;
        // 16 byte boundary
        glm::mat4 InstanceNormalMatrix;
        // 16 byte boundary
        uint32_t MaterialIndex;
        uint32_t UnifiedVertexBufferOffset;
        uint32_t UnifiedIndexBufferOffset;
        uint32_t IndexCount;
        // 16 byte boundary
        uint32_t HasTangentSpace;
    };

    struct GPUMaterialTableEntry
    {
        uint32_t AlbedoMapIndex;
        uint32_t NormalMapIndex;
        uint32_t RoughnessMapIndex;
        uint32_t MetalnessMapIndex;
        // 16 byte boundary
        uint32_t DisplacementMapIndex;
        uint32_t DistanceFieldIndex;
        uint32_t LTC_LUT_MatrixInverse_Specular_Index;
        uint32_t LTC_LUT_Matrix_Specular_Index;
        // 16 byte boundary
        uint32_t LTC_LUT_Terms_Specular_Index;
        uint32_t LTC_LUT_MatrixInverse_Diffuse_Index;
        uint32_t LTC_LUT_Matrix_Diffuse_Index;
        uint32_t LTC_LUT_Terms_Diffuse_Index;
        // 16 byte boundary
        uint32_t LTC_LUT_TextureSize;
        uint32_t SamplerIndex;
        uint32_t HasNormalMap;
        float IOROverride;
        // 16 byte boundary
        glm::vec3 DiffuseAlbedoOverride;
        float RoughnessOverride;
        // 16 byte boundary
        glm::vec3 SpecularAlbedoOverride;
        float MetalnessOverride;
        // 16 byte boundary
        glm::vec3 TransmissionFilter;
        float TranslucencyOverride;
    };

    struct GPULightTableEntry
    {
        enum class LightType : uint32_t
        {
            Sphere = 0, Rectangle = 1, Disk = 2
        };

        glm::vec4 Orientation;
        // 16 byte boundary

        glm::vec4 Position;
        // 16 byte boundary

        glm::vec4 Color;
        // 16 byte boundary

        float Luminance;
        float Width;
        float Height;

        std::underlying_type_t<LightType> LightTypeRaw;
        // 16 byte boundary

        glm::mat4 ModelMatrix;
        // 16 byte boundary

        uint32_t UnifiedVertexBufferOffset;
        uint32_t UnifiedIndexBufferOffset;
        uint32_t IndexCount;
    };

    struct GPULightTablePartitionInfo
    {
        uint32_t SphericalLightsOffset = 0;
        uint32_t EllipticalLightsOffset = 0;
        uint32_t RectangularLightsOffset = 0;
        uint32_t SphericalLightsCount = 0;
        uint32_t EllipticalLightsCount = 0;
        uint32_t RectangularLightsCount = 0;
        uint32_t TotalLightsCount = 0;
        uint32_t Pad0__;
    };

    struct GPUCamera
    {
        glm::vec4 Position;
        // 16 byte boundary
        glm::mat4 View;
        glm::mat4 Projection;
        glm::mat4 ViewProjection;
        glm::mat4 InverseView;
        glm::mat4 InverseProjection;
        glm::mat4 InverseViewProjection;
        glm::mat4 Jitter;
        glm::mat4 ViewProjectionJitter;
        // 16 byte boundary
        float NearPlane = 0.0f;
        float FarPlane = 0.0f;
        float ExposureValue100 = 0.0f;
        float FoVH = 0.0f;
        // 16 byte boundary
        float FoVV = 0.0f;
        float FoVHTan = 0.0f;
        float FoVVTan = 0.0f;
        float AspectRatio = 0;
        // 16 byte boundary
        // 16 byte boundary
        glm::vec2 UVJitter;
        uint32_t Pad0__;
        uint32_t Pad1__;
    };

    struct GPUIrradianceField
    {
        glm::uvec3 GridSize;
        float CellSize;
        // 16 byte boundary
        glm::vec3 GridCornerPosition;
        uint32_t RaysPerProbe;
        // 16 byte boundary
        uint32_t TotalProbeCount;
        glm::uvec2 RayHitInfoTextureSize;
        uint32_t RayHitInfoTextureIdx;
        // 16 byte boundary
        glm::mat4 ProbeRotation;
        // 16 byte boundary
        glm::uvec2 IrradianceProbeAtlasSize;
        glm::uvec2 DepthProbeAtlasSize;
        // 16 byte boundary
        glm::uvec2 IrradianceProbeAtlasProbesPerDimension;
        glm::uvec2 DepthProbeAtlasProbesPerDimension;
        // 16 byte boundary
        uint32_t IrradianceProbeSize;
        uint32_t DepthProbeSize;
        uint32_t CurrentIrradianceProbeAtlasTexIdx;
        uint32_t CurrentDepthProbeAtlasTexIdx;
        // 16 byte boundary
        glm::ivec3 SpawnedProbePlanesCount; // How many probe planes were spawned this frame on each axis
        float DebugProbeRadius;
        // 16 byte boundary
        uint32_t PreviousIrradianceProbeAtlasTexIdx;
        uint32_t PreviousDepthProbeAtlasTexIdx;
        float IrradianceHysteresisDecrease;
        float DepthHysteresisDecrease;
    };

    using GPUInstanceIndex = uint64_t;

    enum class GPUInstanceHitGroupContribution : uint32_t
    {
        Mesh = 0, Light = 1, DebugGIProbe = 2
    };

    enum class GPUInstanceMask : uint32_t
    {
        Mesh = 1 << 0,
        Light = 1 << 1,
        DebugGIProbe = 1 << 2
    };

}
