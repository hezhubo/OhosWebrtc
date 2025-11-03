/**
 * Copyright (c) 2024 Archermind Technology (Nanjing) Co. Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WEBRTC_EVENT_EVENT_H
#define WEBRTC_EVENT_EVENT_H

#include <memory>
#include <functional>

namespace webrtc {

template <typename T>
class Event {
public:
    virtual ~Event() = default;

    virtual void Process(T&) = 0;
};

template <typename T>
class CallbackEvent : public Event<T> {
    struct PrivateTag {};

public:
    static std::unique_ptr<CallbackEvent<T>> Create(std::function<void(T&)> callback)
    {
        return std::make_unique<CallbackEvent<T>>(PrivateTag(), std::move(callback));
    }

    CallbackEvent(PrivateTag, const std::function<void(T&)>& callback) : callback_(callback) {}
    ~CallbackEvent() override = default;

    void Process(T& target) override
    {
        callback_(target);
    }

private:
    const std::function<void(T&)> callback_;
};

template <typename T>
class EmptyEvent : public Event<T> {
    struct PrivateTag {};

public:
    static std::unique_ptr<EmptyEvent> Create()
    {
        return std::make_unique<EmptyEvent>(PrivateTag());
    }

    explicit EmptyEvent(PrivateTag) {}
    ~EmptyEvent() override = default;

    void Process(T&) override
    {
        // Do nothing.
    }
};

} // namespace webrtc

#endif // WEBRTC_EVENT_EVENT_H
