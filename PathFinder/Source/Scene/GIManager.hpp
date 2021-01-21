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

    private:
        inline static const uint64_t IrradianceProbeSize = 8;
        inline static const uint64_t DepthProbeSize = 16;

        // Should be power of 2
        glm::uvec3 mProbeGridSize = glm::uvec3{ 16 };
        float mCellSize = 1.0f;
        uint64_t mRaysPerProbe = 64;
        glm::mat4 mProbeRotation{ 1.0f };
        glm::vec3 mCornerPosition{ 0.0f };

    public:
        Geometry::Dimensions GetRayHitInfoTextureSize() const;
        Geometry::Dimensions GetIrradianceProbeSize() const;
        Geometry::Dimensions GetIrradianceProbeSizeWithBorder() const;
        Geometry::Dimensions GetIrradianceProbeAtlasSize() const;
        Geometry::Dimensions GetDepthProbeSize() const;
        Geometry::Dimensions GetDepthProbeSizeWithBorder() const;
        Geometry::Dimensions GetDepthProbeAtlasSize() const;
        glm::uvec2 GetIrradianceProbeAtlasProbesPerDimension() const;
        glm::uvec2 GetDepthProbeAtlasProbesPerDimension() const;
        uint64_t GetTotalRayCount() const;
        uint64_t GetTotalProbeCount() const;

        const glm::uvec3& GridSize() const { return mProbeGridSize; }
        const glm::vec3& CornerPosition() const { return mCornerPosition; }
        const auto RaysPerProbe() const { return mRaysPerProbe; }
        const auto CellSize() const { return mCellSize; }
        const glm::mat4& ProbeRotation() const { return mProbeRotation; }
    };

    class GIManager
    {
    public:
        void SetCamera(const Camera* camera);

        IrradianceField ProbeField;

    private:
        const Camera* mCamera = nullptr;
        
    public:
    };

}
