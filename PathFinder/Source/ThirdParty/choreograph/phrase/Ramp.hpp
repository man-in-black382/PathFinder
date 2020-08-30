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

#include "choreograph/Phrase.hpp"
#include "choreograph/Easing.h"

///
/// \file
/// Ramp Phrases interpolate between two values over time.
/// They are analogous to a Tween.
///

namespace choreograph {

/// EaseFn's describes a one-dimensional transformation through time.
/// Choreograph accepts any function taking and returning a normalized float.
/// For a large number of ease functions, see Cinder's Easing.h
/// Generally, it is assumed that the following holds true for an EaseFn:
/// EaseFn( 0 ) = 0, EaseFn( 1 ) = 1.
    typedef std::function<float(float)> EaseFn;

//=================================================
// Basic Phrases.
//=================================================

///
/// RampTo is a phrase that interpolates all components with an ease function.
///
    template<typename T>
    class RampTo : public Phrase<T> {
    public:
        using LerpFn = std::function<T(const T &, const T &, float)>;

        RampTo(Time duration, const T &start_value, const T &end_value, const EaseFn &ease_fn = &easeNone, const LerpFn &lerp_fn = &lerpT<T>)
                :
                Phrase<T>(duration),
                _start_value(start_value),
                _end_value(end_value),
                _ease_fn(ease_fn),
                _lerp_fn(lerp_fn) {
        }

        /// Returns the interpolated value at the given time.
        T getValue(Time at_time) const override {
            return _lerp_fn(_start_value, _end_value, _ease_fn(this->normalizeTime(at_time)));
        }

        T getStartValue() const override {
            return _start_value;
        }

        T getEndValue() const override {
            return _end_value;
        }

        void setStartValue(const T &value) {
            _start_value = value;
        }

        void setEndValue(const T &value) {
            _end_value = value;
        }

        void setLerpFn(const LerpFn &lerp_fn) {
            _lerp_fn = lerp_fn;
        }

    private:
        T _start_value;
        T _end_value;
        EaseFn _ease_fn;
        LerpFn _lerp_fn;
    };

///
/// RampToN is a phrase template with N separately-interpolated components of the same type.
/// Allows for the use of separate ease functions per component.
/// All components must be of the same type.
/// If fewer than N ease functions are provided, the last ease function will be used to fill out remaining components.
/// If you only want one ease function for all components, use a normal RampTo.
///
    template<unsigned int SIZE, typename T>
    class RampToN : public Phrase<T> {

    public:

        template<typename... Args>
        RampToN(Time duration, const T &start_value, const T &end_value, Args &&... args):
                Phrase<T>(duration),
                _start_value(start_value),
                _end_value(end_value) {
            std::vector<EaseFn> list = {std::forward<Args>(args)...};
            for (size_t i = 0; i < SIZE && i < list.size(); ++i) {
                _ease_fns[i] = list[i];
            }
            for (size_t i = list.size(); i < SIZE; ++i) {
                _ease_fns[i] = list.back();
            }
        }

        /// Returns the interpolated value at the given time.
        T getValue(Time at_time) const override {
            Time t = this->normalizeTime(at_time);
            T out;
            for (int i = 0; i < SIZE; ++i) {
                out[i] = _componentLerpFn(_start_value[i], _end_value[i], _ease_fns[i](t));
            }
            return out;
        }

        T getStartValue() const override {
            return _start_value;
        }

        T getEndValue() const override {
            return _end_value;
        }

    private:
        using ComponentT = decltype(T().x); // get the type of the x component. decltype( T()[0] ) doesn't compile with glm's vecN unions.
        using ComponentLerpFn = std::function<ComponentT(const ComponentT &, const ComponentT &, float)>;

        ComponentLerpFn _componentLerpFn = &lerpT<ComponentT>;
        T _start_value;
        T _end_value;
        std::array<EaseFn, SIZE> _ease_fns;
    };

/// RampTo2 is a phrase with 2 separately-interpolated components.
    template<typename T>
    using RampTo2 = RampToN<2, T>;

/// RampTo3 is a phrase with 3 separately-interpolated components.
    template<typename T>
    using RampTo3 = RampToN<3, T>;

/// RampTo4 is a phrase with 4 separately-interpolated components.
    template<typename T>
    using RampTo4 = RampToN<4, T>;

} // namespace choreograph
