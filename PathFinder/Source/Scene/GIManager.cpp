#include "GIManager.hpp"

#include <Foundation/Pi.hpp>
#include <Geometry/Utils.hpp>
#include <random>

namespace PathFinder 
{

    void GIManager::SetCamera(const Camera* camera)
    {
        mCamera = camera;
    }

    void GIManager::Update()
    {
        ProbeField.SetCornerPosition(GenerateGridCornerPosition());

        if (!DoNotRotateProbeRays)
        {
            std::random_device rd;  //Will be used to obtain a seed for the random number engine
            std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
            std::uniform_real_distribution<> dis(0.0, 1.0);

            ProbeField.GenerateProbeRotation(glm::vec2{ dis(gen), dis(gen) });
        }
        else
        {
            ProbeField.GenerateProbeRotation(glm::vec2{ 0.0 });
        }
    }

    glm::vec3 GIManager::GenerateGridCornerPosition() const
    {
        glm::vec3 gridSize = glm::vec3{ ProbeField.GridSize() } * ProbeField.CellSize();
        glm::vec3 halfGridSize = gridSize * 0.5f;

        // 4 front corners
        constexpr std::array<glm::vec3, 4> NDCCorners = { 
            glm::vec3(-1,-1,1), glm::vec3(-1,1,1), glm::vec3(1,1,1), glm::vec3(1,-1,1)
        };

        glm::mat4 inverseProjection = mCamera->GetInverseProjection();
        glm::mat4 inverseView = mCamera->GetInverseView();

        std::array<glm::vec3, 5> frustumPoints;

        for (auto corner = 0; corner < 4; ++corner)
        {
            glm::vec4 vertex = inverseProjection * glm::vec4{ NDCCorners[corner], 1.f };
            vertex /= vertex.w;
            vertex = inverseView * vertex;
            frustumPoints[corner] = vertex;
        }

        frustumPoints[4] = mCamera->GetPosition();

        Geometry::AABB frustumAABB{ frustumPoints.begin(), frustumPoints.end() };

        // Camera position is a point on camera's frustum AABB, by definition.
        // We find normalized coordinates of that point.
        glm::vec3 normIntersectionPoint = mCamera->GetPosition() - frustumAABB.GetMin();
        normIntersectionPoint /= frustumAABB.GetMax() - frustumAABB.GetMin();

        // Get cascades min point by centering cascade around the camera position
        glm::vec3 cascadeMinPoint = mCamera->GetPosition() - halfGridSize;

        // Compute anchor which is a point on the cascade's AABB 
        glm::vec3 cascadeAnchor = normIntersectionPoint * gridSize + cascadeMinPoint;

        // We want to move cascade so that anchor point is at the same place as camera position
        glm::vec3 giCascadeDisplacement = mCamera->GetPosition() - cascadeAnchor;

        // We move cascade to camera but then move it backwards by one cell size so that we always have a probe behind the camera, for more smooth GI
        glm::vec3 cascadeOptimalMinPoint = cascadeMinPoint + giCascadeDisplacement - mCamera->GetFront() * ProbeField.CellSize();

        return Geometry::Snap(cascadeOptimalMinPoint, glm::vec3{ ProbeField.CellSize() });
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
        glm::vec3 distanceTravelled = position - mCornerPosition;
        mSpawnedProbePlanesCount = distanceTravelled / mCellSize;
        mCornerPosition = position;
    }

    void IrradianceField::SetDebugProbeRadius(float radius)
    {
        mDebugProbeRadius = radius;
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

    glm::vec3 IrradianceField::GetProbePosition(uint64_t probeIndex) const
    {
        auto x = probeIndex % mProbeGridSize.x;
        auto y = (probeIndex % (mProbeGridSize.x * mProbeGridSize.y)) / mProbeGridSize.x;
        auto z = probeIndex / (mProbeGridSize.x * mProbeGridSize.y);

        return glm::vec3{ float(x * mCellSize), float(y * mCellSize), float(z * mCellSize) } + mCornerPosition;
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
