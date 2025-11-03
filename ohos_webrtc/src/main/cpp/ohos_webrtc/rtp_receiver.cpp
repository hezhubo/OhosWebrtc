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

#include "rtp_receiver.h"
#include "media_stream_track.h"
#include "peer_connection_factory.h"
#include "dtls_transport.h"
#include "rtp_parameters.h"
#include "peer_connection.h"

#include <limits>

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

const char kClassName[] = "RTCRtpReceiver";

const char kAttributeNameTrack[] = "track";
const char kAttributeNameTransport[] = "transport";

const char kMethodNameGetParameters[] = "getParameters";
const char kMethodNameGetStats[] = "getStats";
const char kMethodNameGetContributingSources[] = "getContributingSources";
const char kMethodNameGetSynchronizationSources[] = "getSynchronizationSources";
const char kMethodNameToJson[] = "toJSON";

const char kStaticMethodNameGetCapabilities[] = "getCapabilities";

const char kAttributeNameTimestamp[] = "timestamp";
const char kAttributeNameRtpTimestamp[] = "rtpTimestamp";
const char kAttributeNameSource[] = "source";
const char kAttributeNameAudioLevel[] = "audioLevel";

/// NapiRtpReceiver
FunctionReference NapiRtpReceiver::constructor_;

static constexpr int indexTwo = 2;
static constexpr int callbackInfoLen = 3;

void NapiRtpReceiver::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiRtpReceiver::GetTrack>(kAttributeNameTrack),
            InstanceAccessor<&NapiRtpReceiver::GetTransport>(kAttributeNameTransport),
            InstanceMethod<&NapiRtpReceiver::GetParameters>(kMethodNameGetParameters),
            InstanceMethod<&NapiRtpReceiver::GetStats>(kMethodNameGetStats),
            InstanceMethod<&NapiRtpReceiver::GetContributingSources>(kMethodNameGetContributingSources),
            InstanceMethod<&NapiRtpReceiver::GetSynchronizationSources>(kMethodNameGetSynchronizationSources),
            InstanceMethod<&NapiRtpReceiver::ToJson>(kMethodNameToJson),
            StaticMethod<&NapiRtpReceiver::GetCapabilities>(kStaticMethodNameGetCapabilities),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

Napi::Object NapiRtpReceiver::NewInstance(
    std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<PeerConnectionInterface> pc,
    rtc::scoped_refptr<RtpReceiverInterface> receiver)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto env = constructor_.Env();
    if (!factory || !pc || !receiver) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object());
    }

    return constructor_.New(
        {External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory),
         External<rtc::scoped_refptr<PeerConnectionInterface>>::New(env, &pc),
         External<rtc::scoped_refptr<RtpReceiverInterface>>::New(env, &receiver)});
}

NapiRtpReceiver::NapiRtpReceiver(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NapiRtpReceiver>(info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // Create from native with 3 paramters, and SHOULD NOT create from ArkTS
    if (info.Length() != callbackInfoLen || !info[0].IsExternal() || !info[1].IsExternal() ||
        !info[indexTwo].IsExternal()) {
        NAPI_THROW_VOID(Napi::Error::New(info.Env(), "Invalid Operation"));
    }

    factory_ = *info[0].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();
    pc_ = *info[1].As<External<rtc::scoped_refptr<PeerConnectionInterface>>>().Data();
    rtpReceiver_ = *info[indexTwo].As<External<rtc::scoped_refptr<RtpReceiverInterface>>>().Data();
}

NapiRtpReceiver::~NapiRtpReceiver()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

rtc::scoped_refptr<RtpReceiverInterface> NapiRtpReceiver::Get() const
{
    return rtpReceiver_;
}

Napi::Value NapiRtpReceiver::GetCapabilities(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Null());
    }

    if (!info[0].IsString()) {
        NAPI_THROW(Error::New(info.Env(), "First argument is not string"), info.Env().Null());
    }

    auto kind = info[0].As<String>().Utf8Value();
    auto mediaType = cricket::MEDIA_TYPE_UNSUPPORTED;
    if (kind == cricket::kMediaTypeAudio) {
        mediaType = cricket::MEDIA_TYPE_AUDIO;
    } else if (kind == cricket::kMediaTypeVideo) {
        mediaType = cricket::MEDIA_TYPE_VIDEO;
    } else {
        return info.Env().Null();
    }

    auto factoryWrapper = PeerConnectionFactoryWrapper::GetDefault();
    if (!factoryWrapper) {
        NAPI_THROW(Error::New(info.Env(), "Internal error"), info.Env().Null());
    }

    auto capabilities = factoryWrapper->GetFactory()->GetRtpReceiverCapabilities(mediaType);
    auto jsCapabilities = Object::New(info.Env());
    NapiRtpCapabilities::NativeToJs(capabilities, jsCapabilities);

    return jsCapabilities;
}

Napi::Value NapiRtpReceiver::GetTrack(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto track = rtpReceiver_->track();
    if (!track) {
        NAPI_THROW(Error::New(info.Env(), "No track"), info.Env().Undefined());
    }

    return NapiMediaStreamTrack::NewInstance(factory_, track);
}

Napi::Value NapiRtpReceiver::GetTransport(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto transport = rtpReceiver_->dtls_transport();
    if (!transport) {
        return info.Env().Null();
    }

    return NapiDtlsTransport::NewInstance(info.Env(), factory_, transport);
}

Napi::Value NapiRtpReceiver::GetParameters(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto jsParameters = Object::New(info.Env());
    NapiRtpSendParameters::NativeToJs(rtpReceiver_->GetParameters(), jsParameters);

    return jsParameters;
}

Napi::Value NapiRtpReceiver::GetStats(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto asyncWorker = AsyncWorkerGetStats::Create(info.Env(), "GetStats");
    pc_->GetStats(rtpReceiver_, asyncWorker->GetCallback());

    return asyncWorker->GetPromise();
}

Napi::Value NapiRtpReceiver::GetContributingSources(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto sources = rtpReceiver_->GetSources();

    std::vector<RtpSource> csrcs;
    for (const auto& source : sources) {
        if (source.source_type() == RtpSourceType::CSRC) {
            csrcs.push_back(source);
        }
    }

    auto jsCsrcs = Array::New(info.Env(), csrcs.size());
    for (uint32_t i = 0; i < jsCsrcs.Length(); i++) {
        const auto& source = csrcs[i];

        auto jsSource = Object::New(info.Env());
        jsSource.Set(kAttributeNameTimestamp, Number::New(info.Env(), source.timestamp().ms()));
        jsSource.Set(kAttributeNameRtpTimestamp, Number::New(info.Env(), source.rtp_timestamp()));
        jsSource.Set(kAttributeNameSource, Number::New(info.Env(), source.source_id()));

        auto audioLevel = source.audio_level();
        if (audioLevel) {
            jsSource.Set(kAttributeNameAudioLevel, Number::New(info.Env(), audioLevel.value() / UINT8_MAX));
        }

        jsCsrcs.Set(i, jsSource);
    }

    return jsCsrcs;
}

Napi::Value NapiRtpReceiver::GetSynchronizationSources(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto sources = rtpReceiver_->GetSources();

    std::vector<RtpSource> ssrcs;
    for (const auto& source : sources) {
        if (source.source_type() == RtpSourceType::SSRC) {
            ssrcs.push_back(source);
        }
    }

    auto jsSsrcs = Array::New(info.Env(), ssrcs.size());
    for (uint32_t i = 0; i < jsSsrcs.Length(); i++) {
        const auto& source = ssrcs[i];

        auto jsSource = Object::New(info.Env());
        jsSource.Set(kAttributeNameTimestamp, Number::New(info.Env(), source.timestamp().ms()));
        jsSource.Set(kAttributeNameRtpTimestamp, Number::New(info.Env(), source.rtp_timestamp()));
        jsSource.Set(kAttributeNameSource, Number::New(info.Env(), source.source_id()));

        auto audioLevel = source.audio_level();
        if (audioLevel) {
            jsSource.Set(kAttributeNameAudioLevel, Number::New(info.Env(), audioLevel.value() / UINT8_MAX));
        }

        jsSsrcs.Set(i, jsSource);
    }

    return jsSsrcs;
}

Napi::Value NapiRtpReceiver::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiRtpReceiver");
#endif

    return json;
}

} // namespace webrtc