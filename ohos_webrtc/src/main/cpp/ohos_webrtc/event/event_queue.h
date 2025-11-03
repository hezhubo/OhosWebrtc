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

#ifndef WEBRTC_EVENT_EVENT_QUEUE_H
#define WEBRTC_EVENT_EVENT_QUEUE_H

#include <mutex>
#include <queue>
#include <memory>

#include "event.h"
#include "utils/marcos.h"

namespace webrtc {

template <typename T>
class EventQueue {
public:
    void Enqueue(std::unique_ptr<Event<T>> event)
    {
        UNUSED std::lock_guard<std::mutex> lock(mutex_);
        events_.push(std::move(event));
    }

    std::unique_ptr<Event<T>> Dequeue()
    {
        UNUSED std::lock_guard<std::mutex> lock(mutex_);
        if (events_.empty()) {
            return nullptr;
        }

        auto event = std::move(events_.front());
        events_.pop();

        return event;
    }

private:
    std::queue<std::unique_ptr<Event<T>>> events_;
    std::mutex mutex_;
};

} // namespace webrtc

#endif // WEBRTC_EVENT_EVENT_QUEUE_H
