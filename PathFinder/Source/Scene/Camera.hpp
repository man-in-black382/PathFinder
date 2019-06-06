#pragma once

#include "../Geometry/Ray3D.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace PathFinder {

    class Camera {
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

        void updateVectors();

    public:
        Camera();

        Camera(float fieldOfView,
                float zNear,
                float zFar);

        void moveTo(const glm::vec3 &position);

        void moveBy(const glm::vec3 &translation);

        void lookAt(const glm::vec3 &point);

        void rotateTo(float pitch, float yaw);

        void rotateBy(float pitch, float yaw);

        void zoom(float zoomFactor);

        void setFarPlane(float farPlane);

        //Ray3D rayFromPointOnViewport(const glm::vec2 &point, const GLViewport *viewport) const;

        glm::vec3 worldToNDC(const glm::vec3 &v) const;

        const glm::vec3 &position() const;

        const glm::vec3 &front() const;

        const glm::vec3 &right() const;

        const glm::vec3 &up() const;

        float nearClipPlane() const;

        float farClipPlane() const;

        float FOVH() const;

        float FOVV() const;

        glm::mat4 viewProjectionMatrix() const;

        glm::mat4 viewMatrix() const;

        glm::mat4 projectionMatrix() const;

        glm::mat4 inverseViewProjectionMatrix() const;

        glm::mat4 inverseViewMatrix() const;

        glm::mat4 inverseProjectionMatrix() const;

        void setViewportAspectRatio(float aspectRatio);
    };

}
