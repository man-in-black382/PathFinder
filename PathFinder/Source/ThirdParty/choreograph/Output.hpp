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

namespace choreograph {

    template<typename T>
    class Motion;

///
/// Safe type for Choreograph outputs.
/// Disconnects applied Motion on destruction so you don't accidentally modify stale pointers.
///
/// Output is a move-only type. To pass it around, do the following:
/// Output<T> new_output = std::move( previous_output );
/// This avoids ambiguity around what copying would mean, and since the STL is move-aware,
/// std::vectors of Outputs (or types containing them) just work.
///
    template<typename T>
    class Output {
    public:
        /// Disconnect on destruction.
        ~Output() {
            disconnect();
        }

        /// Default constructor.
        Output() = default;

        /// Construct with a value.
        Output(const T &value) :
                _value(value) {
        }

        /// Move assignment takes value and any input Motion.
        Output<T> &operator=(Output<T> &&rhs);

        /// Move constructor takes value and any input Motion.
        Output(Output<T> &&rhs);

        /// Delete copy assignment. Confusing if we only copy value, lying if we steal input Motions.
        Output<T> &operator=(const Output<T> &rhs) = delete;

        /// Delete copy constructor. Confusing if we only copy value, lying if we steal input Motions.
        Output(const Output<T> &rhs) = delete;

        /// Disconnect from Motion input.
        void disconnect();

        /// Returns true iff this Output has a Motion input.
        bool isConnected() const {
            return _input != nullptr;
        }

        /// Value assignment operator.
        Output<T> &operator=(T value) {
            _value = value;
            return *this;
        }

        /// Value add-assign.
        Output<T> &operator+=(T value) {
            _value += value;
            return *this;
        }

        /// Returns value of output.
        const T &value() const {
            return _value;
        }

        /// Returns value of output.
        const T &operator()() const {
            return _value;
        }

        /// Returns value of output for manipulating.
        T &operator()() {
            return _value;
        }

        /// Returns the value this output will have at the end of its connected motion.
        /// Returns current value if no motion attached.
        T endValue() const;

        /// Enable cast to value type.
        operator const T &() {
            return _value;
        }

        /// Returns pointer to value.
        const T *valuePtr() const {
            return &_value;
        }

        /// Returns pointer to value.
        T *valuePtr() {
            return &_value;
        }

        Motion<T> *inputPtr() {
            return _input;
        }

    private:
        T _value;
        Motion<T> *_input = nullptr;

        friend class Motion<T>;
    };

//=================================================
// Output Implementation
//=================================================

// Move constructor takes value and any input Motion.
    template<typename T>
    Output<T>::Output(Output<T> &&rhs):
            _value(std::move(rhs._value)),
            _input(std::move(rhs._input)) {
        if (_input) {
            _input->setOutput(this);
        }
    }

// Move assignment takes value and any input Motion.
    template<typename T>
    Output<T> &Output<T>::operator=(Output<T> &&rhs) {
        if (this != &rhs) {
            _value = std::move(rhs._value);
            _input = std::move(rhs._input);
            if (_input) {
                _input->setOutput(this);
            }
        }
        return *this;
    }

    template<typename T>
    void Output<T>::disconnect() {
        if (_input) {
            // Use Motion::disconnect, since TimelineItem::cancel only stops evaluation.
            // Motion::disconnect also nullifies our pointer to the input.
            _input->disconnect();
        }
    }

    template<typename T>
    T Output<T>::endValue() const {
        if (_input) {
            return _input->getSequence().getEndValue();
        }
        return _value;
    }

} // namespace choreograph
