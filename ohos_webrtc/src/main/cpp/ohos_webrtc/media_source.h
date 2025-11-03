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

#ifndef WEBRTC_AUDIO_SOURCE_H
#define WEBRTC_AUDIO_SOURCE_H

#include "napi.h"

#include "api/media_stream_interface.h"

#include "video/video_track_source.h"
#include "audio_device/ohos_local_audio_source.h"
#include "utils/marcos.h"

namespace webrtc {

class OhosLocalAudioSource;

template <typename T>
class NapiMediaSource : public Napi::ObjectWrap<T> {
public:
    NAPI_ATTRIBUTE_NAME_DECLARE(State, state);
    NAPI_METHOD_NAME_DECLARE(Release, release);
    NAPI_METHOD_NAME_DECLARE(ToJson, toJSON);
    NAPI_ENUM_NAME_DECLARE(SourceStateInitializing, initializing);
    NAPI_ENUM_NAME_DECLARE(SourceStateLive, live);
    NAPI_ENUM_NAME_DECLARE(SourceStateEnded, ended);
    NAPI_ENUM_NAME_DECLARE(SourceStateMuted, muted);

    explicit NapiMediaSource(const Napi::CallbackInfo& info) : Napi::ObjectWrap<T>(info) {}
};

class NapiAudioSource : public NapiMediaSource<NapiAudioSource> {
public:
    NAPI_CLASS_NAME_DECLARE(AudioSource);
    NAPI_METHOD_NAME_DECLARE(SetVolume, setVolume);

    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(Napi::Env env, rtc::scoped_refptr<OhosLocalAudioSource> source);

    ~NapiAudioSource() override;

    rtc::scoped_refptr<OhosLocalAudioSource> Get() const;

protected:
    friend class ObjectWrap;

    explicit NapiAudioSource(const Napi::CallbackInfo& info);

    Napi::Value GetState(const Napi::CallbackInfo& info);
    Napi::Value Release(const Napi::CallbackInfo& info);
    Napi::Value SetVolume(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;

    rtc::scoped_refptr<OhosLocalAudioSource> source_;
};

class NapiVideoSource : public NapiMediaSource<NapiVideoSource>, public VideoCapturer::Observer {
public:
    NAPI_CLASS_NAME_DECLARE(VideoSource);
    NAPI_ATTRIBUTE_NAME_DECLARE(OnCapturerStarted, oncapturerstarted);
    NAPI_ATTRIBUTE_NAME_DECLARE(OnCapturerStopped, oncapturerstopped);
    NAPI_EVENT_NAME_DECLARE(CapturerStarted, capturerstarted);
    NAPI_EVENT_NAME_DECLARE(CapturerStopped, capturerstopped);
    NAPI_METHOD_NAME_DECLARE(StartCapture, startCapture);
    NAPI_METHOD_NAME_DECLARE(StopCapture, stopCapture);

    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(Napi::Env env, rtc::scoped_refptr<OhosVideoTrackSource> source);

    ~NapiVideoSource() override;

    rtc::scoped_refptr<OhosVideoTrackSource> Get() const
    {
        return source_;
    }

protected:
    friend class ObjectWrap;

    explicit NapiVideoSource(const Napi::CallbackInfo& info);

    Napi::Value GetState(const Napi::CallbackInfo& info);
    Napi::Value Release(const Napi::CallbackInfo& info);
    Napi::Value GetEventHandler(const Napi::CallbackInfo& info);
    void SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value);
    Napi::Value StartCapture(const Napi::CallbackInfo& info);
    Napi::Value StopCapture(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

protected:
    // VideoCapturer::Observer
    void OnCapturerStarted(bool success) override;
    void OnCapturerStopped() override;
    void
    OnFrameCaptured(rtc::scoped_refptr<VideoFrameBuffer> buffer, int64_t timestampUs, VideoRotation rotation) override
    {
    }

private:
    bool GetEventHandler(const std::string& type, Napi::Function& fn);
    bool GetEventHandler(const std::string& type, Napi::ThreadSafeFunction& tsfn);
    bool SetEventHandler(const std::string& type, Napi::Function fn, Napi::Value receiver);
    bool RemoveEventHandler(const std::string& type);
    void RemoveAllEventHandlers();

private:
    static Napi::FunctionReference constructor_;

    rtc::scoped_refptr<OhosVideoTrackSource> source_;
    rtc::Thread* signalThread_{};

    struct EventHandler {
        Napi::FunctionReference ref;
        Napi::ThreadSafeFunction tsfn;
    };

    mutable std::mutex eventMutex_;
    std::map<std::string, EventHandler> eventHandlers_;
};

} // namespace webrtc

#endif // WEBRTC_AUDIO_SOURCE_H
