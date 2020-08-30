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

///
/// \file
/// Retime Phrases take another Phrase and reinterpret the time at which
/// that Phrase's value is computed.
///

namespace choreograph {

///
/// LoopPhrase repeats an existing Phrase N times.
///
    template<typename T>
    class LoopPhrase : public Phrase<T> {
    public:
        /// Create a Phrase that loops \a source \a numLoops times.
        LoopPhrase(const PhraseRef <T> &source, float numLoops, Time inflectionPoint = 0.0f) :
                Phrase<T>(source->getDuration() * numLoops),
                _source(source),
                _inflection_point(inflectionPoint) {
        }

        T getValue(Time atTime) const override {
            return _source->getValueWrapped(atTime, _inflection_point);
        }

        T getStartValue() const override {
            return _source->getStartValue();
        }

        T getEndValue() const override {
            return _source->getValueWrapped(this->getDuration());
        }

    private:
        PhraseRef <T> _source;
        Time _inflection_point;
    };

///
/// PingPongPhrase repeats an existing Phrase N times,
/// playing forward from start to end, then playing
/// in reverse from end to start.
///
    template<typename T>
    class PingPongPhrase : public Phrase<T> {
    public:
        /// Create a Phrase that loops \a source \a numLoops times.
        PingPongPhrase(const PhraseRef <T> &source, float numLoops, Time inflectionPoint = 0.0f) :
                Phrase<T>(source->getDuration() * numLoops),
                _source(source),
                _inflection_point(inflectionPoint) {
        }

        T getValue(Time atTime) const override {
            bool forward = (int) (atTime / _source->getDuration()) % 2 == 0;
            Time insetTime = std::fmod(atTime, _source->getDuration());
            if (forward) {
                return _source->getValue(insetTime);
            } else {
                return _source->getValue(_source->getDuration() - insetTime);
            }
        }

        T getStartValue() const override {
            return _source->getStartValue();
        }

        T getEndValue() const override {
            return getValue(this->getDuration());
        }

    private:
        PhraseRef <T> _source;
        Time _inflection_point;
    };

///
/// ReversePhrase plays an existing Phrase in reverse.
///
    template<typename T>
    class ReversePhrase : public Phrase<T> {
    public:
        /// Create a Phrase that loops \a source \a numLoops times.
        ReversePhrase(const PhraseRef <T> &source) :
                Phrase<T>(source->getDuration()),
                _source(source) {
        }

        T getValue(Time atTime) const override {
            return _source->getValue(_source->getDuration() - atTime);
        }

        T getStartValue() const override {
            return _source->getEndValue();
        }

        T getEndValue() const override {
            return _source->getStartValue();
        }

    private:
        PhraseRef <T> _source;
    };

///
/// ClipPhrase cuts off an existing Phrase at some point in time.
/// End time values are clamped to within the original Phrase's valid range.
/// Begin and end times should be non-negative.
///
    template<typename T>
    class ClipPhrase : public Phrase<T> {
    public:
        ClipPhrase(const PhraseRef <T> &source, Time begin, Time end) :
                Phrase<T>(end - begin),
                _begin(begin),
                _end(end),
                _source(source) {
        }

        T getValue(Time atTime) const override {
            return _source->getValue(clampTime(_begin + atTime));
        }

        Time clampTime(Time t) const {
            return std::min(std::min(t, _source->getDuration()), _end);
        }

    private:
        PhraseRef <T> _source;
        Time _begin;
        Time _end;
    };

    template<typename T>
    class SquashPhrase : public Phrase<T> {
    public:
        SquashPhrase(const PhraseRef <T> &source, Time duration) :
                Phrase<T>(duration),
                _source_duration(_source->getDuration()),
                _new_duration(duration) {
        }

        T getValue(Time atTime) const override {
            return _source->getValue(stretchTime(atTime));
        }

        Time stretchTime(Time t) const {
            return (t / _source_duration) * _new_duration;
        }

    private:
        PhraseRef <T> _source;
        Time _new_duration;
        Time _source_duration;
    };

} // namespace choreograph
