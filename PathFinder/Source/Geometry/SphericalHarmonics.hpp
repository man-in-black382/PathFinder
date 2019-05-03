#pragma once

//#include "Color.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
//#include <bitsery/bitsery.h>
//#include <bitsery/adapter/stream.h>

namespace Geometry {

    // An Efficient Representation for Irradiance Environment Maps using spherical harmonics
    // by Ravi Ramamoorthi & Pat Hanrahan
    // https://graphics.stanford.edu/papers/envmap/envmap.pdf

    // https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/

    //class SphericalHarmonics {
    //public:
    //    static constexpr float Y00 = 0.28209479177387814347f; // 1 / (2*sqrt(pi))
    //    static constexpr float Y11 = -0.48860251190291992159f; // sqrt(3 /(4pi))
    //    static constexpr float Y10 = 0.48860251190291992159f;
    //    static constexpr float Y1_1 = -0.48860251190291992159f;
    //    static constexpr float Y21 = -1.09254843059207907054f; // 1 / (2*sqrt(pi))
    //    static constexpr float Y2_1 = -1.09254843059207907054f;
    //    static constexpr float Y2_2 = 1.09254843059207907054f;
    //    static constexpr float Y20 = 0.31539156525252000603f; // 1/4 * sqrt(5/pi)
    //    static constexpr float Y22 = 0.54627421529603953527f; // 1/4 * sqrt(15/pi)

    //    static constexpr float CosineLobeBandFactors[] = {
    //            M_PI,
    //            2.0f * M_PI / 3.0f, 2.0f * M_PI / 3.0f, 2.0f * M_PI / 3.0f,
    //            M_PI / 4.0f, M_PI / 4.0f, M_PI / 4.0f, M_PI / 4.0f, M_PI / 4.0f
    //    };

    //private:
    //    glm::vec3 mL00 = glm::zero<glm::vec3>();
    //    glm::vec3 mL11 = glm::zero<glm::vec3>();
    //    glm::vec3 mL10 = glm::zero<glm::vec3>();
    //    glm::vec3 mL1_1 = glm::zero<glm::vec3>();
    //    glm::vec3 mL21 = glm::zero<glm::vec3>();
    //    glm::vec3 mL2_1 = glm::zero<glm::vec3>();
    //    glm::vec3 mL2_2 = glm::zero<glm::vec3>();
    //    glm::vec3 mL20 = glm::zero<glm::vec3>();
    //    glm::vec3 mL22 = glm::zero<glm::vec3>();

    //public:
    //    SphericalHarmonics() = default;

    //    SphericalHarmonics(const glm::vec3 &direction, const Color &color);

    //    const glm::vec3 &L00() const;

    //    const glm::vec3 &L11() const;

    //    const glm::vec3 &L10() const;

    //    const glm::vec3 &L1_1() const;

    //    const glm::vec3 &L21() const;

    //    const glm::vec3 &L2_1() const;

    //    const glm::vec3 &L2_2() const;

    //    const glm::vec3 &L20() const;

    //    const glm::vec3 &L22() const;

    //    void convolve();

    //    float magnitude() const;

    //    float magnitude2() const;

    //    void contribute(const glm::vec3 &direction, const glm::vec3 &value, float weight);

    //  //  void contribute(const glm::vec3 &direction, const Color &color, float weight);

    //    void scale(const glm::vec3 &scaleFactors);

    //    glm::vec3 evaluate(const glm::vec3 &direction) const;

  /*      template<typename S>
        void serialize(S &s) {
            s.object(mL00);
            s.object(mL11);
            s.object(mL10);
            s.object(mL1_1);
            s.object(mL21);
            s.object(mL2_1);
            s.object(mL2_2);
            s.object(mL20);
            s.object(mL22);
        }*/
    };


}
