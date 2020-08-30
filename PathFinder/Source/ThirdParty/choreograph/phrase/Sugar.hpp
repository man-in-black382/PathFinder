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

#include "Retime.hpp"
#include "Combine.hpp"

///
/// \file
/// Free functions to make creating certain meta-phrases easier.
///
/// Note that the compiler can't deduce the type of T for PhraseRef<T> from DerivedRef<T>
/// so you may need to help it out. e.g. `loopPhrase<vec3>( DerivedRef<vec3>, 2 );`
/// If you have a PhraseRef<T>, you won't need to specify the type at the call-site.
///

namespace choreograph {

/// Create a Phrase that repeats \a source Phrase \a numLoops times.
    template<typename T>
    inline PhraseRef <T> makeRepeat(const PhraseRef <T> &source, float numLoops, Time inflectionPoint = 0.0f) {
        return std::make_shared<LoopPhrase<T>>(source, numLoops, inflectionPoint);
    }

/// Create a Phrase that loops \a source Phrase \a numLoops times.
    template<typename T>
    inline PhraseRef <T> makePingPong(const PhraseRef <T> &source, float numLoops, Time inflectionPoint = 0.0f) {
        return std::make_shared<PingPongPhrase<T>>(source, numLoops, inflectionPoint);
    }

/// Create a Phrase that plays \a source Phrase in reverse.
    template<typename T>
    inline PhraseRef <T> makeReverse(const PhraseRef <T> &source) {
        return std::make_shared<ReversePhrase<T>>(source);
    }

/// Create a MixPhrase that blends the value of Phrases \a a and \a b.
    template<typename T>
    inline std::shared_ptr<MixPhrase < T>>

    makeBlend(const PhraseRef <T> &a, const PhraseRef <T> &b, float mix = 0.5f, const typename MixPhrase<T>::LerpFn &lerp_fn = &lerpT < T >) {
        return std::make_shared<MixPhrase<T>>(a, b, mix, lerp_fn);
    }

/// Create a RampTo that animates from \a a to \a b.
    template<typename T>
    inline std::shared_ptr<RampTo < T>>

    makeRamp(const T &a, const T &b, Time duration, const EaseFn &ease_fn = &easeNone, const typename RampTo<T>::LerpFn &lerp_fn = &lerpT < T >) {
        return std::make_shared<RampTo<T>>(duration, a, b, ease_fn, lerp_fn);
    }

/// Create an AccumulatePhrase that combines the values of input Phrases via a left fold.
/// The default operator is sum.
    template<typename T>
    inline std::shared_ptr<AccumulatePhrase < T>>

    makeAccumulator(const T &initial_value, const PhraseRef <T> &a, const PhraseRef <T> &b, const typename AccumulatePhrase<T>::CombineFunction &fn = &AccumulatePhrase<T>::sum, Time duration = 0) {
        if (duration > 0)
            return std::make_shared<AccumulatePhrase<T>>(duration, initial_value, a, b, fn);
        else
            return std::make_shared<AccumulatePhrase<T>>(initial_value, a, b, fn);
    }

/// Create an AccumulatedPhrase that sums a phrase with an initial value.
/// You can change the reduce function after creation; providing it as a function parameter caused ambiguity for VS2013.
    template<typename T>
    inline std::shared_ptr<AccumulatePhrase < T>>

    makeAccumulator(const T &initial_value, const PhraseRef <T> &a, Time duration = 0) {
        if (duration > 0)
            return std::make_shared<AccumulatePhrase<T>>(duration, initial_value, a);
        else
            return std::make_shared<AccumulatePhrase<T>>(initial_value, a);
    }

///
/// Create a ProceduralPhrase that evaluates \a fn over \a duration.
///
    template<typename T>
    inline PhraseRef <T> makeProcedure(Time duration, const typename ProceduralPhrase<T>::Function &fn) {
        return std::make_shared<ProceduralPhrase<T>>(duration, fn);
    }

} // namespace choreograph
