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

#ifndef WEBRTC_NATIVE_VIDEO_RENDERER_H
#define WEBRTC_NATIVE_VIDEO_RENDERER_H

#include "native_window_renderer.h"
#include "../media_stream_track.h"
#include "../render/egl_env.h"
#include "../utils/marcos.h"

#include <memory>
#include <optional>

#include "napi.h"

namespace webrtc {

class NapiNativeVideoRenderer : public Napi::ObjectWrap<NapiNativeVideoRenderer> {
public:
    NAPI_CLASS_NAME_DECLARE(NativeVideoRenderer);
    NAPI_ATTRIBUTE_NAME_DECLARE(SurfaceId, surfaceId);
    NAPI_ATTRIBUTE_NAME_DECLARE(VideoTrack, videoTrack);
    NAPI_ATTRIBUTE_NAME_DECLARE(SharedContext, sharedContext);
    NAPI_METHOD_NAME_DECLARE(Init, init);
    NAPI_METHOD_NAME_DECLARE(SetVideoTrack, setVideoTrack);
    NAPI_METHOD_NAME_DECLARE(SetMirror, setMirror);
    NAPI_METHOD_NAME_DECLARE(SetMirrorVertically, setMirrorVertically);
    NAPI_METHOD_NAME_DECLARE(SetScalingMode, setScalingMode);
    NAPI_METHOD_NAME_DECLARE(Release, release);
    NAPI_METHOD_NAME_DECLARE(ToJson, toJSON);

    static void Init(Napi::Env env, Napi::Object exports);

protected:
    friend class ObjectWrap;
    explicit NapiNativeVideoRenderer(const Napi::CallbackInfo& info);
    ~NapiNativeVideoRenderer() override;

    Napi::Value GetSurfaceId(const Napi::CallbackInfo& info);
    Napi::Value GetSharedContext(const Napi::CallbackInfo& info);
    Napi::Value GetVideoTrack(const Napi::CallbackInfo& info);
    Napi::Value SetVideoTrack(const Napi::CallbackInfo& info);
    Napi::Value Init(const Napi::CallbackInfo& info);
    Napi::Value Release(const Napi::CallbackInfo& info);
    Napi::Value SetMirror(const Napi::CallbackInfo& info);
    Napi::Value SetMirrorVertically(const Napi::CallbackInfo& info);
    Napi::Value SetScalingMode(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

    void AddSink();
    void RemoveSink();

private:
    static Napi::FunctionReference constructor_;

    std::optional<std::string> surfaceId_;
    std::shared_ptr<EglContext> sharedContext_;

    // weak reference
    Napi::ObjectReference jsTrackRef_;

    std::unique_ptr<adapter::NativeWindowRenderer> renderer_;
};

} // namespace webrtc

#endif // WEBRTC_NATIVE_VIDEO_RENDERER_H
