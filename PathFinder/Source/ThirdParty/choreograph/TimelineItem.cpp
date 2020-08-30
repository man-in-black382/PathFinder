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

#include "TimelineItem.h"

using namespace choreograph;

Control::Control(TimelineItem *item) :
        _item(item) {
}

void Control::cancel() {
    if (_item) {
        _item->cancel();
        _item = nullptr;
    }
}

bool Control::isValid() const {
    return _item && (!_item->cancelled());
}

bool Control::isInvalid() const {
    return (_item == nullptr) || _item->cancelled();
}

///
///
///

TimelineItem::~TimelineItem() {
    if (_control) {
        _control->cancel();
    }
}

void TimelineItem::step(Time dt) {
    _time += dt * _speed;
    if (!cancelled()) {
        // update properties
        update();
    }
    _previous_time = _time;
}

void TimelineItem::jumpTo(Time time) {
    _time = time;
    if (!cancelled()) {
        // update properties
        update();
    }
    _previous_time = _time;
}

bool TimelineItem::isFinished() const {
    if (backward()) {
        return time() <= 0.0f;
    } else {
        return time() >= getDuration();
    }
}

void TimelineItem::resetTime() {
    if (forward()) {
        setTime(0.0f);
    } else {
        setTime(getEndTime());
    }
}

Time TimelineItem::getTimeUntilFinish() const {
    if (forward()) {
        return (getDuration() / getPlaybackSpeed()) - time();
    } else {
        return time() / getPlaybackSpeed();
    }
}

const std::shared_ptr<Control> &TimelineItem::getControl() {
    if (!_control) {
        _control = std::make_shared<Control>(this);
    }
    return _control;
}