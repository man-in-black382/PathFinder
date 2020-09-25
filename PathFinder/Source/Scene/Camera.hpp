#pragma once

#include "../Geometry/Ray3D.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace PathFinder
{

    using FStop = float;
    using ISO = float;
    using EV = float;

    class Camera
    {
    public:
        Camera();

        glm::mat4 ViewProjection() const;
        glm::mat4 View() const;
        glm::mat4 Projection() const;
        glm::mat4 InverseViewProjection() const;
        glm::mat4 InverseView() const;
        glm::mat4 InverseProjection() const;
        EV ExposureValue100() const;

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

    public:
        inline const glm::vec3& Position() const { return mPosition; }
        inline const glm::vec3& Front() const { return mFront; }
        inline const glm::vec3& Right() const { return mRight; }
        inline const glm::vec3& Up() const { return mUp; }
        inline float NearClipPlane() const { return mNearClipPlane; }
        inline float FarClipPlane() const { return mFarClipPlane; }
        inline float FOVH() const { return mFieldOfView; }
        inline float FOVV() const { return mFieldOfView / mViewportAspectRatio; }
        inline FStop Aperture() const { return mLenseAperture; }
        inline ISO FilmSpeed() const { return mFilmSpeed; }
        inline float ShutterTime() const { return mShutterTime; }
        inline float AspectRatio() const { return mViewportAspectRatio; }
    };

}
