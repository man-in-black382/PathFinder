#pragma once

#include <bitsery/bitsery.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>

namespace bitsery
{

    template <typename S>
    void serialize(S& s, glm::vec2& v)
    {
        s.value4b(v.x);
        s.value4b(v.y);
    }

    template <typename S>
    void serialize(S& s, glm::vec3& v)
    {
        s.value4b(v.x);
        s.value4b(v.y);
        s.value4b(v.z);
    }

    template <typename S>
    void serialize(S& s, glm::vec4& v) 
    {
        s.value4b(v.x);
        s.value4b(v.y);
        s.value4b(v.z);
        s.value4b(v.w);
    }

    template <typename S>
    void serialize(S& s, glm::quat& q)
    {
        s.value4b(q.x);
        s.value4b(q.y);
        s.value4b(q.z);
        s.value4b(q.w);
    }

    template <typename S>
    void serialize(S& s, glm::mat4& m)
    {
        s.value4b(m[0][0]); s.value4b(m[0][1]); s.value4b(m[0][2]); s.value4b(m[0][3]);
        s.value4b(m[1][0]); s.value4b(m[1][1]); s.value4b(m[1][2]); s.value4b(m[1][3]);
        s.value4b(m[2][0]); s.value4b(m[2][1]); s.value4b(m[2][2]); s.value4b(m[2][3]);
        s.value4b(m[3][0]); s.value4b(m[3][1]); s.value4b(m[3][2]); s.value4b(m[3][3]);
    }

}
