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

#include "async_worker_get_user_media.h"
#include "../peer_connection_factory.h"
#include "../media_stream.h"
#include "../media_stream_track.h"
#include "../camera/camera_enumerator.h"
#include "../camera/camera_capturer.h"
#include "../video/video_track_source.h"
#include "../user_media/media_constraints_util.h"
#include "../render/egl_env.h"
#include "../audio_device/ohos_local_audio_source.h"

#include "rtc_base/logging.h"
#include "rtc_base/helpers.h"

namespace webrtc {

AsyncWorkerGetUserMedia* AsyncWorkerGetUserMedia::Create(Napi::Env env,
    std::shared_ptr<PeerConnectionFactoryWrapper> factory)
{
    auto asyncWorker = new AsyncWorkerGetUserMedia(env, std::move(factory));
    return asyncWorker;
}

AsyncWorkerGetUserMedia::AsyncWorkerGetUserMedia(Napi::Env env, std::shared_ptr<PeerConnectionFactoryWrapper> factory)
    : AsyncWorker(env, "getUserMedia"), deferred_(Napi::Promise::Deferred::New(env)), factory_(std::move(factory))
{
}

void AsyncWorkerGetUserMedia::Start(MediaTrackConstraints audio, MediaTrackConstraints video)
{
    audioConstraints_ = std::move(audio);
    videoConstraints_ = std::move(video);
    Queue();
}

rtc::scoped_refptr<AudioTrackInterface> AsyncWorkerGetUserMedia::CreateAudioTrack(std::string* errorMessage)
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

rtc::scoped_refptr<VideoTrackInterface> AsyncWorkerGetUserMedia::CreateVideoTrack(std::string* errorMessage)
{
    auto cameraDevices = CameraEnumerator::GetDevices();

    CameraCaptureSettings selectedSetting;
    std::string failedConstraintName;
    if (SelectSettingsForVideo(cameraDevices, videoConstraints_, kDefaultWidth, kDefaultHeight,
                               kDefaultFrameRate, selectedSetting, failedConstraintName)) {
        RTC_DLOG(LS_INFO) << "Selected camera device: " << selectedSetting.deviceId
                          << ", resolution = " << selectedSetting.profile.resolution.width << "x"
                          << selectedSetting.profile.resolution.height
                          << ", format = " << selectedSetting.profile.format
                          << ", framerate = " << selectedSetting.profile.frameRateRange.min << "-"
                          << selectedSetting.profile.frameRateRange.max;
    } else {
        RTC_LOG(LS_ERROR) << "Failed to select settings for video: " << failedConstraintName;
        if (errorMessage) {
            *errorMessage = std::string("Unsatisfied constraint: ") + failedConstraintName;
        }
    }

    auto capturer = CameraCapturer::Create(selectedSetting.deviceId, selectedSetting.profile);
    if (!capturer) {
        if (errorMessage) {
            *errorMessage = "Failed to create camera capturer";
        }
        return nullptr;
    }

    auto videoSource = factory_->CreateVideoSource(std::move(capturer));
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

void AsyncWorkerGetUserMedia::Execute()
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
        std::string errorMessage;
        auto audioTrack = CreateAudioTrack(&errorMessage);
        if (audioTrack) {
            stream_->AddTrack(audioTrack);
        } else {
            SetError(errorMessage);
            return;
        }
    }

    if (!videoConstraints_.IsNull()) {
        std::string errorMessage;
        auto videoTrack = CreateVideoTrack(&errorMessage);
        if (videoTrack) {
            stream_->AddTrack(videoTrack);
        } else {
            SetError(errorMessage);
            return;
        }
    }
}

void AsyncWorkerGetUserMedia::OnOK()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto mediaStream = NapiMediaStream::NewInstance(factory_, stream_);
    if (Env().IsExceptionPending()) {
        deferred_.Reject(Env().GetAndClearPendingException().Value());
        return;
    }

    deferred_.Resolve(mediaStream);
}

void AsyncWorkerGetUserMedia::OnError(const Napi::Error& e)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    deferred_.Reject(e.Value());
}

} // namespace webrtc
