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

#ifndef WEBRTC_ASYNC_WORKER_GET_USER_MEDIA_H
#define WEBRTC_ASYNC_WORKER_GET_USER_MEDIA_H

#include "../user_media/media_constraints.h"

#include "napi.h"

#include "api/media_stream_interface.h"

namespace webrtc {

class PeerConnectionFactoryWrapper;

class AsyncWorkerGetUserMedia : public Napi::AsyncWorker {
public:
    static AsyncWorkerGetUserMedia* Create(Napi::Env env, std::shared_ptr<PeerConnectionFactoryWrapper> factory);

    Napi::Promise GetPromise()
    {
        return deferred_.Promise();
    }

    Napi::Promise::Deferred GetDeferred()
    {
        return deferred_;
    }

    void Start(MediaTrackConstraints audio, MediaTrackConstraints video);

protected:
    AsyncWorkerGetUserMedia(Napi::Env env, std::shared_ptr<PeerConnectionFactoryWrapper> factory);

    rtc::scoped_refptr<AudioTrackInterface> CreateAudioTrack(std::string* errorMessage = nullptr);
    rtc::scoped_refptr<VideoTrackInterface> CreateVideoTrack(std::string* errorMessage = nullptr);

    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;

private:
    Napi::Promise::Deferred deferred_;

    std::shared_ptr<PeerConnectionFactoryWrapper> factory_;

    MediaTrackConstraints audioConstraints_;
    MediaTrackConstraints videoConstraints_;
    rtc::scoped_refptr<MediaStreamInterface> stream_;
};

} // namespace webrtc

#endif // WEBRTC_ASYNC_WORKER_GET_USER_MEDIA_H
