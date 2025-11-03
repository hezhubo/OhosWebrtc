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

#ifndef WEBRTC_MEDIA_STREAM_H
#define WEBRTC_MEDIA_STREAM_H

#include "napi.h"

#include "api/media_stream_interface.h"
#include "pc/media_stream_observer.h"

namespace webrtc {

class NapiMediaStreamTrack;
class PeerConnectionFactoryWrapper;

class NapiMediaStream : public Napi::ObjectWrap<NapiMediaStream> {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object
    NewInstance(std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<MediaStreamInterface> stream);

    ~NapiMediaStream() override;

    rtc::scoped_refptr<MediaStreamInterface> Get() const
    {
        return stream_;
    }

protected:
    friend class ObjectWrap;

    explicit NapiMediaStream(const Napi::CallbackInfo& info);

    Napi::Value GetId(const Napi::CallbackInfo& info);
    Napi::Value GetActive(const Napi::CallbackInfo& info);

    Napi::Value AddTrack(const Napi::CallbackInfo& info);
    Napi::Value RemoveTrack(const Napi::CallbackInfo& info);
    Napi::Value GetTrackById(const Napi::CallbackInfo& info);
    Napi::Value GetTracks(const Napi::CallbackInfo& info);
    Napi::Value GetAudioTracks(const Napi::CallbackInfo& info);
    Napi::Value GetVideoTracks(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

    bool CreateMediaStream();
    bool CreateMediaStream(const NapiMediaStream* stream);
    bool CreateMediaStream(const std::vector<NapiMediaStreamTrack*>& tracks);
    void SetupObserver();

protected:
    static Napi::FunctionReference& Constructor();

    void OnAudioTrackAddedToStream(AudioTrackInterface* track, MediaStreamInterface* stream);
    void OnVideoTrackAddedToStream(VideoTrackInterface* track, MediaStreamInterface* stream);
    void OnAudioTrackRemovedFromStream(AudioTrackInterface* track, MediaStreamInterface* stream);
    void OnVideoTrackRemovedFromStream(VideoTrackInterface* track, MediaStreamInterface* stream);

private:
    std::shared_ptr<PeerConnectionFactoryWrapper> factory_;
    rtc::scoped_refptr<MediaStreamInterface> stream_;
    std::unique_ptr<MediaStreamObserver> observer_;
};

} // namespace webrtc

#endif // WEBRTC_MEDIA_STREAM_H
