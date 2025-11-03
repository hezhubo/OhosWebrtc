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

#include "native_video_renderer.h"
#include "media_source.h"
#include "media_stream.h"
#include "media_stream_track.h"
#include "render/native_window_renderer_gl.h"
#include "render/native_window_renderer_raster.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace ohos;
using namespace adapter;
using namespace Napi;

FunctionReference NapiNativeVideoRenderer::constructor_;

void NapiNativeVideoRenderer::Init(Napi::Env env, Napi::Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiNativeVideoRenderer::GetSurfaceId>(kAttributeNameSurfaceId),
            InstanceAccessor<&NapiNativeVideoRenderer::GetSharedContext>(kAttributeNameSharedContext),
            InstanceAccessor<&NapiNativeVideoRenderer::GetVideoTrack>(kAttributeNameVideoTrack),
            InstanceMethod<&NapiNativeVideoRenderer::SetVideoTrack>(kMethodNameSetVideoTrack),
            InstanceMethod<&NapiNativeVideoRenderer::SetMirror>(kMethodNameSetMirror),
            InstanceMethod<&NapiNativeVideoRenderer::SetMirrorVertically>(kMethodNameSetMirrorVertically),
            InstanceMethod<&NapiNativeVideoRenderer::SetScalingMode>(kMethodNameSetScalingMode),
            InstanceMethod<&NapiNativeVideoRenderer::Init>(kMethodNameInit),
            InstanceMethod<&NapiNativeVideoRenderer::Release>(kMethodNameRelease),
            InstanceMethod<&NapiNativeVideoRenderer::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiNativeVideoRenderer::NapiNativeVideoRenderer(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NapiNativeVideoRenderer>(info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

NapiNativeVideoRenderer::~NapiNativeVideoRenderer()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    RemoveSink();
}

Napi::Value NapiNativeVideoRenderer::GetSurfaceId(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (surfaceId_) {
        return String::New(info.Env(), *surfaceId_);
    }

    return info.Env().Undefined();
}

Napi::Value NapiNativeVideoRenderer::GetSharedContext(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (sharedContext_) {
        return NapiEglContext::NewInstance(info.Env(), sharedContext_);
    }

    return info.Env().Undefined();
}

Napi::Value NapiNativeVideoRenderer::GetVideoTrack(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return jsTrackRef_.IsEmpty() ? info.Env().Null() : jsTrackRef_.Value();
}

Napi::Value NapiNativeVideoRenderer::SetVideoTrack(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() == 0) {
        return info.Env().Undefined();
    }

    if (info[0].IsNull()) {
        RemoveSink();
        jsTrackRef_.Reset();
        return info.Env().Undefined();
    }

    if (!info[0].IsObject()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), Value());
    }

    auto jsTrack = info[0].As<Object>();
    auto napiTrack = NapiMediaStreamTrack::Unwrap(jsTrack);
    if (!napiTrack) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), Value());
    }

    if (!napiTrack->IsVideoTrack()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), Value());
    }

    RemoveSink();
    jsTrackRef_ = Weak(jsTrack);
    AddSink();

    return info.Env().Undefined();
}

Napi::Value NapiNativeVideoRenderer::Init(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsString()) {
        NAPI_THROW(Error::New(info.Env(), "The first argument is not string"), info.Env().Undefined());
    }

    surfaceId_ = info[0].As<String>().Utf8Value();

    if (info.Length() > 1 && info[1].IsObject()) {
        auto jsSharedContext = info[1].As<Object>();
        auto napiSharedContext = NapiEglContext::Unwrap(jsSharedContext);
        if (napiSharedContext) {
            sharedContext_ = napiSharedContext->Get();
        }
    } else {
        sharedContext_ = EglEnv::GetDefault().GetContext();
    }

    auto nativeWindow = ohos::NativeWindow::CreateFromSurfaceId(std::stoull(*surfaceId_));
    if (nativeWindow.IsEmpty()) {
        NAPI_THROW(Error::New(info.Env(), "Failed to create native window"), info.Env().Undefined());
    }

    renderer_ = NativeWindowRendererGl::Create(nativeWindow, sharedContext_, "native-window-renderer");

    return info.Env().Undefined();
}

Napi::Value NapiNativeVideoRenderer::Release(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RemoveSink();

    surfaceId_.reset();
    renderer_.reset();
    jsTrackRef_.Reset();

    return info.Env().Undefined();
}

Napi::Value NapiNativeVideoRenderer::SetMirror(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsBoolean()) {
        NAPI_THROW(Error::New(info.Env(), "The first argument is not boolean"), info.Env().Undefined());
    }

    if (renderer_) {
        renderer_->SetMirrorHorizontally(info[0].As<Boolean>().Value());
    }

    return info.Env().Undefined();
}

Napi::Value NapiNativeVideoRenderer::SetMirrorVertically(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsBoolean()) {
        NAPI_THROW(Error::New(info.Env(), "The first argument is not boolean"), info.Env().Undefined());
    }

    if (renderer_) {
        renderer_->SetMirrorVertically(info[0].As<Boolean>().Value());
    }

    return info.Env().Undefined();
}

Napi::Value NapiNativeVideoRenderer::SetScalingMode(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsNumber()) {
        NAPI_THROW(Error::New(info.Env(), "The first argument is not number"), info.Env().Undefined());
    }

    auto mode = info[0].As<Number>().Int32Value();
    if (renderer_) {
        renderer_->SetScalingMode(static_cast<NativeWindowRenderer::ScalingMode>(mode));
    }

    return info.Env().Undefined();
}

Napi::Value NapiNativeVideoRenderer::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiNativeVideoRenderer"));
#endif

    return json;
}

void NapiNativeVideoRenderer::AddSink()
{
    if (!renderer_) {
        RTC_DLOG(LS_VERBOSE) << "renderer is null";
        return;
    }

    if (jsTrackRef_.IsEmpty()) {
        RTC_DLOG(LS_VERBOSE) << "track ref is empty";
        return;
    }

    auto jsTrack = jsTrackRef_.Value();
    if (jsTrack.IsEmpty()) {
        RTC_DLOG(LS_VERBOSE) << "track is empty";
        return;
    }

    auto napiTrack = NapiMediaStreamTrack::Unwrap(jsTrack);
    napiTrack->AddSink(renderer_.get());
}

void NapiNativeVideoRenderer::RemoveSink()
{
    if (!renderer_) {
        RTC_DLOG(LS_VERBOSE) << "renderer is null";
        return;
    }

    if (jsTrackRef_.IsEmpty()) {
        RTC_DLOG(LS_VERBOSE) << "track ref is empty";
        return;
    }

    auto jsTrack = jsTrackRef_.Value();
    if (jsTrack.IsEmpty()) {
        RTC_DLOG(LS_VERBOSE) << "track is empty";
        return;
    }

    auto napiTrack = NapiMediaStreamTrack::Unwrap(jsTrack);
    napiTrack->RemoveSink(renderer_.get());
}

} // namespace webrtc
