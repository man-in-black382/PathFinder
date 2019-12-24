#include "CameraInteractor.hpp"
#include "../Foundation/Pi.hpp"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace PathFinder 
{

    CameraInteractor::CameraInteractor(Camera* camera, Input* userInput)
        : mCamera{ camera }, mUserInput{ userInput }
    {
        mUserInput->GetKeyboardEvent()[Input::KeyboardAction::KeyDown] += {"cameraman.key.down", this, & CameraInteractor::HandleKeyDown};
        mUserInput->GetKeyboardEvent()[Input::KeyboardAction::KeyUp] += {"cameraman.key.up", this, & CameraInteractor::HandleKeyUp};
        mUserInput->GetSimpleMouseEvent()[Input::SimpleMouseAction::Drag] += {"cameraman.mouse.drag", this, & CameraInteractor::HandleMouseDrag};
        mUserInput->GetSimpleMouseEvent()[Input::SimpleMouseAction::PressDown] += {"cameraman.mouse.down", this, & CameraInteractor::HandleMouseDown};
        mUserInput->GetScrollMouseEvent() += {"cameraman.mouse.scroll", this, & CameraInteractor::HandleMouseScroll};
    }

    CameraInteractor::~CameraInteractor()
    {
        mUserInput->GetKeyboardEvent()[Input::KeyboardAction::KeyDown] -= "cameraman.key.down";
        mUserInput->GetKeyboardEvent()[Input::KeyboardAction::KeyUp] -= "cameraman.key.up";
        mUserInput->GetSimpleMouseEvent()[Input::SimpleMouseAction::Drag] -= "cameraman.mouse.drag";
        mUserInput->GetSimpleMouseEvent()[Input::SimpleMouseAction::PressDown] -= "cameraman.mouse.down";
        mUserInput->GetScrollMouseEvent() -= "cameraman.mouse.scroll";
    }

    bool CameraInteractor::IsEnabled() const
    {
        return mIsEnabled;
    }

    void CameraInteractor::SetIsEnabled(bool enabled)
    {
        mIsEnabled = enabled;
    }

    void CameraInteractor::HandleKeyDown(const Input* input) 
    {
        glm::vec3 direction = glm::zero<glm::vec3>();

        if (input->IsKeyPressed(Input::Key::W)) { direction += mCamera->Front(); }
        if (input->IsKeyPressed(Input::Key::A)) { direction -= mCamera->Right(); }
        if (input->IsKeyPressed(Input::Key::S)) { direction -= mCamera->Front(); }
        if (input->IsKeyPressed(Input::Key::D)) { direction += mCamera->Right(); }

        mKeyboardMoveDirection = glm::length(direction) > std::numeric_limits<float>::epsilon() ? glm::normalize(direction) : direction;
        mKeyboardMoveDirection *= 0.05f;
    }

    void CameraInteractor::HandleKeyUp(const Input* input) 
    {
        if (input->IsKeyPressed(Input::Key::W)) { return; }
        if (input->IsKeyPressed(Input::Key::A)) { return; }
        if (input->IsKeyPressed(Input::Key::S)) { return; }
        if (input->IsKeyPressed(Input::Key::D)) { return; }

        mKeyboardMoveDirection = glm::zero<glm::vec3>();
    }

    void CameraInteractor::HandleMouseDrag(const Input* input)
    {
        glm::vec2 mouseDirection = input->MousePosition() - mPreviousMousePosition;

        if (input->IsMouseButtonPressed(0) && input->IsMouseButtonPressed(1)) {
            if (mMouseLockDirection == glm::zero<glm::vec2>()) {
                mMouseLockDirection = IsMouseMovingVertically(mouseDirection) ? glm::vec2(0.0, 1.0) : glm::vec2(1.0, 0.0);
            }
            if (mMouseLockDirection.x == 0.0) {
                mMouseMoveDirection = mCamera->Up() * mouseDirection.y * 0.005f;
            }
            else {
                mMouseMoveDirection = mCamera->Right() * mouseDirection.x * 0.005f;
            }
        }
        else if (input->PressedMouseButtonsMask() && mKeyboardMoveDirection != glm::zero<glm::vec3>()) {
            // Acting like FPS-style camera with 'noclip' enabled
            mRotation = mouseDirection;
        }
        else if (input->IsMouseButtonPressed(0)) {
            if (mMouseLockDirection == glm::zero<glm::vec2>()) {
                mMouseLockDirection = IsMouseMovingVertically(mouseDirection) ? glm::vec2(0.0, 1.0) : glm::vec2(1.0, 0.0);
            }
            if (mMouseLockDirection.x == 0.0) {
                mMouseMoveDirection = mCamera->Front() * mouseDirection.y * 0.005f;
            }
            else {
                mRotation = mouseDirection;
                mRotation.y = 0.0;
            }
        }
        else if (input->IsMouseButtonPressed(1)) {
            mRotation = mouseDirection;
        }
        else if (input->IsMouseButtonPressed(2)) {
            glm::vec3 up = mCamera->Up() * mouseDirection.y * 0.005f;
            glm::vec3 right = mCamera->Right() * mouseDirection.x * 0.005f;
            mMouseMoveDirection = up + right;
        }

        mPreviousMousePosition = input->MousePosition();
    }

    void CameraInteractor::HandleMouseScroll(const Input* input)
    {
        glm::vec2 scrollDelta = input->ScrollDelta();
        glm::vec3 front = mCamera->Front() * scrollDelta.y * -0.005f;
        glm::vec3 right = mCamera->Right() * scrollDelta.x * 0.005f;
        mMouseMoveDirection = front + right;
    }

    void CameraInteractor::HandleMouseDown(const Input* input) {
        mPreviousMousePosition = input->MousePosition();
        mMouseLockDirection = glm::zero<glm::vec2>();
    }

    bool CameraInteractor::IsMouseMovingVertically(const glm::vec2& mouseDirection) const
    {
        return glm::angle(mouseDirection, glm::vec2(0.0, -1.0)) < M_PI_4 || glm::angle(mouseDirection, glm::vec2(0.0, 1.0)) < M_PI_4;
    }

    bool CameraInteractor::IsMouseMovingHorizontally(const glm::vec2& mouseDirection) const 
    {
        return glm::angle(mouseDirection, glm::vec2(-1.0, 0.0)) < M_PI_4 || glm::angle(mouseDirection, glm::vec2(1.0, 0.0)) < M_PI_4;
    }

    void CameraInteractor::UpdateCamera() 
    {
        if (mIsEnabled) {
            mCamera->MoveBy(mKeyboardMoveDirection);
            mCamera->MoveBy(mMouseMoveDirection);
            mCamera->RotateBy(mRotation.y, mRotation.x);
            //mCamera->setViewportAspectRatio(mViewport->aspectRatio());
        }

        mRotation = glm::zero<glm::vec2>();
        mMouseMoveDirection = glm::zero<glm::vec3>();
    }

}
