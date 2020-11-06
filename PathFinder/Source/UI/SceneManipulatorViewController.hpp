#pragma once

#include "ViewController.hpp"
#include "CameraViewModel.hpp"
#include "PickedEntityViewModel.hpp"

#include <Scene/Camera.hpp>
#include <imguizmo/ImGuizmo.h>

namespace PathFinder
{
   
    class SceneManipulatorViewController : public ViewController
    {
    public:
        void Draw() override;
        bool IsInteracting() const override;
        void OnCreated() override;

        CameraViewModel* CameraVM;
        PickedEntityViewModel* EntityVM;

    private:
        void DrawCameraControls();
        void DrawImGuizmoControls();

        bool EditTransform(
            const float* cameraView, 
            float* cameraProjection, 
            float* matrix, 
            float* deltaMatrix, 
            bool editTransformDecomposition, 
            bool allowRotations);

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
