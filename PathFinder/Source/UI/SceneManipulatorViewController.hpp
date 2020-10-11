#pragma once

#include "ViewController.hpp"
#include "CameraViewModel.hpp"
#include "MeshInstanceViewModel.hpp"

#include <Scene/Camera.hpp>
#include <imguizmo/ImGuizmo.h>

namespace PathFinder
{
   
    class SceneManipulatorViewController : public ViewController
    {
    public:
        void Draw() override;
        bool IsInteracting() const override;

        CameraViewModel CameraVM;
        MeshInstanceViewModel MeshInstanceVM;

    private:
        bool EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition);
        bool mIsInteracting = false;
        bool mDrawCube = false;
        ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::LOCAL;
        bool mUseSnap = false;
        float mSnap[3] = { 1.f, 1.f, 1.f };
        float mBounds[6] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
        float mBoundsSnap[3] = { 0.1f, 0.1f, 0.1f };
        bool mBoundSizing = false;
        bool mBoundSizingSnap = false;
    };

}
