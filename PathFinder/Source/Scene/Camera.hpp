#pragma once

#include "../Geometry/Ray3D.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace PathFinder
{

    class Camera
    {
    public:
        Camera();
        Camera(float fieldOfView, float zNear, float zFar);

        void MoveTo(const glm::vec3 &position);
        void MoveBy(const glm::vec3 &translation);
        void LookAt(const glm::vec3 &point);
        void RotateTo(float pitch, float yaw);
        void RotateBy(float pitch, float yaw);
        void Zoom(float zoomFactor);
        void SetNearPlane(float nearPlane);
        void SetFarPlane(float farPlane);
        void SetViewportAspectRatio(float aspectRatio);

        //Ray3D rayFromPointOnViewport(const glm::vec2 &point, const GLViewport *viewport) const;

        glm::vec3 WorldToNDC(const glm::vec3 &v) const;
        const glm::vec3& Position() const;
        const glm::vec3& Front() const;
        const glm::vec3& Right() const;
        const glm::vec3& Up() const;
        float NearClipPlane() const;
        float FarClipPlane() const;
        float FOVH() const;
        float FOVV() const;
        glm::mat4 ViewProjection() const;
        glm::mat4 View() const;
        glm::mat4 Projection() const;
        glm::mat4 InverseViewProjection() const;
        glm::mat4 InverseView() const;
        glm::mat4 InverseProjection() const;

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

        void UpdateVectors();
    };

}
