#include "MeshInstanceViewModel.hpp"

namespace PathFinder
{

    void MeshInstanceViewModel::SetScene(Scene* scene)
    {
        mScene = scene;
    }

    void MeshInstanceViewModel::SetGeometryIntersectionInfo(uint32_t info)
    {
        mHoveredInstanceID = info;
    }

    void MeshInstanceViewModel::HandleClick()
    {
        bool idValid = mHoveredInstanceID < mScene->MeshInstances().size();
        mMeshInstance = idValid ? &mScene->MeshInstances()[mHoveredInstanceID] : nullptr;
    }

    void MeshInstanceViewModel::Import()
    {
        ShouldDisplay = mMeshInstance != nullptr;
        ModelMatrix = mMeshInstance ? mMeshInstance->Transformation().ModelMatrix() : glm::mat4{ 1.0f };
    }

    void MeshInstanceViewModel::Export()
    {
        if (mMeshInstance) mMeshInstance->SetTransformation(Geometry::Transformation{ ModelMatrix });
    }

}
