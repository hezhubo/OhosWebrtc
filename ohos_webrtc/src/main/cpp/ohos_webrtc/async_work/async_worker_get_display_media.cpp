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

#include "async_worker_get_display_media.h"
#include "../peer_connection_factory.h"
#include "../media_stream.h"
#include "../media_stream_track.h"
#include "../video/video_track_source.h"
#include "../user_media/media_constraints_util.h"
#include "../render/egl_env.h"
#include "../audio_device/audio_common.h"
#include "../audio_device/ohos_local_audio_source.h"
#include "../screen_capture/system_audio_receiver.h"
#include "../screen_capture/screen_capturer.h"

#include "rtc_base/logging.h"
#include "rtc_base/helpers.h"

namespace webrtc {

AsyncWorkerGetDisplayMedia* AsyncWorkerGetDisplayMedia::Create(Napi::Env env,
    std::shared_ptr<PeerConnectionFactoryWrapper> factory)
{
    auto asyncWorker = new AsyncWorkerGetDisplayMedia(env, std::move(factory));
    return asyncWorker;
}

AsyncWorkerGetDisplayMedia::AsyncWorkerGetDisplayMedia(
    Napi::Env env, std::shared_ptr<PeerConnectionFactoryWrapper> factory)
    : AsyncWorker(env, "getDisplayMedia"), deferred_(Napi::Promise::Deferred::New(env)), factory_(std::move(factory))
{
}

void AsyncWorkerGetDisplayMedia::Start(
    MediaTrackConstraints audio, MediaTrackConstraints video, MediaTrackConstraints systemAudio)
{
    audioConstraints_ = std::move(audio);
    systemAudioConstraints_ = std::move(systemAudio);
    videoConstraints_ = std::move(video);
    Queue();
}

rtc::scoped_refptr<AudioTrackInterface> AsyncWorkerGetDisplayMedia::CreateAudioTrack(std::string* errorMessage)
{
    cricket::AudioOptions options;
    CopyConstraintsIntoAudioOptions(audioConstraints_, options);

    auto audioSource = factory_->CreateAudioSource(options);
    if (!audioSource) {
        if (errorMessage) {
            *errorMessage = "Failed to create audio source";
        }
        return nullptr;
    }
    auto audioTrack = factory_->CreateAudioTrack(rtc::CreateRandomUuid(), audioSource);
    if (!audioTrack) {
        if (errorMessage) {
            *errorMessage = "Failed to create audio track";
        }
        return nullptr;
    }

    return audioTrack;
}

rtc::scoped_refptr<AudioTrackInterface> AsyncWorkerGetDisplayMedia::CreateSystemAudioTrack(
    std::shared_ptr<SystemAudioReceiver> systemAudioReceiver, std::string* errorMessage)
{
    cricket::AudioOptions options;
    CopyConstraintsIntoAudioOptions(systemAudioConstraints_, options);

    auto audioSource = factory_->CreateAudioSource(options, std::move(systemAudioReceiver));
    if (!audioSource) {
        if (errorMessage) {
            *errorMessage = "Failed to create system audio source";
        }
        return nullptr;
    }

    auto audioTrack = factory_->CreateAudioTrack(rtc::CreateRandomUuid(), audioSource);
    if (!audioTrack) {
        if (errorMessage) {
            *errorMessage = "Failed to create system audio track";
        }
        return nullptr;
    }

    return audioTrack;
}

rtc::scoped_refptr<VideoTrackInterface> AsyncWorkerGetDisplayMedia::CreateVideoTrack(
    std::shared_ptr<SystemAudioReceiver> systemAudioReceiver, std::string* errorMessage)
{
    ScreenCaptureOptions options;
    GetScreenCaptureOptionsFromConstraints(videoConstraints_, options);
    RTC_DLOG(LS_INFO) << "Screen capture options: " << options.ToString();

    auto screenCapturer = ScreenCapturer::Create(options, std::move(systemAudioReceiver));
    if (!screenCapturer) {
        if (errorMessage) {
            *errorMessage = "Failed to create screen capturer";
        }
        return nullptr;
    }

    auto videoSource = factory_->CreateVideoSource(std::move(screenCapturer));
    if (!videoSource) {
        if (errorMessage) {
            *errorMessage = "Failed to create video source";
        }
        return nullptr;
    }

    auto videoTrack = factory_->CreateVideoTrack(rtc::CreateRandomUuid(), videoSource);
    if (!videoTrack) {
        if (errorMessage) {
            *errorMessage = "Failed to create video track";
        }
        return nullptr;
    }

    return videoTrack;
}

void AsyncWorkerGetDisplayMedia::Execute()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!factory_) {
        SetError("Internal error");
        return;
    }

    stream_ = factory_->GetFactory()->CreateLocalMediaStream(rtc::CreateRandomUuid());
    if (!stream_) {
        SetError("Failed to create media stream");
        return;
    }

    if (!audioConstraints_.IsNull()) {
        if (videoConstraints_.IsNull()) {
            // Video should be enabled, or use <getUserMedia> instead.
            SetError("Audio should not be enabled individually");
            return;
        }

        std::string errorMessage;
        auto audioTrack = CreateAudioTrack(&errorMessage);
        if (audioTrack) {
            stream_->AddTrack(audioTrack);
        } else {
            SetError(errorMessage);
            return;
        }
    }

    std::shared_ptr<SystemAudioReceiver> systemAudioReceiver;
    if (!systemAudioConstraints_.IsNull()) {
        if (videoConstraints_.IsNull()) {
            SetError("System audio should not be enabled individually");
            return;
        }

        // Use default options
        systemAudioReceiver = SystemAudioReceiver::Create();
        std::string errorMessage;
        auto audioTrack = CreateSystemAudioTrack(systemAudioReceiver, &errorMessage);
        if (audioTrack) {
            stream_->AddTrack(audioTrack);
        } else {
            SetError(errorMessage);
            return;
        }
    }

    if (!videoConstraints_.IsNull()) {
        std::string errorMessage;
        auto videoTrack = CreateVideoTrack(systemAudioReceiver, &errorMessage);
        if (videoTrack) {
            stream_->AddTrack(videoTrack);
        } else {
            SetError(errorMessage);
        }
    }
}

void AsyncWorkerGetDisplayMedia::OnOK()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto mediaStream = NapiMediaStream::NewInstance(factory_, stream_);
    if (Env().IsExceptionPending()) {
        deferred_.Reject(Env().GetAndClearPendingException().Value());
        return;
    }

    deferred_.Resolve(mediaStream);
}

void AsyncWorkerGetDisplayMedia::OnError(const Napi::Error& e)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    deferred_.Reject(e.Value());
}

} // namespace webrtc
