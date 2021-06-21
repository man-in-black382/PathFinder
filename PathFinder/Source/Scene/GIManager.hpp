#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <Geometry/Dimensions.hpp>

namespace PathFinder
{
    class Scene;

    class IlluminanceField
    {
    public:
        void GenerateProbeRotation(const glm::vec2& random0to1);
        void SetCornerPosition(const glm::vec3& position);
        void SetDebugProbeRadius(float radius);
        void SetIlluminanceHysteresisDecrease(float decrease);
        void SetDepthHysteresisDecrease(float decrease);

    private:
        inline static const uint64_t IlluminanceProbeSize = 8; // Should less or equal to depth probe size
        inline static const uint64_t DepthProbeSize = 16;

        // Should be power of 2
        glm::uvec3 mProbeGridSize = glm::uvec3{ 20, 14, 20 };
        float mCellSize = 3.0f;
        float mDebugProbeRadius = 0.3f;
        float mIlluminanceHysteresisDecrease = 0.5f; // Quickly update probes at application startup
        float mDepthHysteresisDecrease = 0.5f;
        uint64_t mRaysPerProbe = 256;
        glm::mat4 mProbeRotation{ 1.0f };
        glm::vec3 mCornerPosition{ 0.0f };
        glm::ivec3 mSpawnedProbePlanesCount;

    public:
        Geometry::Dimensions GetRayHitInfoTextureSize() const;
        Geometry::Dimensions GetIlluminanceProbeSize() const;
        Geometry::Dimensions GetIlluminanceProbeSizeWithBorder() const;
        Geometry::Dimensions GetIlluminanceProbeAtlasSize() const;
        Geometry::Dimensions GetDepthProbeSize() const;
        Geometry::Dimensions GetDepthProbeSizeWithBorder() const;
        Geometry::Dimensions GetDepthProbeAtlasSize() const;
        glm::vec3 GetProbePosition(uint64_t probeIndex) const;
        glm::uvec2 GetIlluminanceProbeAtlasProbesPerDimension() const;
        glm::uvec2 GetDepthProbeAtlasProbesPerDimension() const;
        uint64_t GetTotalRayCount() const;
        uint64_t GetTotalProbeCount() const;

        const glm::uvec3& GetGridSize() const { return mProbeGridSize; }
        const glm::vec3& GetCornerPosition() const { return mCornerPosition; }
        const glm::ivec3& GetSpawnedProbePlanesCount() const { return mSpawnedProbePlanesCount; }
        const auto GetRaysPerProbe() const { return mRaysPerProbe; }
        const auto GetCellSize() const { return mCellSize; }
        const auto GetDebugProbeRadius() const { return mDebugProbeRadius; }
        const auto GetIlluminanceHysteresisDecrease() const { return mIlluminanceHysteresisDecrease; }
        const auto GetDepthHysteresisDecrease() const { return mDepthHysteresisDecrease; }
        const glm::mat4& GetProbeRotation() const { return mProbeRotation; }
    };

    class GIManager
    {
    public:
        GIManager(const Scene* scene);

        void Update();

        IlluminanceField ProbeField;
        std::optional<uint64_t> PickedDebugProbeIndex;
        bool GIDebugEnabled = false;
        bool DoNotRotateProbeRays = false;

    private:
        void UpdateGridCornerPosition();
        void UpdateHysteresisDecrease();

        const Scene* mScene = nullptr;
        uint64_t mIlluminanceHysteresisDecreseFrameCount = 10; // Quickly update probes at application startup
        uint64_t mDepthHysteresisDecreseFrameCount = 7; // Quickly update probes at application startup
    };

}
