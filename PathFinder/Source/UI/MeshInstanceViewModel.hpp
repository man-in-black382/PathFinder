#pragma once

#include "ViewModel.hpp"

#include <Scene/MeshInstance.hpp>
#include <Scene/Scene.hpp>

namespace PathFinder
{
   
    class MeshInstanceViewModel : public ViewModel
    {
    public:
        void SetScene(Scene* scene);
        void SetGeometryIntersectionInfo(uint32_t info);

        void HandleClick();

        void Import() override;
        void Export() override;

        glm::mat4 ModelMatrix;
        bool ShouldDisplay = false;

    private:
        MeshInstance* mMeshInstance = nullptr;
        Scene* mScene = nullptr;
        uint32_t mHoveredInstanceID = std::numeric_limits<uint32_t>::max();
    };

}
