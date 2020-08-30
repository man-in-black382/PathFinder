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

#include "TimelineOptions.hpp"
#include "detail/MakeUnique.hpp"

namespace choreograph {

///
/// Timeline holds a collection of TimelineItems and updates them through time.
/// TimelineItems include Motions and Cues.
///
/// TimelineItems can be canceled using a control object retrieved from TimelineOptions.
/// Additionally, Motions can be cancelled by disconnecting their Output<T>.
/// Public methods are safe to call from cues and motion callbacks unless otherwise noted.
///
/// Timelines are move-only because they contain unique_ptrs.
///
    class Timeline : public TimelineItem {
    public:
        Timeline() = default;

        /// VS2013 requires us to define the default move constructor.
        Timeline(Timeline &&rhs);
        //=================================================
        // Creating Motions. Output<T>* Versions
        //=================================================

        /// Apply a source to output, overwriting any previous connections.
        template<typename T>
        MotionOptions<T> apply(Output<T> *output);

        /// Apply a source to output, overwriting any previous connections.
        template<typename T>
        MotionOptions<T> apply(Output<T> *output, const Sequence<T> &sequence);

        template<typename T>
        MotionOptions<T> apply(Output<T> *output, const PhraseRef<T> &phrase);

        /// Add phrases to the end of the Sequence currently connected to \a output.
        template<typename T>
        MotionOptions<T> append(Output<T> *output);

        //=================================================
        // Creating Cues.
        //=================================================

        /// Add a cue to the timeline. It will be called after \a delay time elapses on this Timeline.
        TimelineOptions cue(const std::function<void()> &fn, Time delay);

        //=================================================
        // Adding TimelineItems.
        //=================================================

        /// Add an item to the timeline. Called by append/apply/cue methods.
        /// Use to pass in MotionGroups and other types that Timeline doesn't create.
        void add(TimelineItemUniqueRef &&item);

        /// Add a shared item to the timeline.
        /// Use in advanced cases when you want to maintain the TimelineItem outside the Timeline.
        TimelineOptions addShared(const TimelineItemRef &item);

        //=================================================
        // Time manipulation.
        //=================================================

        /// Updates all timeline items to the current time.
        void update() override;

        //=================================================
        // Timeline querying methods and callbacks.
        //=================================================

        /// Returns true iff there are no items on this timeline.
        bool empty() const {
            return _items.empty();
        }

        /// Returns the number of items on this timeline.
        size_t size() const {
            return _items.size();
        }

        /// Sets a function to be called when this timeline reaches its end, but is not necessarily empty.
        void setFinishFn(const std::function<void()> &fn) {
            _finish_fn = fn;
        }

        /// Sets a function to be called when this timeline becomes empty.
        /// It is safe to destroy the timeline from this callback, unlike any Cue.
        void setClearedFn(const std::function<void()> &fn) {
            _cleared_fn = fn;
        }

        /// Returns the time (from now) at which all TimelineItems on this timeline will be finished.
        /// Cannot take into account Cues or Callbacks that may change the Timeline before finish.
        /// Useful information to cache when scrubbing Timelines with non-removed items.
        Time timeUntilFinish() const;

        Time getDuration() const override;

        //=================================================
        // Timeline element manipulation.
        //=================================================

        /// Set whether motions should be removed when finished. Default is true.
        /// This value will be passed to all future TimelineItems created by the timeline.
        /// Does not affect TimelineItems already on the Timeline.
        void setDefaultRemoveOnFinish(bool doRemove) {
            _default_remove_on_finish = doRemove;
        }

        /// Remove all items from this timeline.
        /// Do not call from a callback.
        void clear() {
            _items.clear();
        }

        //=================================================
        // Creating Motions. T* Versions.
        // Prefer the Output<T>* versions over these.
        //=================================================

        /// Apply a source to output, overwriting any previous connections. Raw pointer edition.
        /// Unless you have a strong need, prefer the use of apply( Output<T> *output ) over this version.
        template<typename T>
        MotionOptions<T> applyRaw(T *output);

        /// Apply a source to output, overwriting any previous connections.
        template<typename T>
        MotionOptions<T> applyRaw(T *output, const Sequence<T> &sequence);

        /// Add phrases to the end of the Sequence currently connected to \a output. Raw pointer edition.
        /// Unless you have a strong need, prefer the use of append( Output<T> *output ) over this version.
        template<typename T>
        MotionOptions<T> appendRaw(T *output);

        std::vector<TimelineItemUniqueRef>::iterator begin() {
            return _items.begin();
        }

        std::vector<TimelineItemUniqueRef>::iterator end() {
            return _items.end();
        }

        std::vector<TimelineItemUniqueRef>::const_iterator begin() const {
            return _items.cbegin();
        }

        std::vector<TimelineItemUniqueRef>::const_iterator end() const {
            return _items.cend();
        }

    protected:
        void customSetTime(Time time) override;

    private:
        // True if Motions should be removed from timeline when they reach their endTime.
        bool _default_remove_on_finish = true;
        std::vector<TimelineItemUniqueRef> _items;

        // queue to make adding cues from callbacks safe. Used if modifying functions are called during update loop.
        std::vector<TimelineItemUniqueRef> _queue;
        bool _updating = false;
        std::function<void()> _finish_fn = nullptr;
        std::function<void()> _cleared_fn = nullptr;

        // Clean up finished motions and add queued motions after update.
        // Calls finish function if we went from having items to no items this iteration.
        void postUpdate();

        // Remove any motions that have stale pointers or that have completed playing.
        void removeFinishedAndInvalidMotions();

        // Move any items in the queue to our active items collection.
        void processQueue();

        /// Returns a non-owning raw pointer to the Motion applied to \a output, if any.
        /// If there is no Motion applied, returns nullptr.
        /// Used internally when appending to motions.
        template<typename T>
        Motion<T> *find(T *output) const;

        /// Remove motion associated with specific output.
        /// Used internally to manage raw pointer animation.
        void cancel(void *output);
    };

//=================================================
// Timeline Template Function Implementation.
//=================================================

    template<typename T>
    MotionOptions<T> Timeline::apply(Output<T> *output) {
        auto motion = detail::make_unique<Motion<T>>(output);

        auto &motion_ref = *motion;
        add(std::move(motion));

        return MotionOptions<T>(motion_ref, motion_ref.getSequence(), *this);
    }

    template<typename T>
    MotionOptions<T> Timeline::apply(Output<T> *output, const PhraseRef<T> &phrase) {
        auto motion = detail::make_unique<Motion<T>>(output, Sequence<T>(phrase));

        auto &motion_ref = *motion;
        add(std::move(motion));

        return MotionOptions<T>(motion_ref, motion_ref.getSequence(), *this);
    }

    template<typename T>
    MotionOptions<T> Timeline::apply(Output<T> *output, const Sequence<T> &sequence) {
        auto motion = detail::make_unique<Motion<T>>(output, sequence);

        auto &motion_ref = *motion;
        add(std::move(motion));

        return MotionOptions<T>(motion_ref, motion_ref.getSequence(), *this);
    }

    template<typename T>
    MotionOptions<T> Timeline::append(Output<T> *output) {
        auto motion = output->inputPtr();
        if (motion) {
            return MotionOptions<T>(*motion, motion->getSequence(), *this);
        }
        return apply(output);
    }

    template<typename T>
    MotionOptions<T> Timeline::applyRaw(T *output) { // Remove any existing motions that affect the same variable.
        // This is a raw pointer, so we don't know about any prior relationships.
        cancel(output);

        auto motion = detail::make_unique<Motion<T>>(output);

        auto &m = *motion;
        add(std::move(motion));

        return MotionOptions<T>(m, m.getSequence(), *this);
    }

    template<typename T>
    MotionOptions<T> Timeline::applyRaw(T *output, const Sequence<T> &sequence) { // Remove any existing motions that affect the same variable.
        cancel(output);
        auto motion = detail::make_unique<Motion<T>>(output, sequence);

        auto &m = *motion;
        add(std::move(motion));

        return MotionOptions<T>(m, m.getSequence(), *this);
    }

    template<typename T>
    MotionOptions<T> Timeline::appendRaw(T *output) {
        auto motion = find(output);
        if (motion) {
            return MotionOptions<T>(*motion, motion->getSequence(), *this);
        }
        return apply(output);
    }

    template<typename T>
    Motion<T> *Timeline::find(T *output) const {
        for (auto &m : _items) {
            if (m->getTarget() == output) {
                return dynamic_cast<Motion<T> *>( m.get() );
            }
        }
        return nullptr;
    }

} // namespace choreograph
