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

#include "video_encoder_factory.h"
#include "render/egl_env.h"
#include "utils/marcos.h"
#include "video_codec/hardware_video_encoder_factory.h"
#include "video_codec/software_video_encoder_factory.h"
#include "video_codec/default_video_encoder_factory.h"

#include <rtc_base/logging.h>

namespace webrtc {

using namespace Napi;

FunctionReference NapiHardwareVideoEncoderFactory::constructor_;

void NapiHardwareVideoEncoderFactory::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiHardwareVideoEncoderFactory::GetEnableH264HighProfile>(
                kAttributeNameEnableH264HighProfile),
            InstanceAccessor<&NapiHardwareVideoEncoderFactory::GetSharedContext>(kAttributeNameSharedContext),
            InstanceMethod<&NapiHardwareVideoEncoderFactory::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiHardwareVideoEncoderFactory::~NapiHardwareVideoEncoderFactory() = default;

NapiHardwareVideoEncoderFactory::NapiHardwareVideoEncoderFactory(const Napi::CallbackInfo& info) : ObjectWrap(info)
{
    if (info.Length() > 0) {
        if (info[0].IsBoolean()) {
            enableH264HighProfile_ = info[0].As<Boolean>().Value();
        }
    }

    if (info.Length() > 1) {
        if (info[1].IsObject()) {
            sharedContext_ = NapiEglContext::Unwrap(info[1].As<Object>())->Get();
        }
    } else {
        sharedContext_ = EglEnv::GetDefault().GetContext();
    }

    info.This().As<Object>().TypeTag(&kTypeTag);
}

Napi::Value NapiHardwareVideoEncoderFactory::GetEnableH264HighProfile(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return Boolean::New(info.Env(), enableH264HighProfile_);
}

Napi::Value NapiHardwareVideoEncoderFactory::GetSharedContext(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return NapiEglContext::NewInstance(info.Env(), sharedContext_);
}

Napi::Value NapiHardwareVideoEncoderFactory::ToJson(const Napi::CallbackInfo& info)
{
    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiHardwareVideoEncoderFactory");
#endif
    json.Set(kAttributeNameEnableH264HighProfile, Boolean::New(info.Env(), enableH264HighProfile_));
    return json;
}

FunctionReference NapiSoftwareVideoEncoderFactory::constructor_;

void NapiSoftwareVideoEncoderFactory::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceMethod<&NapiSoftwareVideoEncoderFactory::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiSoftwareVideoEncoderFactory::~NapiSoftwareVideoEncoderFactory() = default;

NapiSoftwareVideoEncoderFactory::NapiSoftwareVideoEncoderFactory(const Napi::CallbackInfo& info) : ObjectWrap(info)
{
    info.This().As<Object>().TypeTag(&kTypeTag);
}

Napi::Value NapiSoftwareVideoEncoderFactory::ToJson(const Napi::CallbackInfo& info)
{
    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiSoftwareVideoEncoderFactory");
#endif
    return json;
}

std::unique_ptr<VideoEncoderFactory> CreateVideoEncoderFactory(const Napi::Object& jsVideoEncoderFactory)
{
    if (NAPI_CHECK_TYPE_TAG(jsVideoEncoderFactory, NapiHardwareVideoEncoderFactory)) {
        auto napiFactory = NapiHardwareVideoEncoderFactory::Unwrap(jsVideoEncoderFactory);
        auto sharedContext = napiFactory->GetSharedContext();
        auto enableH264HighProfile = napiFactory->GetEnableH264HighProfile();
        return std::make_unique<adapter::HardwareVideoEncoderFactory>(sharedContext, enableH264HighProfile);
    } else if (NAPI_CHECK_TYPE_TAG(jsVideoEncoderFactory, NapiSoftwareVideoEncoderFactory)) {
        return std::make_unique<adapter::SoftwareVideoEncoderFactory>();
    }

    return nullptr;
}

std::unique_ptr<VideoEncoderFactory> CreateDefaultVideoEncoderFactory()
{
    return std::make_unique<adapter::DefaultVideoEncoderFactory>(EglEnv::GetDefault().GetContext(), false);
}

} // namespace webrtc
