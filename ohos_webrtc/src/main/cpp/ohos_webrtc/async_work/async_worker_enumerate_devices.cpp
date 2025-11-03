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

#include "async_worker_enumerate_devices.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

AsyncWorkerEnumerateDevices* AsyncWorkerEnumerateDevices::Create(Napi::Env env, const char* resourceName)
{
    auto asyncWorker = new AsyncWorkerEnumerateDevices(env, resourceName);
    return asyncWorker;
}

AsyncWorkerEnumerateDevices::AsyncWorkerEnumerateDevices(Napi::Env env, const char* resourceName)
    : AsyncWorker(env, resourceName), deferred_(Napi::Promise::Deferred::New(env))
{
}

void AsyncWorkerEnumerateDevices::Execute()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    cameraDevices_ = CameraEnumerator::GetDevices();
    audioDevices_ = AudioDeviceEnumerator::GetDevices();
}

void AsyncWorkerEnumerateDevices::OnOK()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto result = Array::New(Env(), cameraDevices_.size() + audioDevices_.size());

    for (uint32_t index = 0; index < cameraDevices_.size(); index++) {
        auto obj = Object::New(Env());
        obj.Set("deviceId", String::New(Env(), cameraDevices_[index].deviceId));
        obj.Set("groupId", String::New(Env(), cameraDevices_[index].groupId));
        obj.Set("label", String::New(Env(), cameraDevices_[index].label));
        obj.Set("kind", String::New(Env(), "videoinput"));
        obj.Set("getCapabilities", Function::New(Env(), [](const CallbackInfo& info) {
                    RTC_DLOG(LS_VERBOSE) << "getCapabilities";
                    NAPI_THROW(Error::New(info.Env(), "Not implemented"), info.Env().Undefined());
                }));
        result.Set(index, obj);
    }

    for (uint32_t index = 0; index < audioDevices_.size(); index++) {
        auto obj = Object::New(Env());
        obj.Set("deviceId", String::New(Env(), audioDevices_[index].deviceId));
        obj.Set("groupId", String::New(Env(), audioDevices_[index].groupId));
        obj.Set("label", String::New(Env(), audioDevices_[index].label));
        if (audioDevices_[index].role == AudioDeviceRole::Input) {
            obj.Set("kind", String::New(Env(), "audioinput"));
            obj.Set("getCapabilities", Function::New(Env(), [](const CallbackInfo& info) {
                        RTC_DLOG(LS_VERBOSE) << "getCapabilities";
                        NAPI_THROW(Error::New(info.Env(), "Not implemented"), info.Env().Undefined());
                    }));
        } else if (audioDevices_[index].role == AudioDeviceRole::Output) {
            obj.Set("kind", String::New(Env(), "audiooutput"));
        } else {
            RTC_LOG(LS_WARNING) << "Invalid audio device role";
        }
        result.Set(cameraDevices_.size() + index, obj);
    }

    deferred_.Resolve(result);
}

void AsyncWorkerEnumerateDevices::OnError(const Napi::Error& e)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    deferred_.Reject(e.Value());
}

} // namespace webrtc
