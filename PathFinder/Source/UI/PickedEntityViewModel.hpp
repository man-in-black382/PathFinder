#pragma once

#include "ViewModel.hpp"

#include <Scene/MeshInstance.hpp>
#include <Scene/Scene.hpp>
#include <Foundation/BitwiseEnum.hpp>

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
        enum class GizmoType : uint32_t
        {
            Translation = 1 << 0,
            Rotation = 1 << 1, 
            Scale = 1 << 2,
            All = Translation | Rotation | Scale
        };

        enum class GizmoSpace
        {
            Local = 1 << 0,
            World = 1 << 1,
            All = Local | World
        };

        void HandleClick();
        void HandleEsc();
        void SelectSky();
        void SetModifiedModelMatrix(const glm::mat4& mat, const glm::mat4& delta);

        void Import() override;
        void Export() override;
        void OnCreated() override;

    private:
        glm::mat4 ConstructSunMatrix(const Sky& sky) const;

        bool mShouldDisplay = false;
        GizmoType mAllowedGizmoTypes = GizmoType::All;
        GizmoSpace mAllowedGizmoSpaces = GizmoSpace::All;
        bool mRotateProbeRaysEachFrame = true;
        glm::mat4 mModelMatrix;
        glm::mat4 mModifiedModelMatrix;
        glm::mat4 mDeltaMatrix;
        MeshInstance* mMeshInstance = nullptr;
        SphericalLight* mSphericalLight = nullptr;
        FlatLight* mFlatLight = nullptr;
        Sky* mSky = nullptr;
        Scene* mScene = nullptr;
        
        PickedGPUEntityInfo mPickedEntityInfo;

    public:
        inline const glm::mat4& ModelMatrix() const { return mModelMatrix; }
        inline bool ShouldDisplay() const { return mShouldDisplay; }
        inline GizmoType GetAllowedGizmoTypes() const { return mAllowedGizmoTypes; }
        inline GizmoSpace GetAllowedGizmoSpaces() const { return mAllowedGizmoSpaces; }
    };

}

ENABLE_BITMASK_OPERATORS(PathFinder::PickedEntityViewModel::GizmoType);
ENABLE_BITMASK_OPERATORS(PathFinder::PickedEntityViewModel::GizmoSpace);