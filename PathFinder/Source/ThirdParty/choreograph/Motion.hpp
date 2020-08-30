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

#include "TimelineItem.h"
#include "Sequence.hpp"
#include "Output.hpp"
#include "detail/VectorManipulation.hpp"

namespace choreograph {

//=================================================
// Aliases.
//=================================================

    template<typename T>
    class Motion;

    template<typename T> using MotionRef = std::shared_ptr<Motion<T>>;

///
/// Motion: Moves a playhead along a Sequence and sends its value to a user-defined output.
/// Connects a Sequence and an Output.
///
    template<typename T>
    class Motion : public TimelineItem {
    public:
        using MotionT       = Motion<T>;
        using SequenceT     = Sequence<T>;
        using Callback      = std::function<void()>;

        Motion() = delete;

        Motion(T *target, const SequenceT &sequence) :
                _target(target),
                _source(sequence) {
        }

        Motion(Output<T> *target, const SequenceT &sequence) :
                _target(target->valuePtr()),
                _output(target),
                _source(sequence) {
            _output->disconnect();
            _output->_input = this;
        }

        Motion(Output<T> *target) :
                _target(target->valuePtr()),
                _output(target),
                _source(target->value()) {
            _output->disconnect();
            _output->_input = this;
        }

        ~Motion() {
            disconnect();
        }

        /// Returns duration of the underlying sequence.
        Time getDuration() const final override {
            return _source.getDuration();
        }

        /// Returns ratio of time elapsed, from [0,1] over duration.
        Time getProgress() const {
            return time() / _source.getDuration();
        }

        /// Returns the underlying Sequence sampled for this motion.
        SequenceT &getSequence() {
            return _source;
        }

        const void *getTarget() const final override {
            return _target;
        }

        /// Returns the current value of the target.
        T getCurrentValue() const {
            return *_target;
        }

        /// Set a function to be called when we reach the end of the sequence. Receives *this as an argument.
        void setFinishFn(const Callback &c) {
            _finish_fn = c;
        }

        /// Set a function to be called when we start the sequence. Receives *this as an argument.
        void setStartFn(const Callback &c) {
            _start_fn = c;
        }

        /// Set a function to be called when we cross the given inflection point. Receives *this as an argument.
        void addInflectionCallback(size_t inflection_point, const Callback &callback);

        /// Set a function to be called at each update step of the sequence.
        /// Function will be called immediately after setting the target value.
        void setUpdateFn(const Callback &c) {
            _update_fn = c;
        }

        /// Update the connected target with the current sequence value.
        /// Calls start/update/finish functions as appropriate if assigned.
        void update() final override;

        /// Removes phrases from sequence before specified time.
        /// Note that you can safely share sequences if you add them to each motion as phrases.
        void cutPhrasesBefore(Time time) {
            sliceSequence(time, _source.getDuration());
        }

        /// Cut animation in \a time from the Motion's current time().
        void cutIn(Time time) {
            sliceSequence(this->time(), this->time() + time);
        }

        /// Slices up our underlying Sequence.
        void sliceSequence(Time from, Time to);

    private:
        SequenceT _source;
        Output<T> *_output = nullptr;
        T *_target = nullptr;

        Callback _finish_fn;
        Callback _start_fn;
        Callback _update_fn;
        std::vector<std::pair<int, Callback>> _inflection_callbacks;

        /// Sets the output to a different output.
        /// Used by Output<T>'s move assignment and move constructor.
        void setOutput(Output<T> *output);

        /// Disconnects Motion from Output.
        /// Used on destruction of either Motion or Output.
        void disconnect();

        /// Allow Outputs to call private methods.
        /// Could probably do a song and dance with lambdas to avoid friendship, but this is fine.
        friend class Output<T>;
    };

//=================================================
// Motion Template Implementation.
//=================================================

    template<typename T>
    void Motion<T>::update() {
        if (_start_fn) {
            if (forward() && time() > 0.0f && previousTime() <= 0.0f) {
                _start_fn();
            } else if (backward() && time() < getDuration() && previousTime() >= getDuration()) {
                _start_fn();
            }
        }

        *_target = _source.getValue(time());

        if (!_inflection_callbacks.empty()) {
            auto points = _source.getInflectionPoints(previousTime(), time());
            if (points.first != points.second) {
                // We just crossed into the second inflection point
                auto top = std::max(points.first, points.second);
                auto bottom = std::min(points.first, points.second);
                for (const auto &fn : _inflection_callbacks) {
                    auto inflection = fn.first;
                    if (inflection > bottom && inflection <= top) {
                        fn.second();
                    }
                }
            }
        }

        if (_update_fn) {
            _update_fn();
        }

        if (_finish_fn) {
            if (forward() && time() >= getDuration() && previousTime() < getDuration()) {
                _finish_fn();
            } else if (backward() && time() <= 0.0f && previousTime() > 0.0f) {
                _finish_fn();
            }
        }
    }

    template<typename T>
    void Motion<T>::addInflectionCallback(size_t inflection_point, const Callback &callback) {
        _inflection_callbacks.emplace_back(std::make_pair((int) inflection_point, callback));
    }

    template<typename T>
    void Motion<T>::sliceSequence(Time from, Time to) {
        // Shift inflection point references
        const auto inflection = _source.getInflectionPoints(from, to).first;
        for (auto &fn : _inflection_callbacks) {
            fn.first -= inflection;
        }

        detail::erase_if(&_inflection_callbacks, [](const std::pair<int, Callback> &p) {
            return p.first < 0;
        });

        _source = _source.slice(from, to);

        setTime(this->time() - from);
    }

    template<typename T>
    void Motion<T>::setOutput(Output<T> *output) {
        if (_output) {
            _output->_input = nullptr;
        }

        _output = output;
        _target = _output->valuePtr();
    }

    template<typename T>
    void Motion<T>::disconnect() {
        // Disconnect pointer.
        if (_output) {
            _output->_input = nullptr;
            _output = nullptr;
        }
        // Stop evaluation of TimelineItem.
        cancel();
    }

} // namespace choreograph
