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

#include "egl_context.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

FunctionReference NapiEglContext::constructor_;

void NapiEglContext::Init(Napi::Env env, Napi::Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            InstanceMethod<&NapiEglContext::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

Napi::Value NapiEglContext::NewInstance(Napi::Env env, std::shared_ptr<EglContext> eglContext)
{
    if (!eglContext) {
        NAPI_THROW(Error::New(env, "Invalid arguments"), env.Undefined());
    }

    auto external = External<EglContext>::New(
        env, eglContext.get(), [eglContext](Napi::Env /*env*/, EglContext* /*ctx*/) { (void)eglContext; });
    return constructor_.New({external});
}

NapiEglContext::NapiEglContext(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NapiEglContext>(info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() == 0) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Wrong number of arguments"));
    }

    auto context = info[0].As<External<EglContext>>().Data();
    if (context) {
        eglContext_ = context->shared_from_this();
    }
}

NapiEglContext::~NapiEglContext() = default;

Napi::Value NapiEglContext::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiEglContext"));
#endif

    return json;
}

} // namespace webrtc
