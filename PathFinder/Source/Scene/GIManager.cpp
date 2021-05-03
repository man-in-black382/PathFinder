#include "GIManager.hpp"
#include "Scene.hpp"

#include <Foundation/Pi.hpp>
#include <Geometry/Utils.hpp>
#include <random>
#include <glm/gtx/compatibility.hpp>

namespace PathFinder 
{

    GIManager::GIManager(const Scene* scene)
        : mScene{ scene } {}

    void GIManager::Update()
    {
        UpdateGridCornerPosition();
        UpdateHysteresisDecrease();

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

    void GIManager::UpdateGridCornerPosition()
    {
        glm::vec3 gridSize = glm::vec3{ ProbeField.GetGridSize() } * ProbeField.GetCellSize();
        glm::vec3 halfGridSize = gridSize * 0.5f;

        // 4 front corners
        constexpr std::array<glm::vec3, 4> NDCCorners = { 
            glm::vec3(-1,-1,1), glm::vec3(-1,1,1), glm::vec3(1,1,1), glm::vec3(1,-1,1)
        };

        glm::mat4 inverseProjection = mScene->GetMainCamera().GetInverseProjection();
        glm::mat4 inverseView = mScene->GetMainCamera().GetInverseView();

        std::array<glm::vec3, 5> frustumPoints;

        for (auto corner = 0; corner < 4; ++corner)
        {
            glm::vec4 vertex = inverseProjection * glm::vec4{ NDCCorners[corner], 1.f };
            vertex /= vertex.w;
            vertex = inverseView * vertex;
            frustumPoints[corner] = vertex;
        }

        frustumPoints[4] = mScene->GetMainCamera().GetPosition();

        Geometry::AABB frustumAABB{ frustumPoints.begin(), frustumPoints.end() };

        // Camera position is a point on camera's frustum AABB, by definition.
        // We find normalized coordinates of that point.
        glm::vec3 normIntersectionPoint = mScene->GetMainCamera().GetPosition() - frustumAABB.GetMin();
        normIntersectionPoint /= frustumAABB.GetMax() - frustumAABB.GetMin();

        // Get cascades min point by centering cascade around the camera position
        glm::vec3 cascadeMinPoint = mScene->GetMainCamera().GetPosition() - halfGridSize;

        // Compute anchor which is a point on the cascade's AABB 
        glm::vec3 cascadeAnchor = normIntersectionPoint * gridSize + cascadeMinPoint;

        // We want to move cascade so that anchor point is at the same place as camera position
        glm::vec3 giCascadeDisplacement = mScene->GetMainCamera().GetPosition() - cascadeAnchor;

        // We move cascade to camera but then move it backwards by one cell size so that we always have a probe behind the camera, for more smooth GI
        glm::vec3 cascadeOptimalMinPoint = cascadeMinPoint + giCascadeDisplacement - mScene->GetMainCamera().GetFront() * ProbeField.GetCellSize();

        ProbeField.SetCornerPosition(Geometry::Snap(cascadeOptimalMinPoint, glm::vec3{ ProbeField.GetCellSize() }));
    }

    void GIManager::UpdateHysteresisDecrease()
    {
        float irradianceHysteresisDecrease = 0.0f;
        float depthHysteresisDecrease = 0.0f;

        auto perceptuallyEncodeLuminance = [](float luminance) -> float
        {
            return std::pow(luminance, 1.0 / 5.0);
        };

        float maxHysteresisDecrease = 0.5f;
        float geometricChangeSensitivity = 3.0f;

        auto computeHysteresisDecreaseForLights = [&](auto&& lights)
        {
            for (auto& light : lights)
            {
                float perceptualPreviousLumianance = perceptuallyEncodeLuminance(light.GetPreviousLuminance());
                float perceptualLumianance = perceptuallyEncodeLuminance(light.GetLuminance());

                float areaChange = std::abs(light.GetPreviousArea() - light.GetArea());
                float luminanceChange = std::abs(perceptualPreviousLumianance - perceptualLumianance);
                float distanceTravelled = glm::distance(light.GetPosition(), light.GetPreviousPosition());

                // Computing heuristics relative to probe cell sizes
                // Light area, luminance or movement change means lighting condition change, so we need to drop history
                float areaLerpFactor = glm::clamp(areaChange / ProbeField.GetCellSize() * geometricChangeSensitivity, 0.0f, 1.0f);
                float hysteresisDecreaseDueToAreaChange = glm::lerp(0.0f, maxHysteresisDecrease, areaLerpFactor);

                float luminanceLerpFactor = glm::clamp(
                    std::max(perceptualPreviousLumianance, perceptualLumianance) / std::min(perceptualPreviousLumianance, perceptualLumianance), 
                    0.0f, 1.0f);
                float hysteresisDecreaseDueToLuminanceChange = glm::lerp(0.0f, maxHysteresisDecrease, luminanceLerpFactor);

                float movementLerpFactor = glm::clamp(distanceTravelled / ProbeField.GetCellSize() * geometricChangeSensitivity, 0.0f, 1.0f);
                float hysteresisDecreaseDueToMovement = glm::lerp(0.0f, maxHysteresisDecrease, movementLerpFactor);

                float lightHysteresisDecrease = std::max(hysteresisDecreaseDueToAreaChange, std::max(hysteresisDecreaseDueToAreaChange, hysteresisDecreaseDueToMovement));

                irradianceHysteresisDecrease = std::max(irradianceHysteresisDecrease, lightHysteresisDecrease);
            }
        };

        computeHysteresisDecreaseForLights(mScene->GetSphericalLights());
        computeHysteresisDecreaseForLights(mScene->GetRectangularLights());
        computeHysteresisDecreaseForLights(mScene->GetDiskLights());

        for (const MeshInstance& instance : mScene->GetMeshInstances())
        {
            const Geometry::AABB& aabb = instance.GetAssociatedMesh()->GetBoundingBox();
            const Geometry::AABB previousAABB = aabb.TransformedBy(instance.GetPreviousTransformation());
            const Geometry::AABB currentAABB = aabb.TransformedBy(instance.GetTransformation());

            float diagonalChange = std::abs(previousAABB.Diagonal() - currentAABB.Diagonal());
            float distanceTravelled = glm::distance(instance.GetPreviousTransformation().GetTranslation(), instance.GetTransformation().GetTranslation());

            // The bigger the mesh, the more impact it has on indirect lighting
            float importance = currentAABB.Diagonal() / ProbeField.GetCellSize() * 0.5f;

            float diagonalLerpFactor = glm::clamp(diagonalChange / ProbeField.GetCellSize(), 0.0f, 1.0f);
            // Movement weight depends on how large the object is in relation to probe grid
            float movementLerpFactor = glm::clamp(distanceTravelled / ProbeField.GetCellSize() * importance, 0.0f, 1.0f);

            float hysteresisDecreaseDueSizeChange = glm::lerp(0.0f, maxHysteresisDecrease, diagonalLerpFactor);
            float hysteresisDecreaseDueToMovement = glm::lerp(0.0f, maxHysteresisDecrease, movementLerpFactor);

            float hysteresisDecrease = std::max(hysteresisDecreaseDueSizeChange, hysteresisDecreaseDueToMovement);

            irradianceHysteresisDecrease = std::max(irradianceHysteresisDecrease, hysteresisDecrease);
            depthHysteresisDecrease = std::max(depthHysteresisDecrease, hysteresisDecrease);
        }

        // The larger the hysteresis decrease, the larger the frame count that we must keep it
        float irradianceDecreaseFrameDuration = glm::lerp(0.0f, 10.0f, irradianceHysteresisDecrease / maxHysteresisDecrease);
        float depthDecreaseFrameDuration = glm::lerp(0.0f, 7.0f, depthHysteresisDecrease / maxHysteresisDecrease);

        mIrradianceHysteresisDecreseFrameCount = std::max(mIrradianceHysteresisDecreseFrameCount, uint64_t(irradianceDecreaseFrameDuration));
        irradianceHysteresisDecrease = std::max(irradianceHysteresisDecrease, ProbeField.GetIrradianceHysteresisDecrease());

        mDepthHysteresisDecreseFrameCount = std::max(mDepthHysteresisDecreseFrameCount, uint64_t(depthDecreaseFrameDuration));
        depthHysteresisDecrease = std::max(depthHysteresisDecrease, ProbeField.GetDepthHysteresisDecrease());

        //IC(depthHysteresisDecrease);

        // We stop decreasing hysteresis if there is no decrease this frame and we have no frames left
        if (mIrradianceHysteresisDecreseFrameCount == 0)
            irradianceHysteresisDecrease = 0;    

        if (mDepthHysteresisDecreseFrameCount == 0)
            depthHysteresisDecrease = 0;

        // Decrease frame count
        if (mIrradianceHysteresisDecreseFrameCount > 0)
            mIrradianceHysteresisDecreseFrameCount -= 1;

        if (mDepthHysteresisDecreseFrameCount > 0)
            mDepthHysteresisDecreseFrameCount -= 1;

        ProbeField.SetIrradianceHysteresisDecrease(irradianceHysteresisDecrease);
        ProbeField.SetDepthHysteresisDecrease(depthHysteresisDecrease);
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

    void IrradianceField::SetIrradianceHysteresisDecrease(float decrease)
    {
        mIrradianceHysteresisDecrease = decrease;
    }

    void IrradianceField::SetDepthHysteresisDecrease(float decrease)
    {
        mDepthHysteresisDecrease = decrease;
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
