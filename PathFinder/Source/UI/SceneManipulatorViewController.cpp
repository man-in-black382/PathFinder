#include "SceneManipulatorViewController.hpp"

#include <imguizmo/ImGuizmo.h>
#include <imgui/imgui.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace UI
{

    void SceneManipulatorViewController::SetCamera(PathFinder::Camera* camera)
    {
        mCamera = camera;
    }

    void SceneManipulatorViewController::Draw()
    {
        ImGuiIO& io = ImGui::GetIO();

        glm::mat4 view = mCamera->View();
        glm::mat4 projection = mCamera->Projection();
        glm::mat4 identity{ 1.0f };

        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(1024, 100));
        ImGui::SetNextWindowSize(ImVec2(256, 256));

        // create a window and insert the inspector
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(320, 340));
        ImGui::Begin("Editor");
        ImGui::Text("Camera");

            //ImGui::SliderFloat("Fov", &fov, 20.f, 110.f);

        //viewDirty |= ImGui::SliderFloat("Distance", &camDistance, 1.f, 10.f);

        ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);
        ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(identity), 100.f);
        /*  ImGuizmo::DrawCubes(cameraView, cameraProjection, &objectMatrix[0][0], gizmoCount);
          ImGui::Separator();
          for (int matId = 0; matId < gizmoCount; matId++)
          {
              ImGuizmo::SetID(matId);

              EditTransform(cameraView, cameraProjection, objectMatrix[matId], lastUsing == matId);
              if (ImGuizmo::IsUsing())
              {
                  lastUsing = matId;
              }
          }*/

        ImGui::End();
    }

}
