#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Camera.hpp"
#include "../IO/Input.hpp"

namespace PathFinder
{

    class Cameraman
    {
    private:
        Camera *mCamera = nullptr;
        Input *mUserInput = nullptr;
        //const GLViewport *mViewport = nullptr;

        glm::vec2 mPreviousMousePosition;
        glm::vec2 mRotation;
        glm::vec3 mMouseMoveDirection;
        glm::vec3 mKeyboardMoveDirection;
        glm::vec2 mMouseLockDirection;

        bool mIsEnabled = true;

        void handleKeyDown(const Input *input);

        void handleKeyUp(const Input *input);

        void handleMouseDrag(const Input *input);

        void handleMouseScroll(const Input *input);

        void handleMouseDown(const Input *input);

        void handleMouseUp(const Input *input);

        bool isMouseMovingVertically(const glm::vec2 &mouseDirection) const;

        bool isMouseMovingHorizontally(const glm::vec2 &mouseDirection) const;

    public:
        Cameraman(Camera *camera, Input *userInput/*, const GLViewport *viewport*/);

        Cameraman(const Cameraman &that) = default;

        Cameraman(Cameraman &&that) = default;

        Cameraman &operator=(const Cameraman &rhs) = default;

        Cameraman &operator=(Cameraman &&rhs) = default;

        ~Cameraman();

        bool isEnabled() const;

        void setIsEnabled(bool enabled);

        void updateCamera();
    };

}
