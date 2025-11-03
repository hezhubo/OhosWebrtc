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

#ifndef WEBRTC_ASYNC_WORKER_ENUMERATE_DEVICES_H
#define WEBRTC_ASYNC_WORKER_ENUMERATE_DEVICES_H

#include "../camera/camera_enumerator.h"
#include "../audio_device/audio_device_enumerator.h"

#include <vector>

#include "napi.h"

namespace webrtc {

class AsyncWorkerEnumerateDevices : public Napi::AsyncWorker {
public:
    static AsyncWorkerEnumerateDevices* Create(Napi::Env env, const char* resourceName);

    Napi::Promise GetPromise()
    {
        return deferred_.Promise();
    }

protected:
    AsyncWorkerEnumerateDevices(Napi::Env env, const char* resourceName);

    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;

private:
    Napi::Promise::Deferred deferred_;

    std::vector<CameraDeviceInfo> cameraDevices_;
    std::vector<AudioDeviceInfo> audioDevices_;
};

} // namespace webrtc

#endif // WEBRTC_ASYNC_WORKER_ENUMERATE_DEVICES_H
