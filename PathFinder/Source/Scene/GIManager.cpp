#include "GIManager.hpp"
#include <Foundation/Pi.hpp>

namespace PathFinder 
{

    void GIManager::SetCamera(const Camera* camera)
    {
        mCamera = camera;
    }

    void IrradianceField::GenerateProbeRotation(const glm::vec2& random0to1)
    {
        float phi = random0to1.x * 2.0 * M_PI;
        float cosTheta = 1.0 - random0to1.y;
        float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

        glm::vec3 viewDir{ cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta };
        glm::vec3 up = std::fabs(glm::dot(viewDir, glm::vec3{ 0,1,0 })) < 0.999 ? glm::vec3{ 0,1,0 } : glm::vec3{ 0,0,1 };

        mProbeRotation = glm::lookAt(glm::zero<glm::vec3>(), viewDir, up);
    }

    void IrradianceField::SetCornerPosition(const glm::vec3& position)
    {
        mCornerPosition = position;
    }

    Geometry::Dimensions IrradianceField::GetRayHitInfoTextureSize() const
    {
        return { GetTotalProbeCount(), mRaysPerProbe };
    }

    Geometry::Dimensions IrradianceField::GetIrradianceProbeSize() const
    {
        return { IrradianceProbeSize, IrradianceProbeSize };
    }

    Geometry::Dimensions IrradianceField::GetIrradianceProbeSizeWithBorder() const
    {
        return { IrradianceProbeSize + 2, IrradianceProbeSize + 2 };
    }

    Geometry::Dimensions IrradianceField::GetIrradianceProbeAtlasSize() const
    {
        auto probeCount = GetTotalProbeCount();
        auto probesPerRow = ceil(sqrt(float(probeCount)));
        auto rowCount = ceil(float(probeCount) / probesPerRow);
        auto probeSize = GetIrradianceProbeSizeWithBorder();

        return { uint64_t(probeSize.Width * probesPerRow), uint64_t(probeSize.Height * rowCount) };
    }

    Geometry::Dimensions IrradianceField::GetDepthProbeSize() const
    {
        return { DepthProbeSize, DepthProbeSize };
    }

    Geometry::Dimensions IrradianceField::GetDepthProbeSizeWithBorder() const
    {
        return { DepthProbeSize + 2, DepthProbeSize + 2 };
    }

    glm::uvec2 IrradianceField::GetDepthProbeAtlasProbesPerDimension() const
    {
        auto atlasDimensions = GetDepthProbeAtlasSize();
        auto probeSize = GetDepthProbeSizeWithBorder();

        return { atlasDimensions.Width / probeSize.Width, atlasDimensions.Height / probeSize.Height };
    }

    Geometry::Dimensions IrradianceField::GetDepthProbeAtlasSize() const
    {
        auto probeCount = GetTotalProbeCount();
        auto probesPerRow = ceil(sqrt(float(probeCount)));
        auto rowCount = ceil(float(probeCount) / probesPerRow);
        auto probeSize = GetDepthProbeSizeWithBorder();
        
        return { uint64_t(probeSize.Width * probesPerRow), uint64_t(probeSize.Height * rowCount) };
    }

    glm::uvec2 IrradianceField::GetIrradianceProbeAtlasProbesPerDimension() const
    {
        auto atlasDimensions = GetIrradianceProbeAtlasSize();
        auto probeSize = GetIrradianceProbeSizeWithBorder();

        return { atlasDimensions.Width / probeSize.Width, atlasDimensions.Height / probeSize.Height };
    }

    uint64_t IrradianceField::GetTotalRayCount() const
    {
        return GetTotalProbeCount() * mRaysPerProbe;
    }

    uint64_t IrradianceField::GetTotalProbeCount() const
    {
        return mProbeGridSize.x * mProbeGridSize.y * mProbeGridSize.z;
    }

}
