#include "SceneManipulatorViewController.hpp"

#include <imguizmo/ImGuizmo.h>
#include <imgui/imgui.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace PathFinder
{

    void SceneManipulatorViewController::SetCamera(Camera* camera)
    {
        mCamera = camera;
    }

    bool SceneManipulatorViewController::EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition)
    {
        static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
        static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
        static bool useSnap = false;
        static float snap[3] = { 1.f, 1.f, 1.f };
        static float bounds[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
        static float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
        static bool boundSizing = false;
        static bool boundSizingSnap = false;

        if (editTransformDecomposition)
        {
            if (ImGui::IsKeyPressed(90))
                mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(69))
                mCurrentGizmoOperation = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(82)) // r Key
                mCurrentGizmoOperation = ImGuizmo::SCALE;
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
                useSnap = !useSnap;
            ImGui::Checkbox("", &useSnap);
            ImGui::SameLine();

            switch (mCurrentGizmoOperation)
            {
            case ImGuizmo::TRANSLATE:
                ImGui::InputFloat3("Snap", &snap[0]);
                break;
            case ImGuizmo::ROTATE:
                ImGui::InputFloat("Angle Snap", &snap[0]);
                break;
            case ImGuizmo::SCALE:
                ImGui::InputFloat("Scale Snap", &snap[0]);
                break;
            }
            ImGui::Checkbox("Bound Sizing", &boundSizing);
            if (boundSizing)
            {
                ImGui::PushID(3);
                ImGui::Checkbox("", &boundSizingSnap);
                ImGui::SameLine();
                ImGui::InputFloat3("Snap", boundsSnap);
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
            useSnap ? &snap[0] : nullptr, 
            boundSizing ? bounds : nullptr, 
            boundSizingSnap ? boundsSnap : nullptr);
    }

    void SceneManipulatorViewController::Draw()
    {
        ImGuiIO& io = ImGui::GetIO();

        glm::mat4 view = mCamera->View();
        glm::mat4 projection = mCamera->Projection();

        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(1024, 100));
        ImGui::SetNextWindowSize(ImVec2(256, 256));

        // create a window and insert the inspector
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(320, 340));
        ImGui::Begin("Editor");
        ImGui::Text("Camera");

        float fov = 0.0f;
        ImGui::SliderFloat("Fov", &fov, 20.f, 110.f);

        ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);
        ImGuizmo::DrawCubes(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(modelMatDebug)/*&objectMatrix[0][0]*/, 1);
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Separator();

        ImGuizmo::SetID(0/*matId*/);
        mIsInteracting = EditTransform(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(modelMatDebug), true);

        ImGui::End();
    }

    bool SceneManipulatorViewController::IsInteracting() const
    {
        return mIsInteracting;
    }

}
