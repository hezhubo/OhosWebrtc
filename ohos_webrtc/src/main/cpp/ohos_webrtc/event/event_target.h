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

#ifndef WEBRTC_EVENT_EVENT_TARGET_H
#define WEBRTC_EVENT_EVENT_TARGET_H

#include <map>
#include <queue>
#include <mutex>
#include <thread>

#include "napi.h"
#include "napi/native_api.h"

#include "rtc_base/logging.h"

#include "event.h"
#include "event_queue.h"
#include "../utils/marcos.h"

namespace webrtc {

template <typename T>
class NapiEventTarget : public Napi::ObjectWrap<T>, public EventQueue<T> {
public:
    explicit NapiEventTarget(const Napi::CallbackInfo& info) : Napi::ObjectWrap<T>(info)
    {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

        tsfn_ = Napi::ThreadSafeFunction::New(
            info.Env(), Napi::Function::New(info.Env(), [this](const Napi::CallbackInfo& info) { this->Run(info); }),
            "NapiEventTarget", 0, 1);

        // Hold the reference until stopped
        this->Ref();
    }

    ~NapiEventTarget() override
    {
        tsfn_.Release();
    }

    void Dispatch(std::unique_ptr<Event<T>> event)
    {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

        this->Enqueue(std::move(event));

        napi_status status = tsfn_.BlockingCall([](Napi::Env env, Napi::Function func) { func.Call({}); });
        if (status != napi_ok) {
            RTC_LOG(LS_ERROR) << " tsfn call error: " << status;
        }
    }

    virtual void Stop()
    {
        shouldStop_ = true;
        Dispatch(EmptyEvent<T>::Create());
    }

    bool ShouldStop() const
    {
        return shouldStop_;
    }

    bool GetEventHandler(const std::string& type, Napi::Function& fn) const
    {
        UNUSED std::lock_guard<std::mutex> lock(mutex_);

        auto it = eventHandlers_.find(type);
        if (it == eventHandlers_.end()) {
            return false;
        }

        fn = it->second.Value();
        return true;
    }

    void SetEventHandler(const std::string& type, const Napi::Function& fn)
    {
        UNUSED std::lock_guard<std::mutex> lock(mutex_);

        auto it = eventHandlers_.find(type);
        if (it != eventHandlers_.end()) {
            it->second.Unref();
            eventHandlers_.erase(it);
        }

        eventHandlers_[type] = Persistent(fn);
    }

    void RemoveEventHandler(const std::string& type)
    {
        UNUSED std::lock_guard<std::mutex> lock(mutex_);

        auto it = eventHandlers_.find(type);
        if (it != eventHandlers_.end()) {
            it->second.Unref();
            eventHandlers_.erase(it);
        }
    }

protected:
    virtual void Run(const Napi::CallbackInfo& info)
    {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

        Napi::HandleScope scope(info.Env());
        while (!shouldStop_) {
            auto event = this->Dequeue();
            if (!event) {
                break;
            }

            T* target = static_cast<T*>(this);
            event->Process(*target);
        }

        if (shouldStop_) {
            this->DidStop();
            this->Unref();
        }
    }

    virtual void DidStop()
    {
        // Do nothing.
    }

protected:
    void MakeCallback(const char* name, const std::initializer_list<napi_value>& args)
    {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << ": " << name;

        Napi::Function func;
        GetEventHandler(name, func);
        if (func.IsEmpty()) {
            RTC_DLOG(LS_WARNING) << "No event handler: " << name;
            return;
        }

        auto self = this->Value();
        func.MakeCallback(self, args);
    }

private:
    std::atomic<bool> shouldStop_{false};
    mutable std::mutex mutex_;
    std::map<std::string, Napi::FunctionReference> eventHandlers_;
    Napi::ThreadSafeFunction tsfn_;
};

} // namespace webrtc

#endif // WEBRTC_EVENT_EVENT_TARGET_H
