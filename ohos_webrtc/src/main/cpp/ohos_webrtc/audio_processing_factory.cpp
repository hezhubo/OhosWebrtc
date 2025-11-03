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

#include "audio_processing_factory.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

FunctionReference NapiAudioProcessing::constructor_;

void NapiAudioProcessing::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceMethod<&NapiAudioProcessing::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

Napi::Object NapiAudioProcessing::NewInstance(Napi::Env env, rtc::scoped_refptr<AudioProcessing> audioProcessing)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!audioProcessing) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object());
    }

    auto external = External<AudioProcessing>::New(
        env, audioProcessing.release(), [](Napi::Env /*env*/, AudioProcessing* apm) { apm->Release(); });

    return constructor_.New({external});
}

NapiAudioProcessing::NapiAudioProcessing(const Napi::CallbackInfo& info) : ObjectWrap<NapiAudioProcessing>(info)
{
    if (info.Length() == 0 || !info[0].IsExternal()) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Invalid argument"));
    }

    audioProcessing_ = info[0].As<External<AudioProcessing>>().Data();
}

rtc::scoped_refptr<AudioProcessing> NapiAudioProcessing::Get() const
{
    return audioProcessing_;
}

Napi::Value NapiAudioProcessing::ToJson(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiAudioProcessing");
#endif

    return json;
}

FunctionReference NapiAudioProcessingFactory::constructor_;

void NapiAudioProcessingFactory::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceMethod<&NapiAudioProcessingFactory::Create>(kMethodNameCreate),
            InstanceMethod<&NapiAudioProcessingFactory::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiAudioProcessingFactory::NapiAudioProcessingFactory(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NapiAudioProcessingFactory>(info)
{
}

Napi::Value NapiAudioProcessingFactory::Create(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    AudioProcessing::Config apmConfig;
    if (info.Length() > 0 && info[0].IsObject()) {
        auto jsOptions = info[0].As<Object>();
        (void)jsOptions;
        // nothing to do
    }

    return NapiAudioProcessing::NewInstance(info.Env(), AudioProcessingBuilder().SetConfig(apmConfig).Create());
}

Napi::Value NapiAudioProcessingFactory::ToJson(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiAudioProcessingFactory");
#endif

    return json;
}

} // namespace webrtc
