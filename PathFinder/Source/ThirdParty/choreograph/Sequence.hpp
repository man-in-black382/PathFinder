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

#include "Phrase.hpp"
#include "phrase/Hold.hpp"
#include "phrase/Retime.hpp"
#include <assert.h>

namespace choreograph {

    template<typename T>
    class SequencePhrase;

    template<typename T>
    using SequencePhraseRef = std::shared_ptr<SequencePhrase<T>>;

    template<typename T>
    class Sequence;

    template<typename T>
    using SequenceRef = std::shared_ptr<Sequence<T>>;

    template<typename T>
    using SequenceUniqueRef = std::unique_ptr<Sequence<T>>;

///
/// A Sequence of motions.
/// Our essential compositional tool, describing all the transformations to one element.
/// A kind of platonic idea of an animation sequence; this describes a motion without giving it an output.
///
    template<typename T>
    class Sequence {
    public:
        /// Sequences always need to have some valid value, so we can't start undefined.
        Sequence() = delete;

        /// Construct a Sequence with an initial \a value.
        explicit Sequence(const T &value) :
                _initial_value(value) {
        }

        /// Construct a Sequence with an initial \a value.
        explicit Sequence(T &&value) :
                _initial_value(std::move(value)) {
        }

        // Default copy and move assignment and construction work fine.
        // VS2013 doesn't like the default-declared move, so we omit it.
        Sequence(const Sequence<T> &other) = default;

        // Sequence( Sequence<T> &&other ) = default;
        Sequence &operator=(const Sequence<T> &rhs) = default;
        // Sequence& operator= (Sequence<T> &&rhs) = default;

        /// Construct a Sequence from a single Phrase.
        explicit Sequence(const PhraseRef<T> &phrase) :
                _phrases(1, phrase),
                _initial_value(phrase->getStartValue()),
                _duration(phrase->getDuration()) {
        }

        /// Construct a Sequence from a vector of phrases.
        /// A bug in VS2013 causes this constructor to be called when you meant to use
        /// the single-phrase constructor. Cast to PhraseRef<T> to get around it.
        explicit Sequence(const std::vector<PhraseRef<T>> &phrases) :
                _phrases(phrases),
                _initial_value(phrases.front()->getStartValue()),
                _duration(calcDuration()) {
        }

        //
        // Sequence manipulation and expansion.
        //

        /// Set the end \a value of Sequence.
        /// If there are no Phrases, this is the initial value.
        /// Otherwise, this is an instantaneous hold at \a value.
        Sequence<T> &set(const T &value);

        /// Append a Phrase to the sequence.
        /// Constructs a Phrase starting with Sequence's end value and
        /// ending with \a value after \a duration.
        /// Forwards additional arguments to the end of the Phrase constructor.
        /// Example calls look like:
        /// sequence.then<RampTo>( targetValue, duration, EaseInOutQuad() ).then<Hold>( holdValue, duration );
        template<template<typename> class PhraseT, typename... Args>
        Sequence<T> &then(const T &value, Time duration, Args &&... args);

        /// Append an existing phrase to the Sequence.
        Sequence<T> &then(const PhraseRef<T> &phrase_ptr);

        /// Append all Phrases from another Sequence to this Sequence.
        Sequence<T> &then(const Sequence<T> &next);

        //
        // Sequence conversion.
        //

        /// Returns a Phrase that encapsulates this Sequence.
        /// Duplicates the Sequence, so future changes to this do not affect the Phrase.
        PhraseRef<T> asPhrase() const {
            return std::make_shared<SequencePhrase<T>>(*this);
        }

        /// Returns a Sequence containing the phrases between Times from and to.
        /// Partial phrases at the beginning and end are wrapped in ClipPhrases.
        Sequence slice(Time from, Time to) const;

        /// Splices a collection of Phrases into the sequence at \a start_index.
        /// Removes the specified number of Phrases starting with the phrase at start_index.
        void splice(size_t start_index, size_t phrases_to_remove, const std::vector<PhraseRef<T>> &phrases_to_insert);

        /// Replaces a single Phrase in this Sequence.
        void replacePhraseAtIndex(size_t index, const PhraseRef<T> &phrase) {
            splice(index, 1, {phrase});
        }

        /// Returns a shared_ptr to the phrase at the requested index.
        /// Throws an exception if the index provided is out of bounds.
        PhraseRef<T> getPhraseAtIndex(size_t index) {
            return _phrases.at(index);
        }

        /// Returns the phrase at the requested time.
        /// If the time is past duration, returns the last phrase in the Sequence.
        /// If there are no phrases in the sequence, behavior is undefined (asserts in debug builds).
        PhraseRef<T> getPhraseAtTime(Time time);

        //
        // Phrase<T> Equivalents.
        //

        /// Returns the Sequence value at \a atTime.
        T getValue(Time atTime) const;

        /// Returns the Sequence value at \a atTime, wrapped past the end of .
        T getValueWrapped(Time time, Time inflectionPoint = 0.0f) const {
            return getValue(wrapTime(time, getDuration(), inflectionPoint));
        }

        /// Returns the value at the end of the Sequence.
        T getEndValue() const {
            return _phrases.empty() ? _initial_value : _phrases.back()->getEndValue();
        }

        /// Returns the value at the beginning of the Sequence.
        T getStartValue() const {
            return _phrases.empty() ? _initial_value : _phrases.front()->getStartValue();
        }

        /// Returns the Sequence duration.
        Time getDuration() const {
            return _duration;
        }

        //
        //
        //

        /// Returns which phrase we are in at each point in time.
        /// Note that you cannot inflect over the start or finish.
        std::pair<size_t, size_t> getInflectionPoints(Time t1, Time t2) const;

        Time getTimeAtInflection(size_t inflection) const;

        /// Returns the number of phrases in the Sequence.
        size_t getPhraseCount() const {
            return _phrases.size();
        }

        size_t size() const {
            return _phrases.size();
        }

        bool empty() const {
            return _phrases.empty();
        }

        /// Calculate and return the Sequence duration.
        Time calcDuration() const;

    private:
        // Storing shared_ptr's to Phrases requires their duration to be immutable.
        std::vector<PhraseRef<T>> _phrases;
        T _initial_value;
        Time _duration = 0;
    };

//=================================================
// Sequence Template Implementation.
//=================================================

    template<typename T>
    Sequence<T> &Sequence<T>::set(const T &value) {
        if (_phrases.empty()) {
            _initial_value = value;
        } else {
            then<Hold>(value, 0.0f);
        }
        return *this;
    }

    template<typename T>
    template<template<typename> class PhraseT, typename... Args>
    Sequence<T> &Sequence<T>::then(const T &value, Time duration, Args &&... args) {
        _phrases.emplace_back(std::make_shared<PhraseT < T>>( duration, this->getEndValue(), value, std::forward<Args>(args)... ) );
        _duration += duration;

        return *this;
    }

    template<typename T>
    Sequence<T> &Sequence<T>::then(const PhraseRef<T> &phrase) {
        _phrases.push_back(phrase);
        _duration += phrase->getDuration();

        return *this;
    }

    template<typename T>
    Sequence<T> &Sequence<T>::then(const Sequence<T> &next) {
        auto phrases = next._phrases;
        _phrases.insert(_phrases.end(), phrases.begin(), phrases.end());
        _duration = calcDuration();

        return *this;
    }

    template<typename T>
    PhraseRef<T> Sequence<T>::getPhraseAtTime(Time time) {
        assert(!_phrases.empty());
        if (time < 0) {
            return _phrases.front();
        } else if (time > this->getDuration()) {
            return _phrases.back();
        }

        for (const auto &phrase : _phrases) {
            if (phrase->getDuration() < time) {
                time -= phrase->getDuration();
            } else {
                return phrase;
            }
        }

        // Should be unreachable.
        return _phrases.back();
    }

    template<typename T>
    T Sequence<T>::getValue(Time atTime) const {
        if (atTime < 0) {
            return _initial_value;
        } else if (atTime >= this->getDuration()) {
            return getEndValue();
        }

        for (const auto &phrase : _phrases) {
            if (phrase->getDuration() < atTime) {
                atTime -= phrase->getDuration();
            } else {
                return phrase->getValue(atTime);
            }
        }
        // past the end, get the final value
        // this should be unreachable, given that we return early if time >= duration
        return getEndValue();
    }

    template<typename T>
    Time Sequence<T>::calcDuration() const {
        Time sum = 0;
        for (const auto &phrase : _phrases) {
            sum += phrase->getDuration();
        }
        return sum;
    }

    template<typename T>
    std::pair<size_t, size_t> Sequence<T>::getInflectionPoints(Time t1, Time t2) const {
        auto output = std::make_pair<size_t, size_t>(0, 0);
        auto set = std::make_pair(false, false);

        for (size_t i = 0; i < _phrases.size(); i += 1) {
            const auto duration = _phrases.at(i)->getDuration();

            if (duration < t1) {
                t1 -= duration;
            } else if (!set.first) {
                output.first = i;
                set.first = true;
            }

            if (duration < t2) {
                t2 -= duration;
            } else if (!set.second) {
                output.second = i;
                set.second = true;
            }

            if (set.first && set.second) {
                break;
            }
        }

        if (!set.second) {
            output.second = _phrases.size() - 1;
        }

        return output;
    }

    template<typename T>
    Time Sequence<T>::getTimeAtInflection(size_t inflection) const {
        Time t = 0;
        while (inflection != 0) {
            t += _phrases.at(inflection - 1)->getDuration();
            inflection -= 1;
        }
        return t;
    }

    template<typename T>
    Sequence<T> Sequence<T>::slice(Time from, Time to) const {
        if (_phrases.empty()) {
            return Sequence<T>(PhraseRef<T>(std::make_shared<Hold<T>>(to - from, _initial_value)));
        }

        // the indices of the first and last Phrases in our time range.
        auto points = getInflectionPoints(from, to);
        const auto &first = _phrases.at(points.first);
        const auto &last = _phrases.at(points.second);

        if (points.first < points.second) {
            // construct vector from range [begin, end)
            auto begin = _phrases.begin() + points.first;
            auto end = _phrases.begin() + points.second + 1;
            std::vector<PhraseRef<T>> phrases(begin, end);

            Time t1 = from - getTimeAtInflection(points.first);
            Time t2 = to - getTimeAtInflection(points.second);

            phrases[0] = std::make_shared<ClipPhrase<T>>(first, t1, first->getDuration());
            phrases[phrases.size() - 1] = std::make_shared<ClipPhrase<T>>(last, 0, t2);

            return Sequence<T>(phrases);
        } else {
            Time t = getTimeAtInflection(points.first);
            return Sequence<T>(PhraseRef<T>(std::make_shared<ClipPhrase<T>>(first, from - t, to - t)));
        }
    }

    template<typename T>
    void Sequence<T>::splice(size_t start_index, size_t phrases_to_remove, const std::vector<PhraseRef<T> > &phrases_to_insert) {
        start_index = std::min(start_index, _phrases.size());
        auto last_index = std::min(start_index + phrases_to_remove, _phrases.size());
        if (last_index > start_index) {
            auto begin = _phrases.begin() + start_index;
            auto end = _phrases.begin() + last_index;
            _phrases.erase(begin, end);
        }

        auto begin = _phrases.begin() + start_index;
        _phrases.insert(begin, phrases_to_insert.begin(), phrases_to_insert.end());
        _duration = calcDuration();
    }

//=================================================
// Sequence Decorator Phrase.
//=================================================

/// A Phrase that wraps up a Sequence.
/// Note that the Sequence becomes immutable, as it is inaccessible outside of the phrase.
    template<typename T>
    class SequencePhrase : public Phrase<T> {
    public:
        /// Construct a Phrase that wraps a Sequence, allowing it to be passed into meta-Phrases.
        SequencePhrase(const Sequence<T> &sequence) :
                Phrase<T>(sequence.getDuration()),
                _sequence(sequence) {
        }

        /// Returns the interpolated value at the given time.
        T getValue(Time atTime) const override {
            return _sequence.getValue(atTime);
        }

        T getStartValue() const override {
            return _sequence.getStartValue();
        }

        T getEndValue() const override {
            return _sequence.getEndValue();
        }

    private:
        Sequence<T> _sequence;
    };

} // namespace choreograph
