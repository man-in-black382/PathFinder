#include "PickedEntityViewModel.hpp"

#include <Foundation/STDHelpers.hpp>
#include <fplus/fplus.hpp>

namespace PathFinder
{

    void PickedEntityViewModel::SetScene(Scene* scene)
    {
        mScene = scene;
    }

    void PickedEntityViewModel::SetGeometryIntersectionInfo(uint32_t info)
    {
        mHoveredEntityID = EntityID{ info };
    }

    void PickedEntityViewModel::HandleClick()
    {
        mMeshInstance = nullptr;
        mSphericalLight = nullptr;
        mFlatLight = nullptr;

        if (auto entity = mScene->GetEntityByID(mHoveredEntityID))
        {
            std::visit(Foundation::MakeVisitor(
                [this](MeshInstance* instance) { mMeshInstance = instance; },
                [this](SphericalLight* light) { mSphericalLight = light; },
                [this](FlatLight* light) { mFlatLight = light; }),
                *entity);
        }
    }

    void PickedEntityViewModel::SetModifiedModelMatrix(const glm::mat4& mat, const glm::mat4& delta)
    {
        mModifiedModelMatrix = mat;
        mDeltaMatrix = delta;
    }

    void PickedEntityViewModel::Import()
    {
        mShouldDisplay = mMeshInstance != nullptr || mSphericalLight != nullptr || mFlatLight != nullptr;
        mAreRotationsAllowed = mSphericalLight == nullptr;
       
        if (mMeshInstance) mModelMatrix = mMeshInstance->Transformation().ModelMatrix();
        else if (mSphericalLight) mModelMatrix = mSphericalLight->ModelMatrix();
        else if (mFlatLight) mModelMatrix = mFlatLight->ModelMatrix();

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
            glm::vec3 lightScale{ mModifiedModelMatrix[0][0], mModifiedModelMatrix[1][1], mModifiedModelMatrix[2][2] };
            
            mFlatLight->SetWidth(lightScale.x);
            mFlatLight->SetHeight(lightScale.y);
            mFlatLight->SetPosition(mModifiedModelMatrix[3]);
            mFlatLight->SetNormal(glm::normalize(glm::vec3{ mModifiedModelMatrix[2] }));
        }
    }

}
