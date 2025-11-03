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

#include "media_stream.h"
#include "media_stream_track.h"
#include "peer_connection_factory.h"

#include "api/rtc_error.h"
#include "rtc_base/logging.h"
#include "rtc_base/helpers.h"

namespace webrtc {

using namespace Napi;

const char kClassName[] = "MediaStream";

const char kAttributeNameId[] = "id";
const char kAttributeNameActive[] = "active";

const char kMethodNameAddTrack[] = "addTrack";
const char kMethodNameRemoveTrack[] = "removeTrack";
const char kMethodNameGetTrackById[] = "getTrackById";
const char kMethodNameGetTracks[] = "getTracks";
const char kMethodNameGetAudioTracks[] = "getAudioTracks";
const char kMethodNameGetVideoTracks[] = "getVideoTracks";
const char kMethodNameToJson[] = "toJSON";

static constexpr int callbackInfoLen = 2;

void NapiMediaStream::Init(Napi::Env env, Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiMediaStream::GetId>(kAttributeNameId),
            InstanceAccessor<&NapiMediaStream::GetActive>(kAttributeNameActive),
            InstanceMethod<&NapiMediaStream::AddTrack>(kMethodNameAddTrack),
            InstanceMethod<&NapiMediaStream::RemoveTrack>(kMethodNameRemoveTrack),
            InstanceMethod<&NapiMediaStream::GetTrackById>(kMethodNameGetTrackById),
            InstanceMethod<&NapiMediaStream::GetTracks>(kMethodNameGetTracks),
            InstanceMethod<&NapiMediaStream::GetAudioTracks>(kMethodNameGetAudioTracks),
            InstanceMethod<&NapiMediaStream::GetVideoTracks>(kMethodNameGetVideoTracks),
            InstanceMethod<&NapiMediaStream::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    Constructor() = Persistent(func);
}

Object NapiMediaStream::NewInstance(
    std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<MediaStreamInterface> stream)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto env = Constructor().Env();
    if (!factory || !stream) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object::New(env));
    }

    return Constructor().New(
        {External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory),
         Napi::External<rtc::scoped_refptr<MediaStreamInterface>>::New(env, &stream)});
}

Napi::FunctionReference& NapiMediaStream::Constructor()
{
    static Napi::FunctionReference constructor;
    return constructor;
}

NapiMediaStream::NapiMediaStream(const CallbackInfo& info) : ObjectWrap<NapiMediaStream>(info)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    auto env = info.Env();
    if (info.Length() == callbackInfoLen && info[0].IsExternal() && info[1].IsExternal()) {
        // Create from native with 2 parameters
        factory_ = *info[0].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();
        stream_ = *info[1].As<External<rtc::scoped_refptr<MediaStreamInterface>>>().Data();
    } else if (info.Length() > 0) {
        // Create from ArkTS
        if (info[0].IsArray()) {
            std::vector<NapiMediaStreamTrack*> napiTracks;

            auto jsTracks = info[0].As<Array>();
            for (uint32_t i = 0; i < jsTracks.Length(); i++) {
                auto napiTrack = NapiMediaStreamTrack::Unwrap(jsTracks.Get(i).As<Object>());
                if (!napiTrack || !napiTrack->Get()) {
                    NAPI_THROW_VOID(Error::New(env, "Invalid argument"));
                }
                napiTracks.push_back(napiTrack);
            }

            if (!CreateMediaStream(napiTracks)) {
                NAPI_THROW_VOID(Error::New(env, "Failed to create media stream"));
            }
        } else if (info[0].IsObject()) {
            auto napiStream = NapiMediaStream::Unwrap(info[0].As<Object>());
            if (!napiStream || !napiStream->Get()) {
                NAPI_THROW_VOID(Error::New(env, "Invalid argument"));
            }

            if (!CreateMediaStream(napiStream)) {
                NAPI_THROW_VOID(Error::New(env, "Failed to create media stream"));
            }
        } else {
            NAPI_THROW_VOID(Error::New(env, "Invalid argument"));
        }
    } else {
        // Create from ArkTS with no parameter
        if (!CreateMediaStream()) {
            NAPI_THROW_VOID(Error::New(env, "Failed to create media stream"));
        }
    }

    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " stream_=" << stream_.get();

    SetupObserver();
}

NapiMediaStream::~NapiMediaStream()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " stream_=" << stream_.get();

    for (auto track : stream_->GetAudioTracks()) {
        factory_->RemoveAudioSource(track);
    }

    for (auto track : stream_->GetVideoTracks()) {
        factory_->RemoveVideoSource(track);
    }
}

Napi::Value NapiMediaStream::GetId(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return String::New(info.Env(), stream_->id());
}

Napi::Value NapiMediaStream::GetActive(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto active = false;
    for (const auto& track : stream_->GetAudioTracks()) {
        if (track->state() != MediaStreamTrackInterface::kEnded) {
            active = true;
        }
    }
    for (const auto& track : stream_->GetVideoTracks()) {
        if (track->state() != MediaStreamTrackInterface::kEnded) {
            active = true;
        }
    }

    return Boolean::New(info.Env(), active);
}

Napi::Value NapiMediaStream::AddTrack(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsObject()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    auto jsTrack = info[0].As<Object>();
    auto nativeTrack = NapiMediaStreamTrack::Unwrap(jsTrack);
    if (!nativeTrack) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    auto track = nativeTrack->Get();
    if (!track) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    bool success = false;
    if (track->kind() == MediaStreamTrackInterface::kAudioKind) {
        auto audioTrack = rtc::scoped_refptr<AudioTrackInterface>(static_cast<AudioTrackInterface*>(track.get()));
        success = stream_->AddTrack(audioTrack);
    } else if (track->kind() == MediaStreamTrackInterface::kVideoKind) {
        auto videoTrack = rtc::scoped_refptr<VideoTrackInterface>(static_cast<VideoTrackInterface*>(track.get()));
        success = stream_->AddTrack(videoTrack);
    } else {
        RTC_LOG(LS_WARNING) << "Unknown type of media stream track: " << track->id();
    }

    if (!success) {
        RTC_LOG(LS_ERROR) << "Failed to add track to media stream";
        NAPI_THROW(Error::New(info.Env(), "Unknown error"), info.Env().Undefined());
    }

    return info.Env().Undefined();
}

Napi::Value NapiMediaStream::RemoveTrack(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsObject()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    auto jsTrack = info[0].As<Object>();
    auto nativeTrack = NapiMediaStreamTrack::Unwrap(jsTrack);
    if (!nativeTrack) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    auto track = nativeTrack->Get();
    if (!track) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    bool success = false;
    if (track->kind() == MediaStreamTrackInterface::kAudioKind) {
        auto audioTrack = rtc::scoped_refptr<AudioTrackInterface>(static_cast<AudioTrackInterface*>(track.get()));
        success = stream_->RemoveTrack(audioTrack);
    } else if (track->kind() == MediaStreamTrackInterface::kVideoKind) {
        auto videoTrack = rtc::scoped_refptr<VideoTrackInterface>(static_cast<VideoTrackInterface*>(track.get()));
        success = stream_->RemoveTrack(videoTrack);
    } else {
        RTC_LOG(LS_WARNING) << "Unknown type of media stream track: " << track->id();
    }

    if (!success) {
        RTC_LOG(LS_ERROR) << "Failed to remove track from media stream";
        NAPI_THROW(Error::New(info.Env(), "Unknown error"), info.Env().Undefined());
    }

    return info.Env().Undefined();
}

Napi::Value NapiMediaStream::GetTrackById(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsString()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    auto trackId = info[0].As<String>().Utf8Value();
    auto audioTrack = stream_->FindAudioTrack(trackId);
    if (audioTrack) {
        return NapiMediaStreamTrack::NewInstance(factory_, audioTrack);
    }

    auto videoTrack = stream_->FindVideoTrack(trackId);
    if (videoTrack) {
        return NapiMediaStreamTrack::NewInstance(factory_, videoTrack);
    }

    RTC_LOG(LS_INFO) << "No track with id: " << trackId;

    return info.Env().Null();
}

Napi::Value NapiMediaStream::GetTracks(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto audioTracks = stream_->GetAudioTracks();
    auto videoTracks = stream_->GetVideoTracks();

    auto result = Array::New(info.Env(), audioTracks.size() + videoTracks.size());
    for (uint32_t i = 0; i < audioTracks.size(); i++) {
        result[i] = NapiMediaStreamTrack::NewInstance(factory_, audioTracks[i]);
    }
    for (uint32_t i = 0; i < videoTracks.size(); i++) {
        result[audioTracks.size() + i] = NapiMediaStreamTrack::NewInstance(factory_, videoTracks[i]);
    }

    return result;
}

Napi::Value NapiMediaStream::GetAudioTracks(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto audioTracks = stream_->GetAudioTracks();

    auto result = Array::New(info.Env(), audioTracks.size());
    for (uint32_t i = 0; i < audioTracks.size(); i++) {
        result[i] = NapiMediaStreamTrack::NewInstance(factory_, audioTracks[i]);
    }

    return result;
}

Napi::Value NapiMediaStream::GetVideoTracks(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto videoTracks = stream_->GetVideoTracks();

    auto result = Array::New(info.Env(), videoTracks.size());
    for (uint32_t i = 0; i < videoTracks.size(); i++) {
        result[i] = NapiMediaStreamTrack::NewInstance(factory_, videoTracks[i]);
    }

    return result;
}

Value NapiMediaStream::ToJson(const CallbackInfo& info)
{
    auto json = Object::New(info.Env());
    json.Set(kAttributeNameId, GetId(info));
    json.Set(kAttributeNameActive, GetActive(info));
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiMediaStream"));
#endif

    return json;
}

bool NapiMediaStream::CreateMediaStream()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    factory_ = PeerConnectionFactoryWrapper::GetDefault();
    if (!factory_ || !factory_->GetFactory()) {
        RTC_LOG(LS_WARNING) << "No default peer connection factory";
        return false;
    }

    stream_ = factory_->GetFactory()->CreateLocalMediaStream(rtc::CreateRandomUuid());
    if (!stream_) {
        RTC_LOG(LS_WARNING) << "Failed to create local media stream";
        return false;
    }

    return true;
}

bool NapiMediaStream::CreateMediaStream(const NapiMediaStream* stream)
{
    if (!CreateMediaStream()) {
        return false;
    }

    for (auto& audioTrack : stream->Get()->GetAudioTracks()) {
        stream_->AddTrack(audioTrack);
    }

    for (auto& videoTrack : stream->Get()->GetVideoTracks()) {
        stream_->AddTrack(videoTrack);
    }

    return true;
}

bool NapiMediaStream::CreateMediaStream(const std::vector<NapiMediaStreamTrack*>& tracks)
{
    if (!CreateMediaStream()) {
        return false;
    }

    for (auto& track : tracks) {
        if (track->Get()->kind() == MediaStreamTrackInterface::kAudioKind) {
            auto audioTrack =
                rtc::scoped_refptr<AudioTrackInterface>(static_cast<AudioTrackInterface*>(track->Get().get()));
            stream_->AddTrack(audioTrack);
        } else if (track->Get()->kind() == MediaStreamTrackInterface::kVideoKind) {
            auto videoTrack =
                rtc::scoped_refptr<VideoTrackInterface>(static_cast<VideoTrackInterface*>(track->Get().get()));
            stream_->AddTrack(videoTrack);
        }
    }

    return true;
}

void NapiMediaStream::SetupObserver()
{
    observer_.reset(new MediaStreamObserver(
        stream_.get(),
        [this](AudioTrackInterface* audio_track, MediaStreamInterface* media_stream) {
            OnAudioTrackAddedToStream(audio_track, media_stream);
        },
        [this](AudioTrackInterface* audio_track, MediaStreamInterface* media_stream) {
            OnAudioTrackRemovedFromStream(audio_track, media_stream);
        },
        [this](VideoTrackInterface* video_track, MediaStreamInterface* media_stream) {
            OnVideoTrackAddedToStream(video_track, media_stream);
        },
        [this](VideoTrackInterface* video_track, MediaStreamInterface* media_stream) {
            OnVideoTrackRemovedFromStream(video_track, media_stream);
        }));
}

void NapiMediaStream::OnAudioTrackAddedToStream(AudioTrackInterface* track, MediaStreamInterface* stream)
{
    (void)stream;
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " track: " << track->id();
}

void NapiMediaStream::OnVideoTrackAddedToStream(VideoTrackInterface* track, MediaStreamInterface* stream)
{
    (void)stream;
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " track: " << track->id();
}

void NapiMediaStream::OnAudioTrackRemovedFromStream(AudioTrackInterface* track, MediaStreamInterface* stream)
{
    (void)stream;
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " track: " << track->id();
}

void NapiMediaStream::OnVideoTrackRemovedFromStream(VideoTrackInterface* track, MediaStreamInterface* stream)
{
    (void)stream;
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " track: " << track->id();
}

} // namespace webrtc
