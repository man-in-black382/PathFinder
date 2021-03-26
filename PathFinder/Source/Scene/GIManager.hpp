#pragma once

#include "Camera.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <Geometry/Dimensions.hpp>

namespace PathFinder
{

    class IrradianceField
    {
    public:
        void GenerateProbeRotation(const glm::vec2& random0to1);
        void SetCornerPosition(const glm::vec3& position);
        void SetDebugProbeRadius(float radius);

    private:
        inline static const uint64_t IrradianceProbeSize = 8;
        inline static const uint64_t DepthProbeSize = 16;

        // Should be power of 2
        glm::uvec3 mProbeGridSize = glm::uvec3{ 16, 16, 8 };
        float mCellSize = 2.0f;
        float mDebugProbeRadius = 0.3f;
        uint64_t mRaysPerProbe = 144;
        glm::mat4 mProbeRotation{ 1.0f };
        glm::vec3 mCornerPosition{ 0.0f };
        glm::ivec3 mSpawnedProbePlanesCount;

    public:
        Geometry::Dimensions GetRayHitInfoTextureSize() const;
        Geometry::Dimensions GetIrradianceProbeSize() const;
        Geometry::Dimensions GetIrradianceProbeSizeWithBorder() const;
        Geometry::Dimensions GetIrradianceProbeAtlasSize() const;
        Geometry::Dimensions GetDepthProbeSize() const;
        Geometry::Dimensions GetDepthProbeSizeWithBorder() const;
        Geometry::Dimensions GetDepthProbeAtlasSize() const;
        glm::vec3 GetProbePosition(uint64_t probeIndex) const;
        glm::uvec2 GetIrradianceProbeAtlasProbesPerDimension() const;
        glm::uvec2 GetDepthProbeAtlasProbesPerDimension() const;
        uint64_t GetTotalRayCount() const;
        uint64_t GetTotalProbeCount() const;

        const glm::uvec3& GridSize() const { return mProbeGridSize; }
        const glm::vec3& CornerPosition() const { return mCornerPosition; }
        const glm::ivec3& SpawnedProbePlanesCount() const { return mSpawnedProbePlanesCount; }
        const auto RaysPerProbe() const { return mRaysPerProbe; }
        const auto CellSize() const { return mCellSize; }
        const auto DebugProbeRadius() const { return mDebugProbeRadius; }
        const glm::mat4& ProbeRotation() const { return mProbeRotation; }
    };

    class GIManager
    {
    public:
        void SetCamera(const Camera* camera);
        void Update();

        IrradianceField ProbeField;
        std::optional<uint64_t> PickedDebugProbeIndex;
        bool GIDebugEnabled = false;
        bool DoNotRotateProbeRays = false;

    private:
        const Camera* mCamera = nullptr;
    };

}
