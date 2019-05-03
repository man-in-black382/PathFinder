////
////  SphericalHarmonics.cpp
////  Geometry
////
////  Created by Pavel Muratov on 11/20/17.
////  Copyright © 2017 MPO. All rights reserved.
////
//
//#include "SphericalHarmonics.hpp"
//
//#include <glm/glm.hpp>
//#include <glm/gtc/epsilon.hpp>
//
//namespace Geometry {
//
//#pragma mark - Lifecycle
//
//    SphericalHarmonics::SphericalHarmonics(const glm::vec3 &direction, const Color &color) {
//        contribute(direction, color, 4.0 * M_PI);
//    }
//
//#pragma mark - Getters
//
//    const glm::vec3 &SphericalHarmonics::L00() const {
//        return mL00;
//    }
//
//    const glm::vec3 &SphericalHarmonics::L11() const {
//        return mL11;
//    }
//
//    const glm::vec3 &SphericalHarmonics::L10() const {
//        return mL10;
//    }
//
//    const glm::vec3 &SphericalHarmonics::L1_1() const {
//        return mL1_1;
//    }
//
//    const glm::vec3 &SphericalHarmonics::L21() const {
//        return mL21;
//    }
//
//    const glm::vec3 &SphericalHarmonics::L2_1() const {
//        return mL2_1;
//    }
//
//    const glm::vec3 &SphericalHarmonics::L2_2() const {
//        return mL2_2;
//    }
//
//    const glm::vec3 &SphericalHarmonics::L20() const {
//        return mL20;
//    }
//
//    const glm::vec3 &SphericalHarmonics::L22() const {
//        return mL22;
//    }
//
//#pragma mark -
//
//    float SphericalHarmonics::magnitude() const {
//        return std::sqrt(magnitude2());
//    }
//
//    float SphericalHarmonics::magnitude2() const {
//        return glm::dot(mL00, mL00) +
//                glm::dot(mL1_1, mL1_1) +
//                glm::dot(mL10, mL10) +
//                glm::dot(mL11, mL11) +
//                glm::dot(mL2_2, mL2_2) +
//                glm::dot(mL2_1, mL2_1) +
//                glm::dot(mL21, mL21) +
//                glm::dot(mL20, mL20) +
//                glm::dot(mL22, mL22);
//    }
//
//    void SphericalHarmonics::convolve() {
//        mL00 *= CosineLobeBandFactors[0];
//
//        mL1_1 *= CosineLobeBandFactors[1];
//        mL10 *= CosineLobeBandFactors[2];
//        mL11 *= CosineLobeBandFactors[3];
//
//        mL2_2 *= CosineLobeBandFactors[4];
//        mL2_1 *= CosineLobeBandFactors[5];
//        mL21 *= CosineLobeBandFactors[6];
//        mL20 *= CosineLobeBandFactors[7];
//        mL22 *= CosineLobeBandFactors[8];
//    }
//
//    void SphericalHarmonics::contribute(const glm::vec3 &direction, const glm::vec3 &value, float weight) {
//        // l, m = 0, 0
//        mL00 += value * Y00 * weight;
//
//        // l, m = 1, -1
//        mL1_1 += value * Y1_1 * direction.y * weight;
//        // l, m = 1, 0
//        mL10 += value * Y10 * direction.z * weight;
//        // l, m = 1, 1
//        mL11 += value * Y11 * direction.x * weight;
//
//        // l, m = 2, -2
//        mL2_2 += value * Y2_2 * (direction.x * direction.y) * weight;
//        // l, m = 2, -1
//        mL2_1 += value * Y2_1 * (direction.y * direction.z) * weight;
//        // l, m = 2, 1
//        mL21 += value * Y21 * (direction.x * direction.z) * weight;
//
//        // l, m = 2, 0
//        mL20 += value * Y20 * (3.0f * direction.z * direction.z - 1.0f) * weight;
//
//        // l, m = 2, 2
//        mL22 += value * Y22 * (direction.x * direction.x - direction.y * direction.y) * weight;
//    }
//
//    void SphericalHarmonics::contribute(const glm::vec3 &direction, const Color &color, float weight) {
//        contribute(direction, color.rgb(), weight);
//    }
//
//    void SphericalHarmonics::scale(const glm::vec3 &scaleFactors) {
//        mL00 *= scaleFactors;
//
//        mL1_1 *= scaleFactors;
//        mL10 *= scaleFactors;
//        mL11 *= scaleFactors;
//
//        mL2_2 *= scaleFactors;
//        mL2_1 *= scaleFactors;
//        mL21 *= scaleFactors;
//        mL20 *= scaleFactors;
//        mL22 *= scaleFactors;
//    }
//
//    glm::vec3 SphericalHarmonics::evaluate(const glm::vec3 &direction) const {
//        glm::vec3 result(0.0);
//
//        result += mL00 * Y00;
//
//        result += mL1_1 * Y1_1 * direction.y;
//        result += mL10 * Y10 * direction.z;
//        result += mL11 * Y11 * direction.x;
//
//        result += mL2_2 * Y2_2 * (direction.x * direction.y);
//        result += mL2_1 * Y2_1 * (direction.y * direction.z);
//        result += mL21 * Y21 * (direction.x * direction.z);
//        result += mL20 * Y20 * (3.0f * direction.z * direction.z - 1.0f);
//        result += mL22 * Y22 * (direction.x * direction.x - direction.y * direction.y);
//
//        return result;
//    }
//
//}
