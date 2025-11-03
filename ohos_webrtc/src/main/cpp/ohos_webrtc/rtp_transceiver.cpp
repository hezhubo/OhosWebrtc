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

#include "rtp_transceiver.h"
#include "rtp_receiver.h"
#include "rtp_sender.h"
#include "media_stream.h"
#include "rtp_parameters.h"
#include "peer_connection.h"

#include "rtc_base/logging.h"
#include "api/rtp_transceiver_direction.h"

namespace webrtc {

using namespace Napi;

const char kClassName[] = "RTCRtpTransceiver";

const char kAttributeNameTrack[] = "mid";
const char kAttributeNameSender[] = "sender";
const char kAttributeNameReceiver[] = "receiver";
const char kAttributeNameDirection[] = "direction";
const char kAttributeNameCurrentDirection[] = "currentDirection";
const char kAttributeNameStreams[] = "streams";
const char kAttributeNameSendEncodings[] = "sendEncodings";
const char kAttributeNameActive[] = "active";
const char kAttributeNameMaxBitrate[] = "maxBitrate";
const char kAttributeNameMaxFramerate[] = "maxFramerate";
const char kAttributeNameScaleResolutionDownBy[] = "scaleResolutionDownBy";

const char kMethodNameStop[] = "stop";
const char kMethodNameSetCodecPreferences[] = "setCodecPreferences";
const char kMethodNameToJson[] = "toJSON";

const char kEnumRtpTransceiverDirectionInactive[] = "inactive";
const char kEnumRtpTransceiverDirectionRecvOnly[] = "recvonly";
const char kEnumRtpTransceiverDirectionSendOnly[] = "sendonly";
const char kEnumRtpTransceiverDirectionSendRecv[] = "sendrecv";
const char kEnumRtpTransceiverDirectionStopped[] = "stopped";

FunctionReference NapiRtpTransceiver::constructor_;
static constexpr int indexTwo = 2;
static constexpr int callbackInfoLen = 3;

void NapiRtpTransceiver::Init(Napi::Env env, Napi::Object exports)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiRtpTransceiver::GetMid>(kAttributeNameTrack),
            InstanceAccessor<&NapiRtpTransceiver::GetSender>(kAttributeNameSender),
            InstanceAccessor<&NapiRtpTransceiver::GetReceiver>(kAttributeNameReceiver),
            InstanceAccessor<&NapiRtpTransceiver::GetDirection, &NapiRtpTransceiver::SetDirection>(
                kAttributeNameDirection),
            InstanceAccessor<&NapiRtpTransceiver::GetCurrentDirection>(kAttributeNameCurrentDirection),
            InstanceMethod<&NapiRtpTransceiver::Stop>(kMethodNameStop),
            InstanceMethod<&NapiRtpTransceiver::SetCodecPreferences>(kMethodNameSetCodecPreferences),
            InstanceMethod<&NapiRtpTransceiver::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

Napi::Object NapiRtpTransceiver::NewInstance(
    std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<PeerConnectionInterface> pc,
    rtc::scoped_refptr<RtpTransceiverInterface> transceiver)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto env = constructor_.Env();
    if (!factory || !pc || !transceiver) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object());
    }

    Napi::HandleScope scope(env);
    return constructor_.New(
        {External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory),
         External<rtc::scoped_refptr<PeerConnectionInterface>>::New(env, &pc),
         External<rtc::scoped_refptr<RtpTransceiverInterface>>::New(env, &transceiver)});
}

NapiRtpTransceiver::NapiRtpTransceiver(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NapiRtpTransceiver>(info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // Create from native with 3 paramters, and SHOULD NOT create from ArkTS
    if (info.Length() != callbackInfoLen || !info[0].IsExternal() || !info[1].IsExternal() ||
        !info[indexTwo].IsExternal()) {
        NAPI_THROW_VOID(Napi::Error::New(info.Env(), "Invalid Operation"));
    }

    factory_ = *info[0].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();
    pc_ = *info[1].As<External<rtc::scoped_refptr<PeerConnectionInterface>>>().Data();
    rtpTransceiver_ = *info[indexTwo].As<External<rtc::scoped_refptr<RtpTransceiverInterface>>>().Data();
}

NapiRtpTransceiver::~NapiRtpTransceiver()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

rtc::scoped_refptr<RtpTransceiverInterface> NapiRtpTransceiver::Get() const
{
    return rtpTransceiver_;
}

Napi::Value NapiRtpTransceiver::GetMid(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto mid = rtpTransceiver_->mid();
    if (!mid) {
        return info.Env().Null();
    }

    return String::New(info.Env(), *mid);
}

Napi::Value NapiRtpTransceiver::GetSender(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return NapiRtpSender::NewInstance(factory_, pc_, rtpTransceiver_->sender());
}

Napi::Value NapiRtpTransceiver::GetReceiver(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return NapiRtpReceiver::NewInstance(factory_, pc_, rtpTransceiver_->receiver());
}

Napi::Value NapiRtpTransceiver::GetDirection(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto direction = rtpTransceiver_->direction();
    switch (direction) {
        case RtpTransceiverDirection::kInactive:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionInactive);
        case RtpTransceiverDirection::kRecvOnly:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionRecvOnly);
        case RtpTransceiverDirection::kSendOnly:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionSendOnly);
        case RtpTransceiverDirection::kSendRecv:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionSendRecv);
        case RtpTransceiverDirection::kStopped:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionStopped);
        default:
            RTC_LOG(LS_ERROR) << "Invalid direction: " << direction;
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid direction"), info.Env().Null());
}

void NapiRtpTransceiver::SetDirection(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!value.IsString()) {
        NAPI_THROW_VOID(Error::New(info.Env(), "The argument is not string"));
    }

    auto jsDirection = value.As<String>().Utf8Value();
    auto newDirection = RtpTransceiverDirection::kInactive;

    if (jsDirection == kEnumRtpTransceiverDirectionInactive) {
        newDirection = RtpTransceiverDirection::kInactive;
    } else if (jsDirection == kEnumRtpTransceiverDirectionRecvOnly) {
        newDirection = RtpTransceiverDirection::kRecvOnly;
    } else if (jsDirection == kEnumRtpTransceiverDirectionSendOnly) {
        newDirection = RtpTransceiverDirection::kSendOnly;
    } else if (jsDirection == kEnumRtpTransceiverDirectionSendRecv) {
        newDirection = RtpTransceiverDirection::kSendRecv;
    } else if (jsDirection == kEnumRtpTransceiverDirectionStopped) {
        newDirection = RtpTransceiverDirection::kStopped;
    }

    auto error = rtpTransceiver_->SetDirectionWithError(newDirection);
    if (!error.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to set direction: " << error.type() << ", " << error.message();
        NAPI_THROW_VOID(Error::New(info.Env(), "Failed to set direction"));
    }
}

Napi::Value NapiRtpTransceiver::GetCurrentDirection(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!rtpTransceiver_->current_direction()) {
        return info.Env().Null();
    }

    switch (rtpTransceiver_->current_direction().value()) {
        case RtpTransceiverDirection::kInactive:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionInactive);
        case RtpTransceiverDirection::kRecvOnly:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionRecvOnly);
        case RtpTransceiverDirection::kSendOnly:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionSendOnly);
        case RtpTransceiverDirection::kSendRecv:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionSendRecv);
        case RtpTransceiverDirection::kStopped:
            return String::New(info.Env(), kEnumRtpTransceiverDirectionStopped);
        default:
            RTC_LOG(LS_WARNING) << "Invalid value: " << rtpTransceiver_->current_direction().value();
            break;
    }

    return info.Env().Null();
}

Napi::Value NapiRtpTransceiver::Stop(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto error = rtpTransceiver_->StopStandard();
    if (!error.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to stop: " << error.type() << ", " << error.message();
        NAPI_THROW(Error::New(info.Env(), "Failed to stop"), info.Env().Undefined());
    }

    return info.Env().Undefined();
}

Napi::Value NapiRtpTransceiver::SetCodecPreferences(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsArray()) {
        NAPI_THROW(Error::New(info.Env(), "First argument is not array"), info.Env().Undefined());
    }

    auto jsCodecs = info[0].As<Array>();
    auto codecs = std::vector<RtpCodecCapability>();
    for (uint32_t i = 0; i < jsCodecs.Length(); i++) {
        Napi::Value jsCodec = jsCodecs[i];
        RtpCodecCapability codec;
        NapiRtpCodecCapability::JsToNative(jsCodec.As<Object>(), codec);
        if (info.Env().IsExceptionPending()) {
            NAPI_THROW(info.Env().GetAndClearPendingException(), info.Env().Undefined());
        }
        codecs.push_back(codec);
    }

    auto error = rtpTransceiver_->SetCodecPreferences(codecs);
    if (!error.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to set codec preferences: " << error.type() << ", " << error.message();
        NAPI_THROW(Error::New(info.Env(), "Failed to set codec preferences"), info.Env().Undefined());
    }

    return info.Env().Undefined();
}

Napi::Value NapiRtpTransceiver::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiRtpTransceiver");
#endif

    return json;
}

void PopulateTransceiverInit(const Object& obj, webrtc::RtpTransceiverInit& init)
{
    // Handle direction
    if (obj.Has(kAttributeNameDirection)) {
        std::string direction = obj.Get(kAttributeNameDirection).As<String>();
        if (direction == kEnumRtpTransceiverDirectionInactive) {
            init.direction = webrtc::RtpTransceiverDirection::kInactive;
        } else if (direction == kEnumRtpTransceiverDirectionRecvOnly) {
            init.direction = webrtc::RtpTransceiverDirection::kRecvOnly;
        } else if (direction == kEnumRtpTransceiverDirectionSendOnly) {
            init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
        } else if (direction == kEnumRtpTransceiverDirectionSendRecv) {
            init.direction = webrtc::RtpTransceiverDirection::kSendRecv;
        } else if (direction == kEnumRtpTransceiverDirectionStopped) {
            init.direction = webrtc::RtpTransceiverDirection::kStopped;
        }
    }

    // Handle streams
    if (obj.Has(kAttributeNameStreams)) {
        Array streamsArray = obj.Get(kAttributeNameStreams).As<Array>();
        for (uint32_t i = 0; i < streamsArray.Length(); i++) {
            Value streamValue = streamsArray.Get(i);
            if (streamValue.IsObject()) {
                auto napiStream = NapiMediaStream::Unwrap(streamValue.As<Object>());
                auto stream = napiStream->Get();
                init.stream_ids.push_back(stream->id());
            }
        }
    }

    // Handle sendEncodings
    if (obj.Has(kAttributeNameSendEncodings)) {
        Array encodingsArray = obj.Get(kAttributeNameSendEncodings).As<Array>();
        for (uint32_t i = 0; i < encodingsArray.Length(); i++) {
            Value encodingValue = encodingsArray.Get(i);
            if (encodingValue.IsObject()) {
                Object encodingObj = encodingValue.As<Object>();
                webrtc::RtpEncodingParameters encodingParams;
                NapiRtpEncodingParameters::JsToNative(encodingObj, encodingParams);
                init.send_encodings.push_back(encodingParams);
            }
        }
    }
}

} // namespace webrtc
