#pragma once

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <bitsery/bitsery.h>
#include <Utility/SerializationAdapters.hpp>

namespace Geometry 
{

    struct Transformation
    {
        glm::vec3 Scale;
        glm::vec3 Translation;
        glm::quat Rotation;

        Transformation();
        Transformation(const glm::mat4 &matrix);
        Transformation(glm::vec3 scale, glm::vec3 translation, glm::quat rotation);
        Transformation CombinedWith(const Transformation &other) const;

        glm::mat4 ModelMatrix() const;
        glm::mat4 ScaleMatrix() const;
        glm::mat4 RotationMatrix() const;
        glm::mat4 TranslationMatrix() const;
        glm::mat4 NormalMatrix() const;
        glm::mat4 InverseScaleMatrix() const;
        glm::mat4 InverseRotationMatrix() const;
        glm::mat4 InverseTranslationMatrix() const;
    };

    template <typename S>
    void serialize(S& s, Transformation& t)
    {
        s.object(t.Scale);
        s.object(t.Translation);
        s.object(t.Rotation);
    }

}
