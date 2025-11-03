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

#ifndef WEBRTC_CERTIFICATE_H
#define WEBRTC_CERTIFICATE_H

#include "napi.h"

#include "api/peer_connection_interface.h"

namespace webrtc {

class NapiCertificate : public Napi::ObjectWrap<NapiCertificate> {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(Napi::Env env, rtc::scoped_refptr<rtc::RTCCertificate> certificate);

    explicit NapiCertificate(const Napi::CallbackInfo& info);

    rtc::scoped_refptr<rtc::RTCCertificate> Get() const;

protected:
    Napi::Value GetExpires(const Napi::CallbackInfo& info);
    Napi::Value GetFingerprints(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;

    rtc::scoped_refptr<rtc::RTCCertificate> certificate_;
};

} // namespace webrtc

#endif // WEBRTC_CERTIFICATE_H
