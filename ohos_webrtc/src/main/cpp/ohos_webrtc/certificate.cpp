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

#include "certificate.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

FunctionReference NapiCertificate::constructor_;

void NapiCertificate::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, "RTCCertificate",
        {
            InstanceAccessor<&NapiCertificate::GetExpires>("expires"),
            InstanceMethod<&NapiCertificate::GetFingerprints>("getFingerprints"),
        });
    exports.Set("RTCCertificate", func);

    constructor_ = Persistent(func);
}

Napi::Object NapiCertificate::NewInstance(Napi::Env env, rtc::scoped_refptr<rtc::RTCCertificate> certificate)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto externalCertificate =
        External<rtc::RTCCertificate>::New(env, certificate.release(), [](Napi::Env /*env*/, rtc::RTCCertificate* dc) {
            auto status = dc->Release();
            RTC_DLOG(LS_VERBOSE) << "RTCCertificate release status=" << status;
        });

    return constructor_.New({externalCertificate});
}

NapiCertificate::NapiCertificate(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NapiCertificate>(info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info[0].IsExternal()) {
        auto certificate = info[0].As<External<rtc::RTCCertificate>>().Data();
        certificate_ = rtc::scoped_refptr<rtc::RTCCertificate>(certificate);
        auto status = certificate->Release();
        RTC_LOG(LS_VERBOSE) << "RtpSenderInterface release status=" << status;
    }
}

rtc::scoped_refptr<rtc::RTCCertificate> NapiCertificate::Get() const
{
    return certificate_;
}

Napi::Value NapiCertificate::GetExpires(const Napi::CallbackInfo& info)
{
    // 30 * 24 * 60 * 60 * 1000
    return Number::New(info.Env(), certificate_->Expires());
}

Napi::Value NapiCertificate::GetFingerprints(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto size = certificate_->GetSSLCertificateChain().GetSize();

    Napi::Array array = Napi::Array::New(info.Env(), size);
    for (size_t i = 0; i < size; ++i) {
        auto stats = certificate_->GetSSLCertificateChain().Get(i).GetStats();
        auto statObj = Object::New(info.Env());

        statObj.Set("algorithm", String::New(info.Env(), stats->fingerprint_algorithm));
        statObj.Set("value", String::New(info.Env(), stats->fingerprint));

        array[i] = statObj;
    }

    return array;
}

} // namespace webrtc
