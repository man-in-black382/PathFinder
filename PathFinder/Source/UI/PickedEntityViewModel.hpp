#pragma once

#include "ViewModel.hpp"

#include <Scene/MeshInstance.hpp>
#include <Scene/Scene.hpp>

namespace PathFinder
{
   
    class PickedEntityViewModel : public ViewModel
    {
    public:
        void SetScene(Scene* scene);
        void SetGeometryIntersectionInfo(uint32_t info);

        void HandleClick();
        void SetModifiedModelMatrix(const glm::mat4& mat, const glm::mat4& delta);

        void Import() override;
        void Export() override;

    private:
        bool mShouldDisplay = false;
        bool mAreRotationsAllowed = true;
        glm::mat4 mModelMatrix;
        glm::mat4 mModifiedModelMatrix;
        glm::mat4 mDeltaMatrix;
        MeshInstance* mMeshInstance = nullptr;
        SphericalLight* mSphericalLight = nullptr;
        FlatLight* mFlatLight = nullptr;
        Scene* mScene = nullptr;
        EntityID mHoveredEntityID = NoEntityID;

    public:
        inline const glm::mat4& ModelMatrix() const { return mModelMatrix; }
        inline bool ShouldDisplay() const { return mShouldDisplay; }
        inline bool AreRotationsAllowed() const { return mAreRotationsAllowed; }
    };

}
