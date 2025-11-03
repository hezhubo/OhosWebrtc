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

#include "rtp_sender.h"
#include "media_stream.h"
#include "media_stream_track.h"
#include "dtls_transport.h"
#include "dtmf_sender.h"
#include "rtp_parameters.h"
#include "peer_connection_factory.h"
#include "async_work/async_worker_get_stats.h"
#include "peer_connection.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

const char kClassName[] = "RTCRtpSender";

const char kAttributeNameTrack[] = "track";
const char kAttributeNameTransport[] = "transport";
const char kAttributeNameDtmf[] = "dtmf";

const char kMethodNameSetParameters[] = "setParameters";
const char kMethodNameGetParameters[] = "getParameters";
const char kMethodNameReplaceTrack[] = "replaceTrack";
const char kMethodNameSetStreams[] = "setStreams";
const char kMethodNameGetStats[] = "getStats";
const char kMethodNameToJson[] = "toJSON";

const char kStaticMethodNameGetCapabilities[] = "getCapabilities";
static constexpr int indexTwo = 2;
static constexpr int callbackInfoLen = 3;

class AsyncWorkerReplaceTrack : public Napi::AsyncWorker {
public:
    static Napi::Value DoWork(
        Napi::Env env, rtc::scoped_refptr<RtpSenderInterface> rtpSender,
        rtc::scoped_refptr<MediaStreamTrackInterface> track)
    {
        auto worker = new AsyncWorkerReplaceTrack(env, rtpSender, track);
        worker->Queue();
        return worker->deferred_.Promise();
    }

protected:
    AsyncWorkerReplaceTrack(
        Napi::Env env, rtc::scoped_refptr<RtpSenderInterface> rtpSender,
        rtc::scoped_refptr<MediaStreamTrackInterface> track)
        : Napi::AsyncWorker(env, "replaceTrack"),
          env_(env),
          deferred_(Napi::Promise::Deferred::New(env)),
          rtpSender_(rtpSender),
          track_(track)
    {
    }

    void Execute() override
    {
        if (!rtpSender_->SetTrack(track_.get())) {
            SetError("Unknown error");
        }
    }

    void OnOK() override
    {
        deferred_.Resolve(env_.Undefined());
    }

    void OnError(const Napi::Error& e) override
    {
        deferred_.Reject(e.Value());
    }

private:
    Napi::Env env_;
    Napi::Promise::Deferred deferred_;
    rtc::scoped_refptr<RtpSenderInterface> rtpSender_;
    rtc::scoped_refptr<MediaStreamTrackInterface> track_;
};

FunctionReference NapiRtpSender::constructor_;

void NapiRtpSender::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiRtpSender::GetTrack>(kAttributeNameTrack),
            InstanceAccessor<&NapiRtpSender::GetTransport>(kAttributeNameTransport),
            InstanceAccessor<&NapiRtpSender::GetDtmf>(kAttributeNameDtmf),
            InstanceMethod<&NapiRtpSender::SetParameters>(kMethodNameSetParameters),
            InstanceMethod<&NapiRtpSender::GetParameters>(kMethodNameGetParameters),
            InstanceMethod<&NapiRtpSender::ReplaceTrack>(kMethodNameReplaceTrack),
            InstanceMethod<&NapiRtpSender::SetStreams>(kMethodNameSetStreams),
            InstanceMethod<&NapiRtpSender::GetStats>(kMethodNameGetStats),
            InstanceMethod<&NapiRtpSender::ToJson>(kMethodNameToJson),
            StaticMethod<&NapiRtpSender::GetCapabilities>(kStaticMethodNameGetCapabilities),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

Napi::Object NapiRtpSender::NewInstance(
    std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<PeerConnectionInterface> pc,
    rtc::scoped_refptr<RtpSenderInterface> sender)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto env = constructor_.Env();
    if (!factory || !pc || !sender) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object());
    }

    Napi::HandleScope scope(env);
    return constructor_.New(
        {External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory),
         External<rtc::scoped_refptr<PeerConnectionInterface>>::New(env, &pc),
         External<rtc::scoped_refptr<RtpSenderInterface>>::New(env, &sender)});
}

NapiRtpSender::NapiRtpSender(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NapiRtpSender>(info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // Create from native with 3 paramters, and SHOULD NOT create from ArkTS
    if (info.Length() != callbackInfoLen || !info[0].IsExternal() || !info[1].IsExternal() ||
        !info[indexTwo].IsExternal()) {
        NAPI_THROW_VOID(Napi::Error::New(info.Env(), "Invalid Operation"));
    }

    factory_ = *info[0].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();
    pc_ = *info[1].As<External<rtc::scoped_refptr<PeerConnectionInterface>>>().Data();
    rtpSender_ = *info[indexTwo].As<External<rtc::scoped_refptr<RtpSenderInterface>>>().Data();
}

NapiRtpSender::~NapiRtpSender()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

rtc::scoped_refptr<RtpSenderInterface> NapiRtpSender::Get() const
{
    return rtpSender_;
}

Napi::Value NapiRtpSender::GetCapabilities(const Napi::CallbackInfo& info)
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

    auto capabilities = factoryWrapper->GetFactory()->GetRtpSenderCapabilities(mediaType);
    auto jsCapabilities = Object::New(info.Env());
    NapiRtpCapabilities::NativeToJs(capabilities, jsCapabilities);

    return jsCapabilities;
}

Napi::Value NapiRtpSender::GetTrack(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto track = rtpSender_->track();
    if (!track) {
        return info.Env().Null();
    }

    return NapiMediaStreamTrack::NewInstance(factory_, track);
}

Napi::Value NapiRtpSender::GetTransport(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto transport = rtpSender_->dtls_transport();
    if (!transport) {
        return info.Env().Null();
    }

    return NapiDtlsTransport::NewInstance(info.Env(), factory_, transport);
}

Napi::Value NapiRtpSender::GetDtmf(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto sender = rtpSender_->GetDtmfSender();
    if (!sender) {
        return info.Env().Null();
    }

    return NapiDtmfSender::NewInstance(info.Env(), sender);
}

Napi::Value NapiRtpSender::SetParameters(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() == 0) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsObject()) {
        NAPI_THROW(TypeError::New(info.Env(), "First argument is not object"), info.Env().Undefined());
    }

    auto jsParameters = info[0].As<Object>();
    RtpParameters parameters;
    NapiRtpSendParameters::JsToNative(jsParameters, parameters);
    if (info.Env().IsExceptionPending()) {
        NAPI_THROW(info.Env().GetAndClearPendingException(), info.Env().Undefined());
    }

    auto deferred = Promise::Deferred::New(info.Env());
    auto tsfn = ThreadSafeFunction::New(
        info.Env(),
        Function::New(
            info.Env(),
            [deferred](const CallbackInfo& info) {
                auto error = info[0].As<External<RTCError>>().Data();
                if (error->ok()) {
                    deferred.Resolve(info.Env().Undefined());
                } else {
                    auto type = error->type();
                    auto message = error->message();
                    RTC_LOG(LS_ERROR) << "SetParametersAsync failed: " << type << ", " << message;
                    deferred.Reject(
                        Error::New(info.Env(), (message && strlen(message) > 0) ? message : "Unknown error").Value());
                }
            }),
        "SetParametersAsync", 0, 1);

    rtpSender_->SetParametersAsync(parameters, [tsfn](RTCError error) {
        RTC_DLOG(LS_INFO) << "SetParametersAsync complete: " << error.ok();
        tsfn.BlockingCall([error = new RTCError(std::move(error))](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({External<RTCError>::New(env, error, [](Napi::Env, RTCError* e) { delete e; })});
        });
        tsfn.Release();
    });

    return deferred.Promise();
}

Napi::Value NapiRtpSender::GetParameters(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto jsParameters = Object::New(info.Env());
    NapiRtpSendParameters::NativeToJs(rtpSender_->GetParameters(), jsParameters);

    return jsParameters;
}

Napi::Value NapiRtpSender::ReplaceTrack(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    rtc::scoped_refptr<MediaStreamTrackInterface> track;
    if (info.Length() > 0 && info[0].IsObject()) {
        auto napiTrack = NapiMediaStreamTrack::Unwrap(info[0].As<Object>());
        if (napiTrack) {
            track = napiTrack->Get();
        }
    }

    return AsyncWorkerReplaceTrack::DoWork(info.Env(), rtpSender_, track);
}

Napi::Value NapiRtpSender::SetStreams(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    std::vector<std::string> streamsIds;
    for (uint32_t i = 0; i < info.Length(); i++) {
        Napi::Value jsStream = info[i];
        if (!jsStream.IsObject()) {
            continue;
        }

        auto stream = NapiMediaStream::Unwrap(jsStream.As<Object>());
        if (!stream || !stream->Get()) {
            continue;
        }

        streamsIds.push_back(stream->Get()->id());
    }

    rtpSender_->SetStreams(streamsIds);

    return info.Env().Undefined();
}

Napi::Value NapiRtpSender::GetStats(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto asyncWorker = AsyncWorkerGetStats::Create(info.Env(), "GetStats");
    pc_->GetStats(rtpSender_, asyncWorker->GetCallback());

    return asyncWorker->GetPromise();
}

Napi::Value NapiRtpSender::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiRtpSender");
#endif

    return json;
}

} // namespace webrtc