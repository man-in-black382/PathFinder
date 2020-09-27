#pragma once

#include "ViewController.hpp"

#include "../Scene/Camera.hpp"

namespace UI
{
   
    class SceneManipulatorViewController : ViewController
    {
    public:
        void SetCamera(PathFinder::Camera* camera);
        void Draw() override;

    private:
        PathFinder::Camera* mCamera;
    };

}
