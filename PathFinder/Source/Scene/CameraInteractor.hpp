#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Camera.hpp"
#include "../IO/Input.hpp"

namespace PathFinder
{

    class CameraInteractor
    {
    public:
        CameraInteractor(Camera* camera, Input* userInput);
        ~CameraInteractor();

        bool IsEnabled() const;
        void SetIsEnabled(bool enabled);
        void UpdateCamera();

    private:
        void HandleKeyDown(const Input *input);
        void HandleKeyUp(const Input *input);
        void HandleMouseDrag(const Input* input);
        void HandleMouseScroll(const Input *input);
        void HandleMouseDown(const Input *input);
        void HandleMouseUp(const Input *input);
        bool IsMouseMovingVertically(const glm::vec2 &mouseDirection) const;
        bool IsMouseMovingHorizontally(const glm::vec2 &mouseDirection) const;

        Camera* mCamera = nullptr;
        Input* mUserInput = nullptr;

        glm::vec2 mPreviousMousePosition;
        glm::vec2 mRotation;
        glm::vec3 mMouseMoveDirection;
        glm::vec3 mKeyboardMoveDirection;
        glm::vec2 mMouseLockDirection;

        bool mIsEnabled = true;
    };

}
