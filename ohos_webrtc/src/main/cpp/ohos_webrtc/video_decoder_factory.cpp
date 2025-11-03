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

#include "video_decoder_factory.h"
#include "utils/marcos.h"
#include "video_codec/hardware_video_decoder_factory.h"
#include "video_codec/software_video_decoder_factory.h"
#include "video_codec/default_video_decoder_factory.h"

#include <rtc_base/logging.h>

namespace webrtc {

using namespace Napi;

FunctionReference NapiHardwareVideoDecoderFactory::constructor_;

void NapiHardwareVideoDecoderFactory::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceMethod<&NapiHardwareVideoDecoderFactory::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiHardwareVideoDecoderFactory::~NapiHardwareVideoDecoderFactory() = default;

NapiHardwareVideoDecoderFactory::NapiHardwareVideoDecoderFactory(const Napi::CallbackInfo& info) : ObjectWrap(info)
{
    if (info.Length() > 0) {
        if (info[0].IsObject()) {
            sharedContext_ = NapiEglContext::Unwrap(info[0].As<Object>())->Get();
        }
    } else {
        sharedContext_ = EglEnv::GetDefault().GetContext();
    }

    info.This().As<Object>().TypeTag(&kTypeTag);
}

Napi::Value NapiHardwareVideoDecoderFactory::GetSharedContext(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return NapiEglContext::NewInstance(info.Env(), sharedContext_);
}

Napi::Value NapiHardwareVideoDecoderFactory::ToJson(const Napi::CallbackInfo& info)
{
    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiHardwareVideoDecoderFactory");
#endif
    return json;
}

FunctionReference NapiSoftwareVideoDecoderFactory::constructor_;

void NapiSoftwareVideoDecoderFactory::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceMethod<&NapiSoftwareVideoDecoderFactory::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiSoftwareVideoDecoderFactory::~NapiSoftwareVideoDecoderFactory() = default;

NapiSoftwareVideoDecoderFactory::NapiSoftwareVideoDecoderFactory(const Napi::CallbackInfo& info) : ObjectWrap(info)
{
    info.This().As<Object>().TypeTag(&kTypeTag);
}

Napi::Value NapiSoftwareVideoDecoderFactory::ToJson(const Napi::CallbackInfo& info)
{
    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiSoftwareVideoDecoderFactory");
#endif
    return json;
}

std::unique_ptr<VideoDecoderFactory> CreateVideoDecoderFactory(const Napi::Object& jsVideoDecoderFactory)
{
    if (NAPI_CHECK_TYPE_TAG(jsVideoDecoderFactory, NapiHardwareVideoDecoderFactory)) {
        auto napiFactory = NapiHardwareVideoDecoderFactory::Unwrap(jsVideoDecoderFactory);
        auto sharedContext = napiFactory->GetSharedContext();
        return std::make_unique<adapter::HardwareVideoDecoderFactory>(sharedContext);
    } else if (NAPI_CHECK_TYPE_TAG(jsVideoDecoderFactory, NapiSoftwareVideoDecoderFactory)) {
        return std::make_unique<adapter::SoftwareVideoDecoderFactory>();
    }

    return nullptr;
}

std::unique_ptr<VideoDecoderFactory> CreateDefaultVideoDecoderFactory()
{
    return std::make_unique<adapter::DefaultVideoDecoderFactory>(EglEnv::GetDefault().GetContext());
}

} // namespace webrtc
