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

#ifndef WEBRTC_SESSION_DESCRIPTION_H
#define WEBRTC_SESSION_DESCRIPTION_H

#include "napi.h"

#include "api/jsep_session_description.h"

namespace webrtc {

class NapiSessionDescription : public Napi::ObjectWrap<NapiSessionDescription> {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(Napi::Env env, Napi::Value arg);
    static Napi::Object NewInstance(Napi::Env env, const std::string& sdp, SdpType type);

    explicit NapiSessionDescription(const Napi::CallbackInfo& info);

protected:
    Napi::Value GetSdp(const Napi::CallbackInfo& info);
    Napi::Value GetType(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;

    std::string sdp_;
    std::string type_;
};

} // namespace webrtc

#endif // WEBRTC_SESSION_DESCRIPTION_H
