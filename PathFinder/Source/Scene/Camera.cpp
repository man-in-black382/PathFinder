#include "Camera.hpp"

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/matrix_transform.hpp>

namespace PathFinder
{

    Camera::Camera()
        : mPosition(glm::zero<glm::vec3>()),
        mFieldOfView(75),
        mNearClipPlane(0.1),
        mFarClipPlane(10),
        mViewportAspectRatio(16.f / 9.f),
        mWorldUp(glm::vec3(0, 1, 0)),
        mFront(glm::vec3(0, 0, 1)),
        mRight(glm::vec3(1, 0, 0)),
        mUp(glm::vec3(0, 1, 0)),
        mPitch(0),
        mYaw(-90.f),
        mMaximumPitch(85.f) {}

    Camera::Camera(float fieldOfView, float zNear, float zFar) : Camera()
    {
        mFieldOfView = fieldOfView;
        mNearClipPlane = zNear;
        mFarClipPlane = zFar;

        UpdateVectors();
    }

    void Camera::UpdateVectors()
    {
        mFront.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mFront.y = sin(glm::radians(mPitch));
        mFront.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mFront = glm::normalize(mFront);
        mRight = glm::normalize(glm::cross(mFront, mWorldUp));
        mUp = glm::normalize(glm::cross(mRight, mFront));
    }

    void Camera::MoveTo(const glm::vec3 &position)
    {
        mPosition = position;
    }

    void Camera::MoveBy(const glm::vec3 &translation)
    {
        mPosition += translation;
    }

    void Camera::LookAt(const glm::vec3 &point)
    {
        glm::vec3 direction = glm::normalize(point - mPosition);
        mPitch = glm::degrees(asin(-direction.y));
        mYaw = glm::degrees(atan2(direction.x, direction.z)) + 90;

        UpdateVectors();
    }

    void Camera::RotateTo(float pitch, float yaw)
    {
        mPitch = 0.f;
        mYaw = 0.f;

        RotateBy(pitch, yaw);
    }

    void Camera::RotateBy(float pitch, float yaw)
    {
        mPitch += pitch;
        mYaw += yaw;

        if (mPitch > mMaximumPitch) {
            mPitch = mMaximumPitch;
        }
        if (mPitch < -mMaximumPitch) {
            mPitch = -mMaximumPitch;
        }

        UpdateVectors();
    }

    void Camera::Zoom(float zoomFactor)
    {

    }

    void Camera::SetNearPlane(float nearPlane)
    {
        mNearClipPlane = std::min(mFarClipPlane, nearPlane);
    }

    void Camera::SetFarPlane(float farPlane)
    {
        mFarClipPlane = std::max(mNearClipPlane, farPlane);
    }

    //Ray3D Camera::rayFromPointOnViewport(const glm::vec2 &point, const GLViewport *viewport) const {
    //    glm::vec2 ndc = viewport->NDCFromPoint(point);
    //    glm::mat4 inverseVP = glm::inverse(viewProjectionMatrix());

    //    // -1.0 from NDC maps to near clip plane
    //    glm::vec4 nearPlanePoint = glm::vec4(ndc.x, ndc.y, -1.0, 1.0);

    //    // 1.0 from NDC maps to far clip plane
    //    glm::vec4 farPlanePoint = glm::vec4(ndc.x, ndc.y, 1.0, 1.0);

    //    glm::vec4 nearUntransformed = inverseVP * nearPlanePoint;
    //    nearUntransformed /= nearUntransformed.w;

    //    glm::vec4 farUntransformed = inverseVP * farPlanePoint;
    //    farUntransformed /= farUntransformed.w;

    //    return Ray3D(glm::vec3(nearUntransformed), glm::vec3(farUntransformed) - glm::vec3(nearUntransformed));
    //}

    glm::vec3 Camera::WorldToNDC(const glm::vec3 &v) const
    {
        glm::vec4 clipSpaceVector = ViewProjection() * glm::vec4(v, 1.0);
        return fabs(clipSpaceVector.w) > std::numeric_limits<float>::epsilon() ? clipSpaceVector / clipSpaceVector.w : clipSpaceVector;
    }

    const glm::vec3 &Camera::Position() const
    {
        return mPosition;
    }

    const glm::vec3 &Camera::Front() const
    {
        return mFront;
    }

    const glm::vec3 &Camera::Right() const
    {
        return mRight;
    }

    const glm::vec3 &Camera::Up() const
    {
        return mUp;
    }

    float Camera::NearClipPlane() const
    {
        return mNearClipPlane;
    }

    float Camera::FarClipPlane() const
    {
        return mFarClipPlane;
    }

    float Camera::FOVH() const
    {
        return mFieldOfView;
    }

    float Camera::FOVV() const
    {
        return mFieldOfView / mViewportAspectRatio;
    }

    glm::mat4 Camera::ViewProjection() const
    {
        return Projection() * View();
    }

    glm::mat4 Camera::View() const
    {
        return glm::lookAt(mPosition, mPosition + mFront, mWorldUp);
    }

    glm::mat4 Camera::Projection() const
    {
        float fovV = mFieldOfView / mViewportAspectRatio;
        return glm::perspective(glm::radians(fovV), mViewportAspectRatio, mNearClipPlane, mFarClipPlane);
    }

    glm::mat4 Camera::InverseViewProjection() const
    {
        return glm::inverse(ViewProjection());
    }

    glm::mat4 Camera::InverseView() const
    {
        return glm::inverse(View());
    }

    glm::mat4 Camera::InverseProjection() const
    {
        return glm::inverse(Projection());
    }

    void Camera::SetViewportAspectRatio(float aspectRatio)
    {
        mViewportAspectRatio = aspectRatio;
    }

}
