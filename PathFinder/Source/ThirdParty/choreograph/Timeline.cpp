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

#include "Timeline.h"
#include "detail/VectorManipulation.hpp"

using namespace choreograph;

namespace {

// A single-item timeline for wrapping shared TimelineItems.
    class PassthroughTimelineItem : public TimelineItem {
    public:
        explicit PassthroughTimelineItem(const TimelineItemRef &item)
                : _item(item) {
        }

        void update() override {
            _item->step(deltaTime());
        }

        Time getDuration() const override {
            return _item->getDuration();
        }

        const void *getTarget() const override {
            return _item->getTarget();
        }

    private:
        TimelineItemRef _item;
    };

} // namespace

Timeline::Timeline(Timeline &&rhs)
        : _default_remove_on_finish(std::move(rhs._default_remove_on_finish)),
          _items(std::move(rhs._items)),
          _queue(std::move(rhs._queue)),
          _updating(std::move(rhs._updating)),
          _finish_fn(std::move(rhs._finish_fn)) {
}

void Timeline::removeFinishedAndInvalidMotions() {
    detail::erase_if(&_items, [](const TimelineItemUniqueRef &motion) {
        return (motion->getRemoveOnFinish() && motion->isFinished()) || motion->cancelled();
    });
}

void Timeline::customSetTime(Time time) {
    for (auto &item : _items) {
        item->setTime(time);
    }
}

void Timeline::update() {
    _updating = true;
    for (auto &item : _items) {
        item->step(deltaTime());
    }
    _updating = false;

    postUpdate();
}

void Timeline::postUpdate() {
    bool was_empty = empty();

    removeFinishedAndInvalidMotions();

    processQueue();

    if (_finish_fn) {
        auto d = getDuration();
        if (forward() && time() >= d && previousTime() < d) {
            _finish_fn();
        } else if (backward() && time() <= 0.0f && previousTime() > 0.0f) {
            _finish_fn();
        }
    }

    // Call cleared function last if provided.
    // We do this here so it's safe to destroy the timeline from the callback.
    bool is_empty = empty();
    if (_cleared_fn) {
        if (is_empty && !was_empty) {
            _cleared_fn();
        }
    }
}

Time Timeline::timeUntilFinish() const {
    Time end = 0;
    for (auto &item : _items) {
        end = std::max(end, item->getTimeUntilFinish());
    }
    return end;
}

Time Timeline::getDuration() const {
    Time duration = 0;
    for (auto &item : _items) {
        duration = std::max(duration, item->getEndTime());
    }
    return duration;
}

void Timeline::processQueue() {
    using namespace std;
    std::copy(make_move_iterator(_queue.begin()), make_move_iterator(_queue.end()), back_inserter(_items));
    _queue.clear();
}

void Timeline::cancel(void *output) {
    for (auto &item : _items) {
        if (item->getTarget() == output) {
            item->cancel();
        }
    }

    for (auto &item : _queue) {
        if (item->getTarget() == output) {
            item->cancel();
        }
    }
}

void Timeline::add(TimelineItemUniqueRef &&item) {
    item->setRemoveOnFinish(_default_remove_on_finish);

    if (_updating) {
        _queue.emplace_back(std::move(item));
    } else {
        _items.emplace_back(std::move(item));
    }
}

TimelineOptions Timeline::addShared(const TimelineItemRef &shared) {
    auto item = detail::make_unique<PassthroughTimelineItem>(shared);
    item->setRemoveOnFinish(_default_remove_on_finish);
    auto &ref = *item;

    if (_updating) {
        _queue.emplace_back(std::move(item));
    } else {
        _items.emplace_back(std::move(item));
    }

    return TimelineOptions(ref);
}

TimelineOptions Timeline::cue(const std::function<void()> &fn, Time delay) {
    auto cue = detail::make_unique<Cue>(fn, delay);
    TimelineOptions options(*cue);

    add(std::move(cue));

    return options;
}
