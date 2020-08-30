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

namespace choreograph {

///
/// AnalyticChange is a phrase that calls a std::function every step.
/// You could do similar things with a Motion's updateFn, but this is composable within Sequences.
///
    template<typename T>
    class ProceduralPhrase : public Phrase<T> {
    public:
        /// Analytic Function receives start, end, and normalized time.
        /// Most basic would be mix( a, b, t ) or lerp( a, b, t ).
        /// Intended use is to apply something like cos() or random jitter.
        using Function = std::function<

        T(Time normalizedTime, Time duration)

        >;

        ProceduralPhrase(Time duration, const Function &fn) :
                Phrase<T>(duration),
                _function(fn) {
        }

        T getValue(Time atTime) const override {
            return _function(this->normalizeTime(atTime), this->getDuration());
        }

    private:
        Function _function;
    };

} // namespace choreograph
