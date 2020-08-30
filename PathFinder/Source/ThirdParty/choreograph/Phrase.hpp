/*
 * Copyright (c) 2014 David Wicks, sansumbrella.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other mCookTorranceMaterials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "TimeType.h"

namespace choreograph {

    template<typename T>
    class Phrase;

    template<typename T>
    using PhraseRef = std::shared_ptr<Phrase<T>>;

    template<typename T>
    using PhraseUniqueRef = std::unique_ptr<Phrase<T>>;

/// The default templated linear interpolation function.
    template<typename T>
    T lerpT(const T &a, const T &b, float t) {
        return a + (b - a) * t;
    }

///
/// A Phrase of motion.
/// Virtual base class with concept of value and implementation of time.
///
/// A concrete Phrase is a part of a Sequence.
/// It describes a value over time.
///
    template<typename T>
    class Phrase {
    public:
        Phrase(Time duration) :
                _duration(duration) {
        }

        virtual ~Phrase() = default;

        //=================================================
        // Virtual Interface.
        //=================================================

        /// Override to provide value at requested time.
        /// Returns the interpolated value at the given time.
        virtual T getValue(Time at_time) const = 0;

        /// Override to provide value at start (and before).
        virtual T getStartValue() const {
            return getValue(0);
        }

        /// Override to provide value at end (and beyond).
        virtual T getEndValue() const {
            return getValue(getDuration());
        }

        //=================================================
        // Time querying.
        //=================================================

        /// Returns normalized time if t is in range [start_time, end_time]. Does not clamp output range.
        inline Time normalizeTime(Time t) const {
            return t / _duration;
        }

        /// Returns the duration of this source.
        inline Time getDuration() const {
            return _duration;
        }

        /// Returns the Phrase value at \a time, looping past the end from inflection point to the end.
        /// Relies on the subclass implementation of getValue( t ).
        T getValueWrapped(Time time, Time inflectionPoint = 0.0f) const {
            return getValue(wrapTime(time, getDuration(), inflectionPoint));
        }

    private:
        const Time _duration = 0;
    };

} // namespace choreograph
