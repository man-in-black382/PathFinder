#pragma once

#include "ViewModel.hpp"

#include <Scene/MeshInstance.hpp>
#include <Scene/Scene.hpp>

namespace PathFinder
{
   
    struct PickedGPUEntityInfo
    {
        enum class GPUEntityType : uint32_t
        {
            MeshInstance = 0, Light = 1, DebugGIProbe = 2
        };

        static const uint32_t NoEntity = std::numeric_limits<uint32_t>::max();

        uint32_t GPUIndex = NoEntity;
        uint32_t EntityType = 0;
    };

    class PickedEntityViewModel : public ViewModel
    {
    public:
        void HandleClick();
        void HandleEsc();
        void SetModifiedModelMatrix(const glm::mat4& mat, const glm::mat4& delta);

        void Import() override;
        void Export() override;
        void OnCreated() override;

    private:
        bool mShouldDisplay = false;
        bool mAreRotationsAllowed = true;
        bool mRotateProbeRaysEachFrame = true;
        glm::mat4 mModelMatrix;
        glm::mat4 mModifiedModelMatrix;
        glm::mat4 mDeltaMatrix;
        MeshInstance* mMeshInstance = nullptr;
        SphericalLight* mSphericalLight = nullptr;
        FlatLight* mFlatLight = nullptr;
        Scene* mScene = nullptr;
        PickedGPUEntityInfo mPickedEntityInfo;

    public:
        inline const glm::mat4& ModelMatrix() const { return mModelMatrix; }
        inline bool ShouldDisplay() const { return mShouldDisplay; }
        inline bool AreRotationsAllowed() const { return mAreRotationsAllowed; }
    };

}
