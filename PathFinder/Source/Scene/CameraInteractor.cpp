#include "CameraInteractor.hpp"
#include "../Foundation/Pi.hpp"

#include <windows.h>
#include "../Foundation/StringUtils.hpp"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace PathFinder 
{

    CameraInteractor::CameraInteractor(Camera* camera, Input* userInput)
        : mCamera{ camera }, mUserInput{ userInput } {}

    bool CameraInteractor::IsEnabled() const
    {
        return mIsEnabled;
    }

    void CameraInteractor::SetIsEnabled(bool enabled)
    {
        mIsEnabled = enabled;
    }

    void CameraInteractor::SetViewportSize(const Geometry::Dimensions& viewportSize)
    {
        mViewportSize = { viewportSize.Width, viewportSize.Height };
    }

    void CameraInteractor::HandleKeyDown()
    {
        glm::vec3 direction = glm::zero<glm::vec3>();

        if (mUserInput->IsKeyboardKeyPressed(KeyboardKey::W)) { direction += mCamera->Front(); }
        if (mUserInput->IsKeyboardKeyPressed(KeyboardKey::A)) { direction -= mCamera->Right(); }
        if (mUserInput->IsKeyboardKeyPressed(KeyboardKey::S)) { direction -= mCamera->Front(); }
        if (mUserInput->IsKeyboardKeyPressed(KeyboardKey::D)) { direction += mCamera->Right(); }

        if (glm::any(glm::epsilonNotEqual(direction, glm::zero<glm::vec3>(), glm::vec3(0.0001f))))
        {
            mKeyboardMoveDirection = glm::normalize(direction) * mInputScaleTimeAdjusted.KeyboardMovementScale;
        } 
        else
        {
            mKeyboardMoveDirection = glm::zero<glm::vec3>();
        }
    }

    void CameraInteractor::HandleMouseDrag()
    {
        glm::vec2 mouseDirection = mUserInput->MouseDelta() * mInputScaleTimeAdjusted.MouseMovementScale;

        if (mUserInput->IsMouseButtonPressed(0) && mUserInput->IsMouseButtonPressed(1)) 
        {
            if (IsMouseMovingVertically(mouseDirection))
            {
                mMouseMoveDirection = mCamera->Up() * mouseDirection.y;
            }
            else
            {
                mMouseMoveDirection = mCamera->Right() * mouseDirection.x;
            }
        }
        else if (mUserInput->IsAnyMouseButtonPressed() && mKeyboardMoveDirection != glm::zero<glm::vec3>()) 
        {
            // Acting like FPS-style camera with 'noclip' enabled
            mRotation = mouseDirection;
        }
        else if (mUserInput->IsMouseButtonPressed(0)) 
        {
            mRotation = IsMouseMovingVertically(mouseDirection) ?
                glm::vec2{ 0.0f, mouseDirection.y } : 
                glm::vec2{ mouseDirection.x, 0.0f };
        }
        else if (mUserInput->IsMouseButtonPressed(1)) 
        {
            mRotation = mouseDirection;
        }
        else if (mUserInput->IsMouseButtonPressed(2)) 
        {
            glm::vec3 up = mCamera->Up() * mouseDirection.y;
            glm::vec3 right = mCamera->Right() * mouseDirection.x;
            mMouseMoveDirection = up + right;
        }
    }

    void CameraInteractor::HandleMouseScroll()
    {
        glm::vec2 scrollDelta = mUserInput->ScrollDelta();
        glm::vec3 front = mCamera->Front() * scrollDelta.y * -mInputScaleTimeAdjusted.MouseMovementScale;
        glm::vec3 right = mCamera->Right() * scrollDelta.x * mInputScaleTimeAdjusted.MouseMovementScale;
        mMouseMoveDirection = front + right;
    }

    bool CameraInteractor::IsMouseMovingVertically(const glm::vec2& mouseDirection) const
    {
        return glm::angle(mouseDirection, glm::vec2(0.0, -1.0)) < M_PI_4 || glm::angle(mouseDirection, glm::vec2(0.0, 1.0)) < M_PI_4;
    }

    bool CameraInteractor::IsMouseMovingHorizontally(const glm::vec2& mouseDirection) const 
    {
        return glm::angle(mouseDirection, glm::vec2(-1.0, 0.0)) < M_PI_4 || glm::angle(mouseDirection, glm::vec2(1.0, 0.0)) < M_PI_4;
    }

    void CameraInteractor::PollInputs(uint64_t frameDeltaTimeMicroseconds)
    {
        if (frameDeltaTimeMicroseconds == 0)
        {
            return;
        }

        mInputScaleTimeAdjusted = mInputScale;
     /*   mInputScaleTimeAdjusted.KeyboardMovementScale /= frameDeltaTimeMicroseconds / 1000;
        mInputScaleTimeAdjusted.MouseMovementScale /= frameDeltaTimeMicroseconds / 1000;*/

        if (mIsEnabled) 
        {
            HandleMouseDrag();
            HandleMouseScroll();
            HandleKeyDown();
            mCamera->MoveBy(mKeyboardMoveDirection);
            mCamera->MoveBy(mMouseMoveDirection);
            mCamera->RotateBy(mRotation.y, mRotation.x);
            mCamera->SetViewportAspectRatio(mViewportSize.x / (mViewportSize.y + 1e-05));
        }

        mRotation = glm::zero<glm::vec2>();
        mMouseMoveDirection = glm::zero<glm::vec3>();
    }

}
