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

#include "session_description.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

const char kAttributeNameSdp[] = "sdp";
const char kAttributeNameType[] = "type";

const char kMethodNameToJson[] = "toJSON";

const char kEnumSdpTypeOffer[] = "offer";
const char kEnumSdpTypeAnswer[] = "answer";
const char kEnumSdpTypePranswer[] = "pranswer";
const char kEnumSdpTypeRollback[] = "rollback";

FunctionReference NapiSessionDescription::constructor_;

void NapiSessionDescription::Init(Napi::Env env, Napi::Object exports)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, "RTCSessionDescription",
        {
            InstanceAccessor<&NapiSessionDescription::GetSdp>(kAttributeNameSdp),
            InstanceAccessor<&NapiSessionDescription::GetType>(kAttributeNameType),
            InstanceMethod<&NapiSessionDescription::ToJson>(kMethodNameToJson),
        });

    constructor_ = Persistent(func);

    exports.Set("RTCSessionDescription", func);
}

Napi::Object NapiSessionDescription::NewInstance(Napi::Env /*env*/, Napi::Value arg)
{
    return constructor_.New({arg});
}

Napi::Object NapiSessionDescription::NewInstance(Napi::Env env, const std::string& sdp, SdpType type)
{
    auto jsSdp = Object::New(env);
    jsSdp.Set(kAttributeNameSdp, String::New(env, sdp));
    switch (type) {
        case SdpType::kOffer:
            jsSdp.Set(kAttributeNameType, String::New(env, kEnumSdpTypeOffer));
            break;
        case SdpType::kAnswer:
            jsSdp.Set(kAttributeNameType, String::New(env, kEnumSdpTypeAnswer));
            break;
        case SdpType::kPrAnswer:
            jsSdp.Set(kAttributeNameType, String::New(env, kEnumSdpTypePranswer));
            break;
        case SdpType::kRollback:
            jsSdp.Set(kAttributeNameType, String::New(env, kEnumSdpTypeRollback));
            break;
        default:
            RTC_LOG(LS_WARNING) << "Invalid value of type";
            break;
    }

    return constructor_.New({jsSdp});
}

NapiSessionDescription::NapiSessionDescription(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NapiSessionDescription>(info)
{
    RTC_LOG(LS_INFO) << "NapiSessionDescription::NapiSessionDescription info.Length()=" << info.Length();
    if (info.Length() > 0) {
        auto jsSdpInit = info[0];
        if (jsSdpInit.IsObject()) {
            auto jsSdpInitObj = jsSdpInit.As<Object>();
            if (jsSdpInitObj.Has(kAttributeNameSdp)) {
                sdp_ = jsSdpInitObj.Get(kAttributeNameSdp).As<String>().Utf8Value();
            }
            type_ = jsSdpInitObj.Get(kAttributeNameType).As<String>().Utf8Value();
        }
    }
}

Napi::Value NapiSessionDescription::GetSdp(const Napi::CallbackInfo& info)
{
    return String::New(info.Env(), sdp_);
}

Napi::Value NapiSessionDescription::GetType(const Napi::CallbackInfo& info)
{
    return String::New(info.Env(), type_);
}

Napi::Value NapiSessionDescription::ToJson(const Napi::CallbackInfo& info)
{
    auto jsSdp = Object::New(info.Env());
    jsSdp.Set(kAttributeNameSdp, String::New(info.Env(), sdp_));
    jsSdp.Set(kAttributeNameType, String::New(info.Env(), type_));
    return jsSdp;
}

} // namespace webrtc
