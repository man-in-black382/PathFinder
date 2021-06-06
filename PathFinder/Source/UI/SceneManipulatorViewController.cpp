#include "SceneManipulatorViewController.hpp"
#include "UIManager.hpp"

#include <RenderPipeline/RenderPasses/PipelineNames.hpp>

#include <imgui/imgui.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace PathFinder
{

    void SceneManipulatorViewController::OnCreated()
    {
        CameraVM = GetViewModel<CameraViewModel>();
        EntityVM = GetViewModel<PickedEntityViewModel>();
    }

    void SceneManipulatorViewController::DrawCameraControls()
    {
        ImGui::Text("Camera");
        ImGui::SliderFloat("FoV", &CameraVM->FOVH, 60.f, 120.f);
        ImGui::SliderFloat("Aperture", &CameraVM->LenseAperture, 1.f, 16.f);
        ImGui::SliderFloat("Film Speed (ISO)", &CameraVM->FilmSpeed, 100.f, 2000.f);
        ImGui::SliderFloat("Shutter Speed", &CameraVM->ShutterTime, 30.f, 240.f);
        ImGui::Text("Position: %f %f %f", CameraVM->CameraPosition().x, CameraVM->CameraPosition().y, CameraVM->CameraPosition().z);
        ImGui::Text("View Direction: %f %f %f", CameraVM->CameraDirection().x, CameraVM->CameraDirection().y, CameraVM->CameraDirection().z);
    }

    void SceneManipulatorViewController::DrawSkyControls()
    {
        ImGui::Separator();
        if (ImGui::Button("Select Sky"))
            EntityVM->SelectSky();
    }

    void SceneManipulatorViewController::DrawImGuizmoControls()
    {
        ImGuiIO& io = ImGui::GetIO();

        glm::mat4 modelMatrix = EntityVM->ModelMatrix();
        glm::mat4 deltaMatrix{ 1.0f };

        if (EntityVM->ShouldDisplay())
        {
            ImGuizmo::BeginFrame();
            ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
            ImGui::Separator();

            ImGuizmo::SetID(0);
            EditTransform(
                glm::value_ptr(CameraVM->View),
                glm::value_ptr(CameraVM->Projection),
                glm::value_ptr(modelMatrix),
                glm::value_ptr(deltaMatrix),
                EntityVM->ShouldDisplay(),
                EntityVM->GetAllowedGizmoTypes(),
                EntityVM->GetAllowedGizmoSpaces());

            mIsInteracting = ImGuizmo::IsUsing();

            if (mIsInteracting)
            {
                EntityVM->SetModifiedModelMatrix(modelMatrix, deltaMatrix);
            }

            ImGui::Checkbox("Draw Cube", &mDrawCube);
            if (mDrawCube)
            {
                ImGuizmo::DrawCubes(glm::value_ptr(CameraVM->View), glm::value_ptr(CameraVM->Projection), glm::value_ptr(modelMatrix), 1);
            }
        }
    }

    bool SceneManipulatorViewController::EditTransform(
        const float* cameraView, 
        float* cameraProjection, 
        float* matrix, 
        float* deltaMatrix, 
        bool editTransformDecomposition,
        PickedEntityViewModel::GizmoType allowedGizmoTypes,
        PickedEntityViewModel::GizmoSpace allowedGizmoSpaces)
    {
        if (editTransformDecomposition)
        {
            bool rotationAllowed = EnumMaskContains(allowedGizmoTypes, PickedEntityViewModel::GizmoType::Rotation);
            bool translationAllowed = EnumMaskContains(allowedGizmoTypes, PickedEntityViewModel::GizmoType::Translation);
            bool scaleAllowed = EnumMaskContains(allowedGizmoTypes, PickedEntityViewModel::GizmoType::Scale);

            if (translationAllowed)
            {
                if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
                    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
                ImGui::SameLine();
            }
            else
            {
                mCurrentGizmoOperation = ImGuizmo::ROTATE;
            }

            if (rotationAllowed)
            {
                if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
                    mCurrentGizmoOperation = ImGuizmo::ROTATE;
                ImGui::SameLine();
            }
            else
            {
                mCurrentGizmoOperation = ImGuizmo::SCALE;
            }
            
            if (scaleAllowed)
            {
                if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
                    mCurrentGizmoOperation = ImGuizmo::SCALE;
            }
            
            float matrixTranslation[3], matrixRotation[3], matrixScale[3];
            ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);

            if (translationAllowed)
            {
                ImGui::InputFloat3("Tr", matrixTranslation, 3);
            }
            
            if (rotationAllowed)
            {
                ImGui::InputFloat3("Rt", matrixRotation, 3);
            }
            
            if (scaleAllowed)
            {
                ImGui::InputFloat3("Sc", matrixScale, 3);
            }
                
            ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

            if (EnumMaskEquals(allowedGizmoSpaces, PickedEntityViewModel::GizmoSpace::All) && mCurrentGizmoOperation != ImGuizmo::SCALE)
            {
                if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                    mCurrentGizmoMode = ImGuizmo::LOCAL;
                ImGui::SameLine();
                if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                    mCurrentGizmoMode = ImGuizmo::WORLD;
            }
            else if (EnumMaskEquals(allowedGizmoSpaces, PickedEntityViewModel::GizmoSpace::Local))
            {
                mCurrentGizmoMode = ImGuizmo::LOCAL;
            }
            else if (EnumMaskEquals(allowedGizmoSpaces, PickedEntityViewModel::GizmoSpace::World))
            {
                mCurrentGizmoMode = ImGuizmo::WORLD;
            }

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
            deltaMatrix, 
            mUseSnap ? &mSnap[0] : nullptr, 
            mBoundSizing ? mBounds : nullptr, 
            mBoundSizingSnap ? mBoundsSnap : nullptr);
    }

    void SceneManipulatorViewController::Draw()
    {
        if (GetInput()->CurrentClickCount() == 1 && !GetUIManager()->IsInteracting() && !GetUIManager()->IsMouseOverUI())
        {
            EntityVM->HandleClick();
        }

        if (GetInput()->WasKeyboardKeyUnpressed(KeyboardKey::T) &&
            EnumMaskContains(EntityVM->GetAllowedGizmoTypes(), PickedEntityViewModel::GizmoType::Translation))
        {
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        }

        if (GetInput()->WasKeyboardKeyUnpressed(KeyboardKey::R) &&
            EnumMaskContains(EntityVM->GetAllowedGizmoTypes(), PickedEntityViewModel::GizmoType::Rotation))
        {
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        }

        if (GetInput()->WasKeyboardKeyUnpressed(KeyboardKey::S) &&
            EnumMaskContains(EntityVM->GetAllowedGizmoTypes(), PickedEntityViewModel::GizmoType::Scale))
        {
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        }

        if (GetInput()->WasKeyboardKeyUnpressed(KeyboardKey::B))
        {
            mBoundSizing = !mBoundSizing;
        }

        if (GetInput()->WasKeyboardKeyUnpressed(KeyboardKey::W) && 
            mCurrentGizmoOperation != ImGuizmo::SCALE)
        {
            mCurrentGizmoMode = mCurrentGizmoMode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }

        if (GetInput()->WasKeyboardKeyUnpressed(KeyboardKey::Escape))
        {
            EntityVM->HandleEsc();
        }
        
        CameraVM->Import();
        EntityVM->Import();

        ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        DrawCameraControls();
        DrawSkyControls();
        DrawImGuizmoControls();

        mIsInteracting = EntityVM->ShouldDisplay() || ImGuizmo::IsUsing();

        ImGui::End();

        CameraVM->Export();
        EntityVM->Export();
    }

    bool SceneManipulatorViewController::IsInteracting() const
    {
        return mIsInteracting;
    }

}
