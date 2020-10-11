#include "SceneManipulatorViewController.hpp"
#include "UIManager.hpp"

#include <RenderPipeline/RenderPasses/PipelineNames.hpp>

#include <imgui/imgui.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace PathFinder
{

    bool SceneManipulatorViewController::EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition)
    {
        if (editTransformDecomposition)
        {
            if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
                mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
                mCurrentGizmoOperation = ImGuizmo::ROTATE;
            ImGui::SameLine();
            if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
                mCurrentGizmoOperation = ImGuizmo::SCALE;
            float matrixTranslation[3], matrixRotation[3], matrixScale[3];
            ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
            ImGui::InputFloat3("Tr", matrixTranslation, 3);
            ImGui::InputFloat3("Rt", matrixRotation, 3);
            ImGui::InputFloat3("Sc", matrixScale, 3);
            ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

            if (mCurrentGizmoOperation != ImGuizmo::SCALE)
            {
                if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                    mCurrentGizmoMode = ImGuizmo::LOCAL;
                ImGui::SameLine();
                if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                    mCurrentGizmoMode = ImGuizmo::WORLD;
            }
            if (ImGui::IsKeyPressed(83))
                mUseSnap = !mUseSnap;
            ImGui::Checkbox("", &mUseSnap);
            ImGui::SameLine();

            switch (mCurrentGizmoOperation)
            {
            case ImGuizmo::TRANSLATE:
                ImGui::InputFloat3("Snap", &mSnap[0]);
                break;
            case ImGuizmo::ROTATE:
                ImGui::InputFloat("Angle Snap", &mSnap[0]);
                break;
            case ImGuizmo::SCALE:
                ImGui::InputFloat("Scale Snap", &mSnap[0]);
                break;
            }
            ImGui::Checkbox("Bound Sizing", &mBoundSizing);
            if (mBoundSizing)
            {
                ImGui::PushID(3);
                ImGui::Checkbox("", &mBoundSizingSnap);
                ImGui::SameLine();
                ImGui::InputFloat3("Snap", mBoundsSnap);
                ImGui::PopID();
            }
        }
        ImGuiIO& io = ImGui::GetIO();

        return ImGuizmo::Manipulate(
            cameraView, 
            cameraProjection, 
            mCurrentGizmoOperation, 
            mCurrentGizmoMode, 
            matrix,
            nullptr, 
            mUseSnap ? &mSnap[0] : nullptr, 
            mBoundSizing ? mBounds : nullptr, 
            mBoundSizingSnap ? mBoundsSnap : nullptr);
    }

    void SceneManipulatorViewController::Draw()
    {
        if (mInput->CurrentClickCount() == 1 && !mUIManager->IsInteracting() && !mUIManager->IsMouseOverUI())
        {
            MeshInstanceVM.HandleClick();
        }

        if (mInput->WasKeyboardKeyUnpressed(KeyboardKey::T)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        if (mInput->WasKeyboardKeyUnpressed(KeyboardKey::R)) mCurrentGizmoOperation = ImGuizmo::ROTATE;
        if (mInput->WasKeyboardKeyUnpressed(KeyboardKey::S)) mCurrentGizmoOperation = ImGuizmo::SCALE;
        
        if (mInput->WasKeyboardKeyUnpressed(KeyboardKey::B)) mBoundSizing = !mBoundSizing;

        CameraVM.Import();
        MeshInstanceVM.Import();

        ImGuiIO& io = ImGui::GetIO();

        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(1024, 100));
        ImGui::SetNextWindowSize(ImVec2(256, 256));

        // create a window and insert the inspector
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(320, 340));
        ImGui::Begin("Editor");
        ImGui::Text("Camera");

        ImGui::SliderFloat("FoV", &CameraVM.FOVH, 20.f, 110.f);

        ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);
        
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Separator();

        mIsInteracting = false;

        if (MeshInstanceVM.ShouldDisplay)
        {
            ImGuizmo::SetID(0);
            mIsInteracting = EditTransform(glm::value_ptr(CameraVM.View), glm::value_ptr(CameraVM.Projection), glm::value_ptr(MeshInstanceVM.ModelMatrix), MeshInstanceVM.ShouldDisplay);
        
            ImGui::Checkbox("Draw Cube", &mDrawCube);
            if (mDrawCube)
            {
                ImGuizmo::DrawCubes(glm::value_ptr(CameraVM.View), glm::value_ptr(CameraVM.Projection), glm::value_ptr(MeshInstanceVM.ModelMatrix), 1);
            }
        }

        mIsInteracting = mIsInteracting || MeshInstanceVM.ShouldDisplay;

        ImGui::End();

        CameraVM.Export();
        MeshInstanceVM.Export();
    }

    bool SceneManipulatorViewController::IsInteracting() const
    {
        return mIsInteracting;
    }

}
