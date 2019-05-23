#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

#include "BitwiseEnum.hpp"

namespace Foundation 
{

    template<class PublisherT, class KeyT, class DelegateT>
    class Event;

    template<class PublisherT, class KeyT, class RetT, class... ArgTs>
    class Event<PublisherT, KeyT, RetT(ArgTs...)> {

        friend PublisherT;

    public:

        using Delegate = std::function<RetT(ArgTs...)>;

        struct Binding {
            KeyT key;
            Delegate delegate;

            Binding(KeyT key, Delegate delegate) : key(key), delegate(delegate) {}

            template<class T>
            Binding(KeyT key, T *target, RetT(T::*funcPtr)(ArgTs...))
                :
                key(key),
                delegate([target, funcPtr](ArgTs... args) {
                return (target->*funcPtr)(args...);
            }) {}
        };

        void Subscribe(const Binding &binding) {
            mContainer.bindings[binding.key] = binding.delegate;
        }

        void Unsubscribe(KeyT key) {
            mContainer.bindings.erase(key);
        }

        void Clear() {
            mContainer.bindings.clear();
        }

        size_t Size() const {
            return mContainer.bindings.size();
        }

        template<typename T>
        Event &operator+=(T &&binding) {
            Subscribe(std::forward<T>(binding));
            return *this;
        }

        Event &operator+=(const Binding &binding) {
            Subscribe(binding);
            return *this;
        }

        template<typename T>
        Event &operator-=(T &&key) {
            Unsubscribe(std::forward<T>(key));
            return *this;
        }

    private:
        // Hide bindings map from publisher (PublisherT)
        struct BindingContainer {
        private:
            friend Event;
            std::unordered_map<KeyT, Delegate> bindings;
        };

        BindingContainer mContainer;

        void Raise(ArgTs... args) {
            for (auto &binding : mContainer.bindings) {
                binding.second(args...);
            }
        }

        template<typename... T>
        void operator()(T &&... args) {
            Raise(std::forward<T>(args)...);
        }

    };



    template<class PublisherT, class IdxT, class KeyT, class DelegateT>
    class MultiEvent;

    template<class PublisherT, class IdxT, class KeyT, class RetT, class... ArgTs>
    class MultiEvent<PublisherT, IdxT, KeyT, RetT(ArgTs...)> {

    public:
        friend PublisherT;
        using EventType = Event<PublisherT, KeyT, RetT(ArgTs...)>;

    private:
        // Make events map inaccessible to publisher (PublisherT)
        struct EventsContainer {
        private:
            friend MultiEvent;
            std::unordered_map<IdxT, EventType> events;
        };

        EventsContainer mEventsContainer;

    public:
        EventType &At(IdxT index) {
            return mEventsContainer.events[index];
        }

        EventType &operator[](IdxT index) {
            return mEventsContainer.events[index];
        }
    };

}
