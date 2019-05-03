#pragma once

namespace Geometry {

    struct Interval {
        float min = 0;
        float max = 0;

        Interval() = default;

        Interval(float min, float max);
    };

}
