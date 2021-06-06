#include "PickedEntityViewModel.hpp"

#include <Foundation/STDHelpers.hpp>
#include <Geometry/Utils.hpp>
#include <fplus/fplus.hpp>
#include <RenderPipeline/RenderPasses/PipelineNames.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/transform.hpp>

namespace PathFinder
{

    void PickedEntityViewModel::HandleClick()
    {
        mMeshInstance = nullptr;
        mSphericalLight = nullptr;
        mFlatLight = nullptr;
        mSky = nullptr;

        if (mPickedEntityInfo.GPUIndex != PickedGPUEntityInfo::NoEntity)
        {
            switch (PickedGPUEntityInfo::GPUEntityType{ mPickedEntityInfo.EntityType })
            {
            case PickedGPUEntityInfo::GPUEntityType::MeshInstance:
                mMeshInstance = mScene->GetMeshInstanceForGPUIndex(mPickedEntityInfo.GPUIndex);
                break;

            case PickedGPUEntityInfo::GPUEntityType::Light:
                std::visit(Foundation::MakeVisitor(
                    [this](SphericalLight* light) { mSphericalLight = light; },
                    [this](FlatLight* light) { mFlatLight = light; }),
                    mScene->GetLightForGPUIndex(mPickedEntityInfo.GPUIndex));
                break;

            case PickedGPUEntityInfo::GPUEntityType::DebugGIProbe:
                Dependencies->ScenePtr->GetGIManager().PickedDebugProbeIndex = mPickedEntityInfo.GPUIndex;
                break;
            }
        }
        else
        {
            // Clear probe selection only when pressed on empty space to allow simultaneously picking meshes
            Dependencies->ScenePtr->GetGIManager().PickedDebugProbeIndex = std::nullopt;
        }
    }

    void PickedEntityViewModel::HandleEsc()
    {
        mMeshInstance = nullptr;
        mSphericalLight = nullptr;
        mFlatLight = nullptr;
        mSky = nullptr;
        Dependencies->ScenePtr->GetGIManager().PickedDebugProbeIndex = std::nullopt;
    }

    void PickedEntityViewModel::SelectSky()
    {
        mMeshInstance = nullptr;
        mSphericalLight = nullptr;
        mFlatLight = nullptr;
        mSky = &Dependencies->ScenePtr->GetSky();
        mModelMatrix = ConstructSunMatrix(*mSky);
        mModifiedModelMatrix = mModelMatrix;
    }

    void PickedEntityViewModel::SetModifiedModelMatrix(const glm::mat4& mat, const glm::mat4& delta)
    {
        mModifiedModelMatrix = mat;
        mDeltaMatrix = delta;
    }

    void PickedEntityViewModel::Import()
    {
        mScene = Dependencies->ScenePtr;

        mShouldDisplay = mMeshInstance != nullptr || mSphericalLight != nullptr || mFlatLight != nullptr || mSky != nullptr;
        mAllowedGizmoTypes = GizmoType::All;
        mAllowedGizmoSpaces = GizmoSpace::All;

        if (mSphericalLight)
        {
            mAllowedGizmoTypes &= ~GizmoType::Rotation;
            mAllowedGizmoSpaces = GizmoSpace::World;
        }
            
        if (mSky)
        {
            mAllowedGizmoTypes = GizmoType::Rotation;
            mAllowedGizmoSpaces = GizmoSpace::World;
        }
           
        if (mMeshInstance)
            mModelMatrix = mMeshInstance->GetTransformation().GetMatrix();
        else if (mSphericalLight)
            mModelMatrix = mSphericalLight->GetModelMatrix();
        else if (mFlatLight)
            mModelMatrix = mFlatLight->GetModelMatrix();
        else if (mSky)
            mModelMatrix = ConstructSunMatrix(*mSky);
        else
            mModelMatrix = glm::mat4{ 1.0f };

        mModifiedModelMatrix = mModelMatrix;
        mDeltaMatrix = glm::mat4{ 1.0f };
    }

    void PickedEntityViewModel::Export()
    {
        if (mMeshInstance)
        {
            mMeshInstance->SetTransformation(Geometry::Transformation{ mModifiedModelMatrix });
        }
        else if (mSphericalLight)
        {
            float scale = 1.0f;

            // Find dimension that's changed
            for (auto i = 0; i < 3; ++i)
            {
                if (mDeltaMatrix[i][i] != 1.0f)
                    scale = mModifiedModelMatrix[i][i];
            }

            mSphericalLight->SetPosition(mModifiedModelMatrix[3]);

            // Update radius if any dimension changed
            if (scale != 1.0f)
            {
                mSphericalLight->SetRadius(scale);
            }
        }
        else if (mFlatLight)
        {
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::vec3 scale;
            glm::quat rotation;
            glm::vec3 translation;
            glm::decompose(mModifiedModelMatrix, scale, rotation, translation, skew, perspective);

            mFlatLight->SetWidth(scale.x);
            mFlatLight->SetHeight(scale.y);
            mFlatLight->SetPosition(translation);
            mFlatLight->SetRotation(rotation);
        }
        else if (mSky)
        {
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::vec3 scale;
            glm::quat rotation;
            glm::vec3 translation;
            glm::decompose(mModifiedModelMatrix, scale, rotation, translation, skew, perspective);

            glm::vec3 newSunDirection = glm::mat3_cast(rotation) * glm::vec3{ 0.0, 0.0, 1.0 };
            newSunDirection.y = glm::clamp(newSunDirection.y, 0.001f, 1.0f);
            mSky->SetSunDirection(glm::normalize(newSunDirection));
        }
    }

    void PickedEntityViewModel::OnCreated()
    {
        Dependencies->RenderEngine->PostRenderEvent() += { "PickedEntityViewModel.Post.Render", [this]()
        {
            const Memory::Buffer* pickedGeometryInfo = Dependencies->ResourceStorage->GetPerResourceData(PathFinder::ResourceNames::PickedGeometryInfo)->Buffer.get();

            pickedGeometryInfo->Read<PickedGPUEntityInfo>([this](const PickedGPUEntityInfo* info)
            {
                if (info)
                {
                    mPickedEntityInfo = *info;
                }
            });
        }};
    }

    glm::mat4 PickedEntityViewModel::ConstructSunMatrix(const Sky& sky) const
    {
        const Camera& camera = mScene->GetMainCamera();

        glm::vec3 position = camera.GetPosition() + camera.GetFront() * 1.5f - camera.GetRight() * 0.5f;
        glm::mat3 rotation = Geometry::OrientationMatrix(sky.GetSunDirection());
        glm::mat4 translation = glm::translate(position);
        glm::mat4 gizmoTransform = translation * glm::mat4{ rotation };

        return gizmoTransform;
    }

}
