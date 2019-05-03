//
//  Size.cpp
//  Geometry
//
//  Created by Pavlo Muratov on 16.06.17.
//  Copyright © 2017 MPO. All rights reserved.
//

#include "Size2D.hpp"
#include <algorithm>

namespace Geometry {

    const Size2D &Size2D::zero() {
        static Size2D zero = { 0, 0 };
        return zero;
    }

    const Size2D &Size2D::unit() {
        static Size2D unit = { 1, 1 };
        return unit;
    }

    Size2D::Size2D(float w, float h)
        :
        width(w),
        height(h) {
    }

    Size2D::Size2D(float side)
        :
        width(side),
        height(side) {
    }

    bool Size2D::operator==(const Size2D &rhs) {
        return width == rhs.width && height == rhs.height;
    }

    bool Size2D::operator!=(const Size2D &rhs) {
        return !operator==(rhs);
    }

    Size2D Size2D::transformedBy(const glm::vec2 &vector) const {
        return Size2D(width * vector.x, height * vector.y);
    }

    Size2D Size2D::makeUnion(const Size2D &size) {
        return { std::max(width, size.width), std::max(height, size.height) };
    }

}
