#pragma once

#include <Geometry/Ray3D.hpp>
#include <bitsery/bitsery.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <Utility/SerializationAdapters.hpp>

namespace PathFinder
{

    using FStop = float;
    using ISO = float;
    using EV = float;

    class Camera
    {
    public:
        Camera();

        glm::mat4 GetViewProjection() const;
        glm::mat4 GetView() const;
        glm::mat4 GetProjection() const;
        glm::mat4 GetInverseViewProjection() const;
        glm::mat4 GetInverseView() const;
        glm::mat4 GetInverseProjection() const;
        EV GetExposureValue100() const;

        void MoveTo(const glm::vec3 &position);
        void MoveBy(const glm::vec3 &translation);
        void LookAt(const glm::vec3 &point);
        void RotateTo(float pitch, float yaw);
        void RotateBy(float pitch, float yaw);
        void Zoom(float zoomFactor);
        void SetNearPlane(float nearPlane);
        void SetFarPlane(float farPlane);
        void SetViewportAspectRatio(float aspectRatio);
        void SetAperture(FStop aperture);
        void SetFilmSpeed(ISO filmSpeed);
        void SetShutterTime(float time);
        void SetFieldOfView(float degrees);

        glm::vec3 WorldToNDC(const glm::vec3 &v) const;
        std::array<glm::vec3, 8> GetFrustumCorners() const;

    private:
        glm::vec3 mFront;
        glm::vec3 mRight;
        glm::vec3 mUp;
        glm::vec3 mWorldUp;
        glm::vec3 mPosition;

        float mMaximumPitch;
        float mPitch;
        float mYaw;
        float mFieldOfView;
        float mNearClipPlane;
        float mFarClipPlane;
        float mViewportAspectRatio;

        /// Relative aperture. Controls how wide the aperture is opened. Impacts the depth of field.
        FStop mLenseAperture;

        /// Sensor sensitivity/gain. Controls how photons are counted / quantized on the digital sensor.
        ISO mFilmSpeed;

        /// Controls how long the aperture is opened. Impacts the motion blur.
        float mShutterTime;

        void UpdateVectors();

        friend bitsery::Access;

        template <typename S>
        void serialize(S& s)
        {
            s.object(mFront);
            s.object(mRight);
            s.object(mUp);
            s.object(mWorldUp);
            s.object(mPosition);
            s.value4b(mMaximumPitch);
            s.value4b(mPitch);
            s.value4b(mYaw);
            s.value4b(mFieldOfView);
            s.value4b(mNearClipPlane);
            s.value4b(mFarClipPlane);
            s.value4b(mViewportAspectRatio);
            s.value4b(mLenseAperture);
            s.value4b(mFilmSpeed);
            s.value4b(mShutterTime);
        }

    public:
        inline const glm::vec3& GetPosition() const { return mPosition; }
        inline const glm::vec3& GetFront() const { return mFront; }
        inline const glm::vec3& GetRight() const { return mRight; }
        inline const glm::vec3& GetUp() const { return mUp; }
        inline float GetNearClipPlane() const { return mNearClipPlane; }
        inline float GetFarClipPlane() const { return mFarClipPlane; }
        inline float GetFOVH() const { return mFieldOfView; }
        inline float GetFOVV() const { return mFieldOfView / mViewportAspectRatio; }
        inline FStop GetAperture() const { return mLenseAperture; }
        inline ISO GetFilmSpeed() const { return mFilmSpeed; }
        inline float GetShutterTime() const { return mShutterTime; }
        inline float GetAspectRatio() const { return mViewportAspectRatio; }
    };

}
