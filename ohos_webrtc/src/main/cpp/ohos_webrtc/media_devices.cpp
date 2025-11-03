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

#include "media_devices.h"
#include "media_track_constraints.h"
#include "peer_connection_factory.h"
#include "async_work/async_worker_enumerate_devices.h"
#include "async_work/async_worker_get_user_media.h"
#include "async_work/async_worker_get_display_media.h"
#include "utils/marcos.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

FunctionReference NapiMediaDevices::constructor_;

void NapiMediaDevices::Init(Napi::Env env, Napi::Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            InstanceMethod<&NapiMediaDevices::EnumerateDevices>(kMethodNameEnumerateDevices),
            InstanceMethod<&NapiMediaDevices::GetSupportedConstraints>(kMethodNameGetSupportedConstraints),
            InstanceMethod<&NapiMediaDevices::GetUserMedia>(kMethodNameGetUserMedia),
            InstanceMethod<&NapiMediaDevices::GetDisplayMedia>(kMethodNameGetDisplayMedia),
            InstanceMethod<&NapiMediaDevices::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiMediaDevices::NapiMediaDevices(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NapiMediaDevices>(info) {}

Napi::Value NapiMediaDevices::EnumerateDevices(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto asyncWorker = AsyncWorkerEnumerateDevices::Create(info.Env(), "enumerateDevices");
    asyncWorker->Queue();

    return asyncWorker->GetPromise();
}

Napi::Value NapiMediaDevices::GetSupportedConstraints(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto result = Object::New(info.Env());

#define NAPI_SET_ATTRIBUTE(name)                                                                                       \
    result.Set(name, Boolean::New(info.Env(), NapiMediaConstraints::IsConstraintSupported(name)))

    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameWidth);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameHeight);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameAspectRatio);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameFrameRate);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameFacingMode);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameResizeMode);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameSampleRate);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameSampleSize);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameEchoCancellation);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameAutoGainControl);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameNoiseSuppression);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameLatency);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameChannelCount);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameDeviceId);
    NAPI_SET_ATTRIBUTE(NapiMediaConstraints::kAttributeNameGroupId);

#undef NAPI_SET_ATTRIBUTE

    return result;
}

Napi::Value NapiMediaDevices::GetUserMedia(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto factory = PeerConnectionFactoryWrapper::GetDefault();
    if (!factory || !factory->GetFactory()) {
        NAPI_THROW(Error::New(info.Env(), "No default peer connection factory"), info.Env().Undefined());
    }

    auto asyncWorker = AsyncWorkerGetUserMedia::Create(info.Env(), factory);
    auto deferred = asyncWorker->GetDeferred();

    if (info.Length() == 0 || !info[0].IsObject()) {
        deferred.Reject(TypeError::New(info.Env(), "Invalid argument").Value());
        return asyncWorker->GetPromise();
    }

    MediaTrackConstraints audio;
    MediaTrackConstraints video;

    auto jsConstraints = info[0].As<Object>();
    if (jsConstraints.Has("audio")) {
        NapiMediaConstraints::JsToNative(jsConstraints.Get("audio"), audio);
        if (info.Env().IsExceptionPending()) {
            deferred.Reject(info.Env().GetAndClearPendingException().Value());
            return asyncWorker->GetPromise();
        }
    }

    if (jsConstraints.Has("video")) {
        NapiMediaConstraints::JsToNative(jsConstraints.Get("video"), video);
        if (info.Env().IsExceptionPending()) {
            deferred.Reject(info.Env().GetAndClearPendingException().Value());
            return asyncWorker->GetPromise();
        }
    }

    RTC_DLOG(LS_VERBOSE) << "audio constraints: " << audio.ToString();

    if (audio.IsNull() && video.IsNull()) {
        deferred.Reject(TypeError::New(info.Env(), "At least one of audio and video must be requested").Value());
        return asyncWorker->GetPromise();
    }

    asyncWorker->Start(std::move(audio), std::move(video));
    return asyncWorker->GetPromise();
}

Napi::Value NapiMediaDevices::GetDisplayMedia(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto factory = PeerConnectionFactoryWrapper::GetDefault();
    if (!factory || !factory->GetFactory()) {
        NAPI_THROW(Error::New(info.Env(), "No default peer connection factory"), info.Env().Undefined());
    }

    auto asyncWorker = AsyncWorkerGetDisplayMedia::Create(info.Env(), std::move(factory));
    auto deferred = asyncWorker->GetDeferred();

    if (info.Length() == 0 || !info[0].IsObject()) {
        deferred.Reject(TypeError::New(info.Env(), "Invalid argument").Value());
        return asyncWorker->GetPromise();
    }

    MediaTrackConstraints audio;
    MediaTrackConstraints systemAudio;
    MediaTrackConstraints video;

    auto jsConstraints = info[0].As<Object>();
    if (jsConstraints.Has("audio")) {
        NapiMediaConstraints::JsToNative(jsConstraints.Get("audio"), audio);
        if (info.Env().IsExceptionPending()) {
            deferred.Reject(info.Env().GetAndClearPendingException().Value());
            return asyncWorker->GetPromise();
        }
    }

    if (jsConstraints.Has("ohosSystemAudio")) {
        NapiMediaConstraints::JsToNative(jsConstraints.Get("ohosSystemAudio"), systemAudio);
        if (info.Env().IsExceptionPending()) {
            deferred.Reject(info.Env().GetAndClearPendingException().Value());
            return asyncWorker->GetPromise();
        }
    }

    if (jsConstraints.Has("video")) {
        NapiMediaConstraints::JsToNative(jsConstraints.Get("video"), video);
        if (info.Env().IsExceptionPending()) {
            deferred.Reject(info.Env().GetAndClearPendingException().Value());
            return asyncWorker->GetPromise();
        }
    }

    RTC_DLOG(LS_VERBOSE) << "video constraints: " << video.ToString();
    RTC_DLOG(LS_VERBOSE) << "systemAudio constraints: " << systemAudio.ToString();
    RTC_DLOG(LS_VERBOSE) << "audio constraints: " << audio.ToString();

    if (audio.IsNull() && video.IsNull()) {
        deferred.Reject(TypeError::New(info.Env(), "At least one of audio and video must be requested").Value());
        return asyncWorker->GetPromise();
    }

    asyncWorker->Start(std::move(audio), std::move(video), std::move(systemAudio));
    return asyncWorker->GetPromise();
}

Napi::Value NapiMediaDevices::ToJson(const Napi::CallbackInfo& info)
{
    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiMediaDevices");
#endif

    return json;
}

} // namespace webrtc
