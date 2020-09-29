#pragma once

#include "ViewController.hpp"

#include "../Scene/Camera.hpp"

namespace PathFinder
{
   
    class SceneManipulatorViewController : ViewController
    {
    public:
        void SetCamera(Camera* camera);
        void Draw() override;
        bool IsInteracting() const override;

    private:
        bool EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition);

        glm::mat4 modelMatDebug{ 1.0f };

        bool mIsInteracting = false;
        Camera* mCamera;
    };

}
