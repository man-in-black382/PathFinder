#include "Camera.hpp"

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
        mMaximumPitch(85.f) 
    {
        UpdateVectors();
    }

    void Camera::UpdateVectors()
    {
        mFront.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mFront.y = sin(glm::radians(mPitch));
        mFront.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mFront = glm::normalize(mFront);
        mRight = glm::normalize(glm::cross(mWorldUp, mFront));
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
        mPitch = glm::degrees(asin(direction.y));
        mYaw = glm::degrees(atan2(direction.x, -direction.z)) - 90;

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
        mYaw -= yaw;

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

    glm::vec3 Camera::WorldToNDC(const glm::vec3 &v) const
    {
        glm::vec4 clipSpaceVector = ViewProjection() * glm::vec4(v, 1.0);
        return fabs(clipSpaceVector.w) > std::numeric_limits<float>::epsilon() ? clipSpaceVector / clipSpaceVector.w : clipSpaceVector;
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

    PathFinder::EV Camera::ExposureValue100() const
    {
        // EV number is defined as:
        // 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
        // This gives
        // EV_s = log2 (N^2 / t)
        // EV_100 + log2 (S /100) = log2 (N^2 / t)
        // EV_100 = log2 (N^2 / t) - log2 (S /100)
        // EV_100 = log2 (N^2 / t . 100 / S)
        return std::log2((mLenseAperture * mLenseAperture) / mShutterTime * 100.0 / mFilmSpeed);
    }

    void Camera::SetViewportAspectRatio(float aspectRatio)
    {
        mViewportAspectRatio = aspectRatio;
    }

    void Camera::SetAperture(FStop aperture)
    {
        mLenseAperture = aperture;
    }

    void Camera::SetFilmSpeed(ISO filmSpeed)
    {
        mFilmSpeed = filmSpeed;
    }

    void Camera::SetShutterTime(float time)
    {
        mShutterTime = time;
    }

    void Camera::SetFieldOfView(float degrees)
    {
        mFieldOfView = degrees;
    }

}
