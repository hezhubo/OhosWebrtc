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

#ifndef WEBRTC_MEDIA_DEVICES_H
#define WEBRTC_MEDIA_DEVICES_H

#include "utils/marcos.h"

#include "napi.h"

namespace webrtc {

class NapiMediaDevices : public Napi::ObjectWrap<NapiMediaDevices> {
public:
    NAPI_CLASS_NAME_DECLARE(MediaDevices);
    NAPI_METHOD_NAME_DECLARE(EnumerateDevices, enumerateDevices);
    NAPI_METHOD_NAME_DECLARE(GetSupportedConstraints, getSupportedConstraints);
    NAPI_METHOD_NAME_DECLARE(GetUserMedia, getUserMedia);
    NAPI_METHOD_NAME_DECLARE(GetDisplayMedia, getDisplayMedia);
    NAPI_METHOD_NAME_DECLARE(ToJson, toJSON);

    static void Init(Napi::Env env, Napi::Object exports);

protected:
    friend class ObjectWrap;

    explicit NapiMediaDevices(const Napi::CallbackInfo& info);

    Napi::Value EnumerateDevices(const Napi::CallbackInfo& info);
    Napi::Value GetSupportedConstraints(const Napi::CallbackInfo& info);
    Napi::Value GetUserMedia(const Napi::CallbackInfo& info);
    Napi::Value GetDisplayMedia(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;
};

} // namespace webrtc

#endif // WEBRTC_MEDIA_DEVICES_H
