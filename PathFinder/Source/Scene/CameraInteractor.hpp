#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Camera.hpp"
#include "../IO/Input.hpp"
#include "../Geometry/Dimensions.hpp"

namespace PathFinder
{

    class CameraInteractor
    {
    public:
        struct InputScale
        {
            float KeyboardMovementScale = 0.2f;
            float MouseMovementScale = 0.07f;
        };

        CameraInteractor(Camera* camera, Input* userInput);

        bool IsEnabled() const;
        void SetIsEnabled(bool enabled);
        void SetViewportSize(const Geometry::Dimensions& viewportSize);
        void PollInputs(uint64_t frameDeltaTime);

    private:
        void HandleKeyDown();
        void HandleMouseDrag();
        void HandleMouseScroll();
        bool IsMouseMovingVertically(const glm::vec2 &mouseDirection) const;
        bool IsMouseMovingHorizontally(const glm::vec2 &mouseDirection) const;

        Camera* mCamera = nullptr;
        Input* mUserInput = nullptr;

        glm::vec2 mViewportSize;
        glm::vec2 mRotation;
        glm::vec3 mMouseMoveDirection;
        glm::vec3 mKeyboardMoveDirection;
        glm::vec2 mMouseLockDirection;
        InputScale mInputScale;
        InputScale mInputScaleTimeAdjusted;

        bool mIsEnabled = true;
    };

}
