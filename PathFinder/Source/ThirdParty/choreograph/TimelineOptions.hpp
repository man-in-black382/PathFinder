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

#include "Motion.hpp"
#include "phrase/Ramp.hpp"
#include "Cue.h"

namespace choreograph {

    class Timeline;

///
/// Options for manipulating newly created TimelineItems.
/// Uses CRTP so we don't lose the actual type when chaining methods.
/// Do not store the TimelineOptions object, as it contains a non-owning reference.
///
    template<typename Derived>
    class TimelineOptionsBase {
    public:
        TimelineOptionsBase(TimelineItem &item) :
                _item(item) {
        }

        //=================================================
        // TimelineItem Interface Mirroring.
        //=================================================

        /// Set whether the item should be removed from the timeline on finish.
        Derived &removeOnFinish(bool doRemove) {
            _item.setRemoveOnFinish(doRemove);
            return self();
        }

        /// Set the rate at which time advances for Motion.
        Derived &playbackSpeed(Time speed) {
            _item.setPlaybackSpeed(speed);
            return self();
        }

        /// Set the initial time offset of the TimelineItem.
        /// For Cues, this sets the time in the future.
        /// For Motions, this is akin to adding a hold at the beginning of the Sequence.
        Derived &setStartTime(Time t) {
            _item.setStartTime(t);
            return self();
        }

        TimelineItem &getItem() {
            return _item;
        }

        /// Returns a shared_ptr to the control object for the Item. Allows you to cancel the Item later.
        TimelineItemControlRef getControl() {
            return _item.getControl();
        }

        /// Returns an object that cancels the Cue when it falls out of scope.
        /// You should store a ScopedCueRef in any class that captures [this] in a cued lambda.
        ScopedCancelRef getScopedControl() {
            return std::make_shared<ScopedCancel>(_item.getControl());
        }

    private:
        TimelineItem &_item;

        Derived &self() {
            return static_cast<Derived &>( *this );
        }
    };

///
/// TimelineOptions with no additional behaviors beyond base.
/// Returned when creating cues.
/// Do not store the TimelineOptions object, as it contains a non-owning reference.
///
    class TimelineOptions : public TimelineOptionsBase<TimelineOptions> {
    public:
        TimelineOptions(TimelineItem &item)
                : TimelineOptionsBase<TimelineOptions>(item) {
        }
    };

///
/// MotionOptions provide a temporary facade for manipulating a timeline Motion and its underlying Sequence.
/// All methods return a reference back to the MotionOptions object for chaining.
/// Do not store the MotionOptions object, as it contains non-owning references.
///
    template<typename T>
    class MotionOptions : public TimelineOptionsBase<MotionOptions<T>> {
    public:
        using SelfT = MotionOptions<T>;
        using MotionCallback = typename Motion<T>::Callback;

        MotionOptions(Motion<T> &motion, Sequence<T> &sequence, const Timeline &timeline) :
                TimelineOptionsBase<MotionOptions<T>>(motion),
                _motion(motion),
                _sequence(sequence),
                _timeline(timeline) {
        }

        //=================================================
        // Motion Interface Mirroring.
        //=================================================

        /// Set function to be called when Motion starts. Receives reference to motion.
        SelfT &startFn(const MotionCallback &fn) {
            _motion.setStartFn(fn);
            return *this;
        }

        /// Set function to be called when Motion updates. Receives current target value.
        SelfT &updateFn(const typename Motion<T>::Callback &fn) {
            _motion.setUpdateFn(fn);
            return *this;
        }

        /// Set function to be called when Motion finishes. Receives reference to motion.
        SelfT &finishFn(const MotionCallback &fn) {
            _motion.setFinishFn(fn);
            return *this;
        }

        /// Set a function to be called when the current inflection point is crossed.
        /// An inflection occcurs when the Sequence moves from one Phrase to the next.
        /// You must add a phrase after this for the inflection to occur.
        SelfT &onInflection(const MotionCallback &fn) {
            return onInflection(_sequence.getPhraseCount(), fn);
        }

        /// Adds an inflection callback when the specified phrase index is crossed.
        SelfT &onInflection(size_t point, const MotionCallback &fn) {
            _motion.addInflectionCallback(point, fn);
            return *this;
        }

        /// Clip the motion in \t time from the current Motion playhead.
        /// Also discards any phrases we have already played up to this point.
        SelfT &cutIn(Time t) {
            _motion.cutIn(t);
            return *this;
        }

        /// Clip the motion at time \t from the beginning of the Motion's Sequence.
        /// When used after Timeline::apply, will have the same effect as cutIn().
        SelfT &cutAt(Time t) {
            _motion.sliceSequence(0, t);
            return *this;
        }

        //=================================================
        // Sequence Interface Mirroring.
        //=================================================

        /// Set the current value of the Sequence. Acts as an instantaneous hold.
        SelfT &set(const T &value) {
            _sequence.set(value);
            return *this;
        }

        /// Construct and append a Phrase to the Sequence.
        template<template<typename> class PhraseT, typename... Args>
        SelfT &then(const T &value, Time duration, Args &&... args) {
            _sequence.template then<PhraseT>(value, duration, std::forward<Args>(args)...);
            return *this;
        }

        /// Append a phrase to the Sequence.
        SelfT &then(const PhraseRef<T> &phrase) {
            _sequence.then(phrase);
            return *this;
        }

        /// Append a sequence to the Sequence.
        SelfT &then(const Sequence<T> &sequence) {
            _sequence.then(sequence);
            return *this;
        }

        //=================================================
        // Extra Sugar.
        //=================================================

        /// Append a Hold to the end of the Sequence. Assumes you want to hold using the Sequence's current end value.
        SelfT &hold(Time duration) {
            _sequence.template then<Hold>(_sequence.getEndValue(), duration);
            return *this;
        }

        SelfT &holdUntil(Time time) {
            _sequence.template then<Hold>(_sequence.getEndValue(), std::max<Time>(time - _sequence.getDuration(), 0));
            return *this;
        }

        template<typename... Args>
        SelfT &rampTo(const T &value, Time duration, Args &&... args) {
            _sequence.template then<RampTo>(value, duration, std::forward<Args>(args)...);
            return *this;
        }

        //=================================================
        // Accessors to Motion and Sequence.
        //=================================================

        Sequence<T> &getSequence() {
            return _sequence;
        }

        Motion<T> &getMotion() {
            return _motion;
        }

    private:
        Motion<T> &_motion;
        Sequence<T> &_sequence;
        const Timeline &_timeline;
    };

} // namespace choreograph
