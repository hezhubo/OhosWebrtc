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

#include "ice_candidate.h"

#include "pc/webrtc_sdp.h"
#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

// https://www.w3.org/TR/webrtc/#dfn-candidate-attribute
// https://www.rfc-editor.org/rfc/rfc5245#section-15.1
constexpr int32_t kComponentIdRtp = 1;
constexpr int32_t kComponentIdRtcp = 2;

const char kEnumIceCandidateTypeHost[] = "host";
const char kEnumIceCandidateTypeSrflx[] = "srflx";
const char kEnumIceCandidateTypePrflx[] = "prflx";
const char kEnumIceCandidateTypeRelay[] = "relay";

const char kEnumIceCandidateRtp[] = "rtp";
const char kEnumIceCandidateRtcp[] = "rtcp";

FunctionReference NapiIceCandidate::constructor_;

void NapiIceCandidate::Init(class Env env, Object exports)
{
    Function func = DefineClass(
        env, "RTCIceCandidate",
        {
            InstanceAccessor<&NapiIceCandidate::GetCandidate>("candidate"),
            InstanceAccessor<&NapiIceCandidate::GetSdpMid>("sdpMid"),
            InstanceAccessor<&NapiIceCandidate::GetSdpMLineIndex>("sdpMLineIndex"),
            InstanceAccessor<&NapiIceCandidate::GetUsernameFragment>("usernameFragment"),
            InstanceAccessor<&NapiIceCandidate::GetFoundation>("foundation"),
            InstanceAccessor<&NapiIceCandidate::GetComponent>("component"),
            InstanceAccessor<&NapiIceCandidate::GetPriority>("priority"),
            InstanceAccessor<&NapiIceCandidate::GetAddress>("address"),
            InstanceAccessor<&NapiIceCandidate::GetProtocol>("protocol"),
            InstanceAccessor<&NapiIceCandidate::GetPort>("port"),
            InstanceAccessor<&NapiIceCandidate::GetType>("type"),
            InstanceAccessor<&NapiIceCandidate::GetTcpType>("tcpType"),
            InstanceAccessor<&NapiIceCandidate::GetRelatedAddress>("relatedAddress"),
            InstanceAccessor<&NapiIceCandidate::GetRelatedPort>("relatedPort"),
            InstanceMethod<&NapiIceCandidate::ToJson>("toJSON"),
        });
    exports.Set("RTCIceCandidate", func);

    constructor_ = Persistent(func);
}

Napi::Object NapiIceCandidate::NewInstance(const Napi::CallbackInfo& info, IceCandidateInterface* candidate)
{
    // Unused
    NAPI_THROW(Error::New(info.Env(), "Unimplemented"), Object());
}

NapiIceCandidate::NapiIceCandidate(const Napi::CallbackInfo& info) : ObjectWrap<NapiIceCandidate>(info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1 || !info[0].IsObject()) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Wrong number of argument"));
    }

    auto from = info[0].As<Object>();
    if (!from.Has("sdpMid") && !from.Has("sdpMLineIndex")) {
        NAPI_THROW_VOID(Error::New(info.Env(), "TypeError"));
    }

    if (from.Has("sdpMid") && !from.Get("sdpMid").IsUndefined()) {
        sdpMid_ = from.Get("sdpMid").As<String>().Utf8Value();
    }

    if (from.Has("sdpMLineIndex") && !from.Get("sdpMLineIndex").IsUndefined()) {
        sdpMLineIndex_ = from.Get("sdpMLineIndex").As<Number>();
    }

    if (from.Has("candidate")) {
        auto sdp = from.Get("candidate").As<String>().Utf8Value();
        sdp_ = sdp;
        if (!sdp.empty()) {
            auto sdpMid = sdpMid_.value_or("");
            if (!SdpDeserializeCandidate(sdpMid, sdp, &candidate_, nullptr)) {
                NAPI_THROW_VOID(Error::New(info.Env(), "SdpDescrializeCandidate failed with sdp"));
            }
        }
    } else {
        NAPI_THROW_VOID(Error::New(info.Env(), "candidate is null"));
    }

    if (from.Has("usernameFragment")) {
        candidate_.set_username(from.Get("usernameFragment").As<String>().Utf8Value());
    }
}

Napi::Value NapiIceCandidate::GetCandidate(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return String::New(info.Env(), sdp_);
}

Napi::Value NapiIceCandidate::GetSdpMid(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (sdpMid_.has_value()) {
        return String::New(info.Env(), sdpMid_.value());
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetSdpMLineIndex(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (sdpMLineIndex_.has_value()) {
        return Number::New(info.Env(), sdpMLineIndex_.value());
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetUsernameFragment(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!candidate_.username().empty()) {
        return String::New(info.Env(), candidate_.username());
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetFoundation(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!candidate_.foundation().empty()) {
        return String::New(info.Env(), candidate_.foundation());
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetComponent(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (candidate_.component() == kComponentIdRtp) {
        return String::New(info.Env(), kEnumIceCandidateRtp);
    }
    if (candidate_.component() == kComponentIdRtcp) {
        return String::New(info.Env(), kEnumIceCandidateRtcp);
    }

    return info.Env().Undefined();
}

Napi::Value NapiIceCandidate::GetPriority(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return Number::New(info.Env(), candidate_.priority());
}

Napi::Value NapiIceCandidate::GetAddress(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!candidate_.address().hostname().empty()) {
        return String::New(info.Env(), candidate_.address().hostname());
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetProtocol(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!candidate_.protocol().empty()) {
        return String::New(info.Env(), candidate_.protocol());
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetPort(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return Number::New(info.Env(), candidate_.address().port());
}

Napi::Value NapiIceCandidate::GetType(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto env = info.Env();

    if (candidate_.type() == cricket::LOCAL_PORT_TYPE) {
        return String::New(env, kEnumIceCandidateTypeHost);
    } else if (candidate_.type() == cricket::STUN_PORT_TYPE) {
        return String::New(env, kEnumIceCandidateTypeSrflx);
    } else if (candidate_.type() == cricket::RELAY_PORT_TYPE) {
        return String::New(env, kEnumIceCandidateTypeRelay);
    } else if (candidate_.type() == cricket::PRFLX_PORT_TYPE) {
        return String::New(env, kEnumIceCandidateTypePrflx);
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetTcpType(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!candidate_.tcptype().empty()) {
        return String::New(info.Env(), candidate_.tcptype());
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetRelatedAddress(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!candidate_.related_address().hostname().empty()) {
        return String::New(info.Env(), candidate_.related_address().hostname());
    } else {
        return info.Env().Undefined();
    }
}

Napi::Value NapiIceCandidate::GetRelatedPort(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return Number::New(info.Env(), candidate_.related_address().port());
}

Value NapiIceCandidate::ToJson(const CallbackInfo& info)
{
    auto from = info.This().As<Object>();
    auto to = Object::New(info.Env());

    to.Set("candidate", from.Get("candidate"));
    if (from.Has("sdpMLineIndex"))
        to.Set("sdpMLineIndex", from.Get("sdpMLineIndex"));
    if (from.Has("sdpMid"))
        to.Set("sdpMid", from.Get("sdpMid"));
    if (from.Has("usernameFragment"))
        to.Set("usernameFragment", from.Get("usernameFragment"));

    return to;
}

cricket::Candidate JsToNativeCandidate(Napi::Env env, const Napi::Object& jsCandidate)
{
    auto sdpMid = jsCandidate.Has("sdpMid") ? jsCandidate.Get("sdpMid").As<String>().Utf8Value() : std::string();
    auto sdp = jsCandidate.Get("candidate").As<String>().Utf8Value();

    cricket::Candidate candidate;
    if (!SdpDeserializeCandidate(sdpMid, sdp, &candidate, nullptr)) {
        RTC_LOG(LS_ERROR) << "SdpDescrializeCandidate failed with sdp " << sdp;
    }

    return candidate;
}

Napi::Object NativeToJsCandidate(
    Napi::Env env, const std::string& sdpMid, int sdpMLineIndex, const std::string& sdp,
    const cricket::Candidate& candidate)
{
    if (sdp.empty()) {
        RTC_LOG(LS_ERROR) << "got an empty ICE candidate";
        return Object::New(env);
    }

    auto obj = Object::New(env);
    obj.Set("sdpMLineIndex", Number::New(env, sdpMLineIndex));
    obj.Set("sdpMid", String::New(env, sdpMid));
    obj.Set("candidate", String::New(env, sdp));
    obj.Set("foundation", String::New(env, candidate.foundation()));
    if (candidate.component() == kComponentIdRtp) {
        obj.Set("component", String::New(env, kEnumIceCandidateRtp));
    }
    if (candidate.component() == kComponentIdRtcp) {
        obj.Set("component", String::New(env, kEnumIceCandidateRtcp));
    }
    obj.Set("priority", Number::New(env, candidate.priority()));
    obj.Set("address", String::New(env, candidate.address().hostname()));
    obj.Set("protocol", String::New(env, candidate.protocol()));
    obj.Set("port", Number::New(env, candidate.address().port()));
    obj.Set("tcpType", String::New(env, candidate.tcptype()));
    obj.Set("relatedAddress", String::New(env, candidate.related_address().hostname()));
    obj.Set("relatedPort", Number::New(env, candidate.related_address().port()));
    obj.Set("usernameFragment", String::New(env, candidate.username()));

    if (candidate.type() == cricket::LOCAL_PORT_TYPE) {
        obj.Set("type", String::New(env, kEnumIceCandidateTypeHost));
    } else if (candidate.type() == cricket::STUN_PORT_TYPE) {
        obj.Set("type", String::New(env, kEnumIceCandidateTypeSrflx));
    } else if (candidate.type() == cricket::RELAY_PORT_TYPE) {
        obj.Set("type", String::New(env, kEnumIceCandidateTypeRelay));
    } else if (candidate.type() == cricket::PRFLX_PORT_TYPE) {
        obj.Set("type", String::New(env, kEnumIceCandidateTypePrflx));
    } else {
        // invalid type
    }

    // extension attribute
    obj.Set("adapterType", String::New(env, rtc::AdapterTypeToString(candidate.network_type())));
    obj.Set("serverUrl", String::New(env, candidate.url()));

    // instance method of IceCandidate
    obj.Set("toJSON", Function::New(env, [](const CallbackInfo& info) {
                auto from = info.This().As<Object>();
                auto to = Object::New(info.Env());
                RTC_LOG(LS_VERBOSE) << __FUNCTION__ << " toJson";

                to.Set("candidate", from.Get("candidate"));
                if (from.Has("sdpMLineIndex"))
                    to.Set("sdpMLineIndex", from.Get("sdpMLineIndex"));
                if (from.Has("sdpMid"))
                    to.Set("sdpMid", from.Get("sdpMid"));
                if (from.Has("usernameFragment"))
                    to.Set("usernameFragment", from.Get("usernameFragment"));

                return to;
            }));

    return obj;
}

Napi::Object NativeToJsCandidate(Napi::Env env, const cricket::Candidate& candidate)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    auto obj = Object::New(env);

    obj.Set("foundation", String::New(env, candidate.foundation()));
    if (candidate.component() == kComponentIdRtp) {
        obj.Set("component", String::New(env, kEnumIceCandidateRtp));
    }
    if (candidate.component() == kComponentIdRtcp) {
        obj.Set("component", String::New(env, kEnumIceCandidateRtcp));
    }
    obj.Set("priority", Number::New(env, candidate.priority()));
    obj.Set("address", String::New(env, candidate.address().hostname()));
    obj.Set("protocol", String::New(env, candidate.protocol()));
    obj.Set("port", Number::New(env, candidate.address().port()));
    obj.Set("tcpType", String::New(env, candidate.tcptype()));
    obj.Set("relatedAddress", String::New(env, candidate.related_address().hostname()));
    obj.Set("relatedPort", Number::New(env, candidate.related_address().port()));
    obj.Set("usernameFragment", String::New(env, candidate.username()));

    if (candidate.type() == cricket::LOCAL_PORT_TYPE) {
        obj.Set("type", String::New(env, kEnumIceCandidateTypeHost));
    } else if (candidate.type() == cricket::STUN_PORT_TYPE) {
        obj.Set("type", String::New(env, kEnumIceCandidateTypeSrflx));
    } else if (candidate.type() == cricket::RELAY_PORT_TYPE) {
        obj.Set("type", String::New(env, kEnumIceCandidateTypeRelay));
    } else if (candidate.type() == cricket::PRFLX_PORT_TYPE) {
        obj.Set("type", String::New(env, kEnumIceCandidateTypePrflx));
    } else {
        // invalid type
    }

    // extension attribute
    obj.Set("adapterType", String::New(env, rtc::AdapterTypeToString(candidate.network_type())));
    obj.Set("serverUrl", String::New(env, candidate.url()));

    // instance method of IceCandidate
    obj.Set("toJSON", Function::New(env, [](const CallbackInfo& info) {
                auto from = info.This().As<Object>();
                auto to = Object::New(info.Env());
                RTC_LOG(LS_VERBOSE) << __FUNCTION__ << " toJson";

                to.Set("candidate", from.Get("candidate"));
                if (from.Has("sdpMLineIndex"))
                    to.Set("sdpMLineIndex", from.Get("sdpMLineIndex"));
                if (from.Has("sdpMid"))
                    to.Set("sdpMid", from.Get("sdpMid"));
                if (from.Has("usernameFragment"))
                    to.Set("usernameFragment", from.Get("usernameFragment"));

                return to;
            }));

    return obj;
}

Napi::Object NativeToJsIceCandidate(Napi::Env env, const IceCandidateInterface& candidate)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    std::string sdp;
    if (!candidate.ToString(&sdp)) {
        RTC_LOG(LS_ERROR) << "got so far: " << sdp;
    }

    auto obj = Object::New(env);
    obj.Set("sdpMLineIndex", Number::New(env, candidate.sdp_mline_index()));
    obj.Set("sdpMid", String::New(env, candidate.sdp_mid()));
    obj.Set("candidate", String::New(env, sdp));

    RTC_LOG(LS_VERBOSE) << __FUNCTION__ << " xxx";
    // instance method of IceCandidate
    obj.Set("toJSON", Function::New(env, [](const CallbackInfo& info) {
                auto from = info.This().As<Object>();
                auto to = Object::New(info.Env());
                RTC_LOG(LS_VERBOSE) << __FUNCTION__ << " toJson";

                to.Set("candidate", from.Get("candidate"));
                if (from.Has("sdpMLineIndex"))
                    to.Set("sdpMLineIndex", from.Get("sdpMLineIndex"));
                if (from.Has("sdpMid"))
                    to.Set("sdpMid", from.Get("sdpMid"));
                if (from.Has("usernameFragment"))
                    to.Set("usernameFragment", from.Get("usernameFragment"));

                return to;
            }));

    RTC_LOG(LS_VERBOSE) << __FUNCTION__ << " exit";

    return obj;
}

} // namespace webrtc
