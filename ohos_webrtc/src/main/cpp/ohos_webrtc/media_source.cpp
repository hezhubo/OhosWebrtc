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

#include "media_source.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

FunctionReference NapiAudioSource::constructor_;

void NapiAudioSource::Init(Napi::Env env, Napi::Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiAudioSource::GetState>(kAttributeNameState),
            InstanceMethod<&NapiAudioSource::Release>(kMethodNameRelease),
            InstanceMethod<&NapiAudioSource::SetVolume>(kMethodNameSetVolume),
            InstanceMethod<&NapiAudioSource::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

Napi::Object NapiAudioSource::NewInstance(Napi::Env env, rtc::scoped_refptr<OhosLocalAudioSource> source)
{
    if (!source) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object::New(env));
    }

    auto external = External<OhosLocalAudioSource>::New(
        env, source.release(), [](Napi::Env /*env*/, OhosLocalAudioSource* source) { source->Release(); });

    return constructor_.New({external});
}

NapiAudioSource::NapiAudioSource(const Napi::CallbackInfo& info) : NapiMediaSource<NapiAudioSource>(info)
{
    if (info[0].IsExternal()) {
        auto source = info[0].As<External<OhosLocalAudioSource>>().Data();
        source_ = rtc::scoped_refptr<OhosLocalAudioSource>(source);
    }
}

NapiAudioSource::~NapiAudioSource()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

rtc::scoped_refptr<OhosLocalAudioSource> NapiAudioSource::Get() const
{
    return source_;
}

Napi::Value NapiAudioSource::GetState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!source_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    auto state = source_->state();
    switch (state) {
        case MediaSourceInterface::kInitializing:
            return String::New(info.Env(), kEnumNameSourceStateInitializing);
        case MediaSourceInterface::kLive:
            return String::New(info.Env(), kEnumNameSourceStateLive);
        case MediaSourceInterface::kEnded:
            return String::New(info.Env(), kEnumNameSourceStateEnded);
        case MediaSourceInterface::kMuted:
            return String::New(info.Env(), kEnumNameSourceStateMuted);
        default:
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid state"), info.Env().Undefined());
}

Napi::Value NapiAudioSource::Release(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!source_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    source_ = nullptr;

    return info.Env().Undefined();
}

Napi::Value NapiAudioSource::SetVolume(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsNumber()) {
        NAPI_THROW(TypeError::New(info.Env(), "The argument is not number"), info.Env().Undefined());
    }

    if (!source_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    double volume = info[0].As<Number>().DoubleValue();
    source_->SetVolume(volume);

    return info.Env().Undefined();
}

Value NapiAudioSource::ToJson(const CallbackInfo& info)
{
    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiAudioSource"));
#endif

    return json;
}

FunctionReference NapiVideoSource::constructor_;

void NapiVideoSource::Init(Napi::Env env, Napi::Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiVideoSource::GetState>(kAttributeNameState),
            InstanceAccessor<&NapiVideoSource::GetEventHandler, &NapiVideoSource::SetEventHandler>(
                kAttributeNameOnCapturerStarted, napi_default, (void*)kEventNameCapturerStarted),
            InstanceAccessor<&NapiVideoSource::GetEventHandler, &NapiVideoSource::SetEventHandler>(
                kAttributeNameOnCapturerStopped, napi_default, (void*)kEventNameCapturerStopped),
            InstanceMethod<&NapiVideoSource::Release>(kMethodNameRelease),
            InstanceMethod<&NapiVideoSource::StartCapture>(kMethodNameStartCapture),
            InstanceMethod<&NapiVideoSource::StopCapture>(kMethodNameStopCapture),
            InstanceMethod<&NapiVideoSource::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

Napi::Object NapiVideoSource::NewInstance(Napi::Env env, rtc::scoped_refptr<OhosVideoTrackSource> source)
{
    if (!source) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object());
    }

    auto external = External<OhosVideoTrackSource>::New(
        env, source.release(), [](Napi::Env /*env*/, OhosVideoTrackSource* source) { source->Release(); });

    return constructor_.New({external});
}

NapiVideoSource::NapiVideoSource(const Napi::CallbackInfo& info) : NapiMediaSource<NapiVideoSource>(info)
{
    if (info[0].IsExternal()) {
        auto source = info[0].As<External<OhosVideoTrackSource>>().Data();
        source_ = rtc::scoped_refptr<OhosVideoTrackSource>(source);
    }

    if (source_) {
        source_->SetCapturerObserver(this);
    }
}

NapiVideoSource::~NapiVideoSource()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    source_->SetCapturerObserver(nullptr);
    source_ = nullptr;

    RemoveAllEventHandlers();
}

Napi::Value NapiVideoSource::GetState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!source_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    auto source = static_cast<VideoTrackSourceInterface*>(source_.get());
    auto state = source->state();
    switch (state) {
        case MediaSourceInterface::kInitializing:
            return String::New(info.Env(), kEnumNameSourceStateInitializing);
        case MediaSourceInterface::kLive:
            return String::New(info.Env(), kEnumNameSourceStateLive);
        case MediaSourceInterface::kEnded:
            return String::New(info.Env(), kEnumNameSourceStateEnded);
        case MediaSourceInterface::kMuted:
            return String::New(info.Env(), kEnumNameSourceStateMuted);
        default:
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid state"), info.Env().Undefined());
}

Napi::Value NapiVideoSource::Release(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!source_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    source_->AddRef();

    source_->SetCapturerObserver(nullptr);
    source_ = nullptr;

    RemoveAllEventHandlers();

    return info.Env().Undefined();
}

Napi::Value NapiVideoSource::GetEventHandler(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!source_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    Function fn;
    if (!GetEventHandler((const char*)info.Data(), fn)) {
        return info.Env().Null();
    }

    return fn;
}

void NapiVideoSource::SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!source_) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Illegal state"));
    }

    RemoveEventHandler((const char*)info.Data());

    if (value.IsFunction()) {
        Function cb = value.As<Function>();
        if (!SetEventHandler((const char*)info.Data(), std::move(cb), info.This())) {
            NAPI_THROW_VOID(Napi::Error::New(info.Env(), "Failed to set event handler"));
        }
    }
}

Napi::Value NapiVideoSource::StartCapture(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!source_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    source_->Start();

    return info.Env().Undefined();
}

Napi::Value NapiVideoSource::StopCapture(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!source_) {
        NAPI_THROW(Error::New(info.Env(), "Illegal state"), info.Env().Undefined());
    }

    source_->Stop();

    return info.Env().Undefined();
}

Value NapiVideoSource::ToJson(const CallbackInfo& info)
{
    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiVideoSource"));
#endif

    return json;
}

void NapiVideoSource::OnCapturerStarted(bool success)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    ThreadSafeFunction tsfn;
    if (!GetEventHandler(kEventNameCapturerStarted, tsfn)) {
        return;
    }

    Reference<Napi::Value>* context = tsfn.GetContext();
    napi_status status = tsfn.BlockingCall([context, success](Napi::Env env, Napi::Function jsCallback) {
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, "VideoCapturerStartedEvent"));
        jsEvent.Set("success", Boolean::New(env, success));
        jsCallback.Call(context ? context->Value() : env.Undefined(), {jsEvent});
    });

    if (status != napi_ok) {
        RTC_LOG(LS_ERROR) << "tsfn call error: " << status;
    }
}

void NapiVideoSource::OnCapturerStopped()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    ThreadSafeFunction tsfn;
    if (!GetEventHandler(kEventNameCapturerStopped, tsfn)) {
        return;
    }

    Reference<Napi::Value>* context = tsfn.GetContext();
    napi_status status = tsfn.BlockingCall([context](Napi::Env env, Napi::Function jsCallback) {
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, "Event"));
        jsCallback.Call(context ? context->Value() : env.Undefined(), {jsEvent});
    });

    if (status != napi_ok) {
        RTC_LOG(LS_ERROR) << "tsfn call error: " << status;
    }
}

bool NapiVideoSource::GetEventHandler(const std::string& type, Napi::Function& fn)
{
    RTC_DLOG(LS_VERBOSE) << "NapiPeerConnectionWrapper::GetEventHandler type: " << type;

    UNUSED std::lock_guard<std::mutex> lock(eventMutex_);
    auto it = eventHandlers_.find(type);
    if (it == eventHandlers_.end()) {
        RTC_LOG(LS_VERBOSE) << "No event handler for type: " << type;
        return false;
    }

    fn = it->second.ref.Value();

    return true;
}

bool NapiVideoSource::GetEventHandler(const std::string& type, Napi::ThreadSafeFunction& tsfn)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " type=" << type;

    UNUSED std::lock_guard<std::mutex> lock(eventMutex_);
    auto it = eventHandlers_.find(type);
    if (it == eventHandlers_.end()) {
        RTC_LOG(LS_VERBOSE) << "No event handler for type: " << type;
        return false;
    }

    tsfn = it->second.tsfn;

    return true;
}

bool NapiVideoSource::SetEventHandler(const std::string& type, Napi::Function fn, Napi::Value receiver)
{
    RTC_DLOG(LS_VERBOSE) << "SetEventHandler type: " << type;

    Reference<Napi::Value>* context = new Reference<Napi::Value>;
    *context = Persistent(receiver);

    EventHandler handler;
    handler.ref = Persistent(fn);
    handler.tsfn =
        ThreadSafeFunction::New(fn.Env(), fn, type, 0, 1, context, [](Napi::Env, Reference<Napi::Value>* ctx) {
            ctx->Reset();
            delete ctx;
        });

    UNUSED std::lock_guard<std::mutex> lock(eventMutex_);
    eventHandlers_[type] = std::move(handler);

    return true;
}

bool NapiVideoSource::RemoveEventHandler(const std::string& type)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    UNUSED std::lock_guard<std::mutex> lock(eventMutex_);
    auto it = eventHandlers_.find(type);
    if (it != eventHandlers_.end()) {
        it->second.tsfn.Release();
        eventHandlers_.erase(it);
    }

    return true;
}

void NapiVideoSource::RemoveAllEventHandlers()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    UNUSED std::lock_guard<std::mutex> lock(eventMutex_);
    for (auto& handler : eventHandlers_) {
        handler.second.tsfn.Release();
    }
    eventHandlers_.clear();
}

} // namespace webrtc
