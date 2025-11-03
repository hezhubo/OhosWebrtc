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

#include "media_stream_track.h"
#include "media_source.h"
#include "peer_connection_factory.h"
#include "audio_device/ohos_local_audio_source.h"
#include "video/video_track_source.h"

#include "rtc_base/logging.h"
#include "utils/marcos.h"

namespace webrtc {

using namespace Napi;

const char kClassName[] = "MediaStreamTrack";

const char kAttributeNameId[] = "id";
const char kAttributeNameKind[] = "kind";
const char kAttributeNameLabel[] = "label";
const char kAttributeNameEnabled[] = "enabled";
const char kAttributeNameMuted[] = "muted";
const char kAttributeNameReadyState[] = "readyState";
const char kAttributeNameOnMute[] = "onmute";
const char kAttributeNameOnUnmute[] = "onunmute";
const char kAttributeNameOnEnded[] = "onended";

const char kMethodNameStop[] = "stop";
const char kMethodNameGetSource[] = "getSource";
const char kMethodNameToJson[] = "toJSON";

const char kEnumMediaStreamTrackStateLive[] = "live";
const char kEnumMediaStreamTrackStateEnded[] = "ended";

const char kEventNameMute[] = "mute";
const char kEventNameUnmute[] = "unmute";
const char kEventNameEnded[] = "ended";

void NapiMediaStreamTrack::Init(Napi::Env env, Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiMediaStreamTrack::GetKind>(kAttributeNameKind),
            InstanceAccessor<&NapiMediaStreamTrack::GetId>(kAttributeNameId),
            InstanceAccessor<&NapiMediaStreamTrack::GetEnabled, &NapiMediaStreamTrack::SetEnabled>(
                kAttributeNameEnabled),
            InstanceAccessor<&NapiMediaStreamTrack::GetReadyState>(kAttributeNameReadyState),
            InstanceMethod<&NapiMediaStreamTrack::Stop>(kMethodNameStop),
            InstanceMethod<&NapiMediaStreamTrack::GetSource>(kMethodNameGetSource),
            InstanceMethod<&NapiMediaStreamTrack::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    Constructor() = Persistent(func);
}

Napi::Object NapiMediaStreamTrack::NewInstance(
    std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<MediaStreamTrackInterface> track)
{
    auto env = Constructor().Env();
    if (!factory || !track) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object::New(env));
    }

    return Constructor().New(
        {External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory),
         Napi::External<rtc::scoped_refptr<MediaStreamTrackInterface>>::New(env, &track)});
}

Napi::FunctionReference& NapiMediaStreamTrack::Constructor()
{
    static Napi::FunctionReference constructor;
    return constructor;
}

NapiMediaStreamTrack::NapiMediaStreamTrack(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NapiMediaStreamTrack>(info)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    auto env = info.Env();
    // Create from native with 2 paramters, and SHOULD NOT create from ArkTS
    if (info.Length() != 2 || !info[0].IsExternal() || !info[1].IsExternal()) {
        NAPI_THROW_VOID(Napi::Error::New(info.Env(), "Invalid Operation"));
    }

    factory_ = *info[0].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();
    track_ = *info[1].As<External<rtc::scoped_refptr<MediaStreamTrackInterface>>>().Data();

    track_->RegisterObserver(this);
}

NapiMediaStreamTrack::~NapiMediaStreamTrack()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    track_->UnregisterObserver(this);

    RemoveAllVideoSinks();

    if (IsAudioTrack()) {
        factory_->RemoveAudioSource(track_);
    } else if (IsVideoTrack()) {
        factory_->RemoveVideoSource(track_);
    }
}

bool NapiMediaStreamTrack::IsAudioTrack() const
{
    return (track_->kind() == MediaStreamTrackInterface::kAudioKind);
}

bool NapiMediaStreamTrack::IsVideoTrack() const
{
    return (track_->kind() == MediaStreamTrackInterface::kVideoKind);
}

AudioTrackInterface* NapiMediaStreamTrack::GetAudioTrack() const
{
    if (!IsAudioTrack()) {
        RTC_LOG(LS_WARNING) << "Not a audio track";
        return nullptr;
    }
    return static_cast<AudioTrackInterface*>(track_.get());
}

VideoTrackInterface* NapiMediaStreamTrack::GetVideoTrack() const
{
    if (!IsVideoTrack()) {
        RTC_LOG(LS_WARNING) << "Not a video track";
        return nullptr;
    }
    return static_cast<VideoTrackInterface*>(track_.get());
}

void NapiMediaStreamTrack::AddSink(rtc::VideoSinkInterface<VideoFrame>* sink)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    AddVideoSink(sink);
}

void NapiMediaStreamTrack::RemoveSink(rtc::VideoSinkInterface<VideoFrame>* sink)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RemoveVideoSink(sink);
}

void NapiMediaStreamTrack::AddVideoSink(rtc::VideoSinkInterface<VideoFrame>* sink)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!IsVideoTrack()) {
        RTC_LOG(LS_WARNING) << "Not a video track";
        return;
    }

    {
        UNUSED std::lock_guard<std::mutex> lock(sinksMutex_);
        auto ret = videoSinks_.insert(sink);
        if (!ret.second) {
            RTC_LOG(LS_WARNING) << "Failed to insert video sink";
            return;
        }
    }

    auto videoTrack = static_cast<VideoTrackInterface*>(track_.get());
    videoTrack->AddOrUpdateSink(sink, rtc::VideoSinkWants());
}

void NapiMediaStreamTrack::RemoveVideoSink(rtc::VideoSinkInterface<VideoFrame>* sink)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!IsVideoTrack()) {
        RTC_LOG(LS_WARNING) << "Not a video track";
        return;
    }

    {
        UNUSED std::lock_guard<std::mutex> lock(sinksMutex_);
        if (videoSinks_.erase(sink) == 0) {
            RTC_LOG(LS_WARNING) << "Failed to erase video sink";
            return;
        }
    }

    auto videoTrack = static_cast<VideoTrackInterface*>(track_.get());
    videoTrack->RemoveSink(sink);
}

void NapiMediaStreamTrack::RemoveAllVideoSinks()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!IsVideoTrack()) {
        return;
    }

    std::set<rtc::VideoSinkInterface<VideoFrame>*> sinks;
    {
        UNUSED std::lock_guard<std::mutex> lock(sinksMutex_);
        videoSinks_.swap(sinks);
    }

    auto videoTrack = static_cast<VideoTrackInterface*>(track_.get());
    for (auto& sink : sinks) {
        if (sink) {
            videoTrack->RemoveSink(sink);
        }
    }
}

void NapiMediaStreamTrack::OnChanged()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << "(" << track_->kind() << ") state=" << track_->state()
                      << ", enabled=" << track_->enabled();
}

Napi::Value NapiMediaStreamTrack::GetId(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!track_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    return String::New(info.Env(), track_->id());
}

Napi::Value NapiMediaStreamTrack::GetKind(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!track_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    return String::New(info.Env(), track_->kind());
}

Napi::Value NapiMediaStreamTrack::GetEnabled(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!track_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    return Boolean::New(info.Env(), track_->enabled());
}

void NapiMediaStreamTrack::SetEnabled(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!value.IsBoolean()) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Invalid argument"));
    }

    if (!track_) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Illegal state"));
    }

    track_->set_enabled(value.As<Boolean>());
}

Napi::Value NapiMediaStreamTrack::GetReadyState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!track_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    auto state = track_->state();
    switch (state) {
        case MediaStreamTrackInterface::kLive:
            return String::New(info.Env(), kEnumMediaStreamTrackStateLive);
        case MediaStreamTrackInterface::kEnded:
            return String::New(info.Env(), kEnumMediaStreamTrackStateEnded);
        default:
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid state"), info.Env().Undefined());
}

Napi::Value NapiMediaStreamTrack::Stop(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!track_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    auto state = track_->state();
    if (state == MediaStreamTrackInterface::kEnded) {
        RTC_LOG(LS_VERBOSE) << "The track is already ended";
        return info.Env().Undefined();
    }

    RemoveAllVideoSinks();

    return info.Env().Undefined();
}

Napi::Value NapiMediaStreamTrack::GetSource(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!track_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    if (IsAudioTrack()) {
        auto source = factory_->GetAudioSource(track_);
        if (source) {
            return NapiAudioSource::NewInstance(info.Env(), source);
        }
    } else if (IsVideoTrack()) {
        auto source = factory_->GetVideoSource(track_);
        if (source) {
            return NapiVideoSource::NewInstance(info.Env(), source);
        }
    }

    return info.Env().Null();
}

Napi::Value NapiMediaStreamTrack::ToJson(const Napi::CallbackInfo& info)
{
    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiMediaStreamTrack");
#endif
    if (track_) {
        json.Set(kAttributeNameId, GetId(info));
        json.Set(kAttributeNameKind, GetKind(info));
        json.Set(kAttributeNameEnabled, GetEnabled(info));
        json.Set(kAttributeNameReadyState, GetReadyState(info));
    }

    return json;
}

} // namespace webrtc
