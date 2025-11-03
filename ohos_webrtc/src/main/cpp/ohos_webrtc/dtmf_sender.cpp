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

#include "dtmf_sender.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

constexpr int32_t kDefaultDurationMs = 100;
constexpr int32_t kMinDurationMs = 40;
constexpr int32_t kMaxDurationMs = 6000;
constexpr int32_t kDefaultInterToneGapMs = 70;
constexpr int32_t kMinInterToneGapMs = 30;
constexpr int32_t kMaxInterToneGapMs = 6000;

const char kClassName[] = "RTCDTMFSender";

const char kAttributeNameCanInsertDTMF[] = "canInsertDTMF";
const char kAttributeNameToneBuffer[] = "toneBuffer";
const char kAttributeNameOnToneChange[] = "ontonechange";

const char kMethodNameInsertDTMF[] = "insertDTMF";
const char kMethodNameToJson[] = "toJSON";

const char kEventNameToneChange[] = "tonechange";

FunctionReference NapiDtmfSender::constructor_;
static constexpr int callbackInfoLen = 2;

void NapiDtmfSender::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiDtmfSender::GetCanInsertDTMF>(kAttributeNameCanInsertDTMF),
            InstanceAccessor<&NapiDtmfSender::GetToneBuffer>(kAttributeNameToneBuffer),
            InstanceAccessor<&NapiDtmfSender::GetEventHandler, &NapiDtmfSender::SetEventHandler>(
                kAttributeNameOnToneChange, napi_default, (void*)kEventNameToneChange),
            InstanceMethod<&NapiDtmfSender::InsertDTMF>(kMethodNameInsertDTMF),
            InstanceMethod<&NapiDtmfSender::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiDtmfSender::~NapiDtmfSender()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
}

Napi::Object NapiDtmfSender::NewInstance(Napi::Env env, rtc::scoped_refptr<DtmfSenderInterface> dtmfSender)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!dtmfSender) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object::New(env));
    }

    auto external = External<DtmfSenderInterface>::New(
        env, dtmfSender.release(), [](Napi::Env /*env*/, DtmfSenderInterface* sender) { sender->Release(); });

    return constructor_.New({external});
}

NapiDtmfSender::NapiDtmfSender(const Napi::CallbackInfo& info) : NapiEventTarget<NapiDtmfSender>(info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    rtc::scoped_refptr<DtmfSenderInterface> dtmfSender;

    if (info.Length() > 0 && info[0].IsExternal()) {
        dtmfSender = info[0].As<External<DtmfSenderInterface>>().Data();
    } else {
        // nothing to do now
    }

    if (!dtmfSender) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Invalid argument"));
    }

    dtmfSender_ = dtmfSender;
    dtmfSender_->RegisterObserver(this);
}

Napi::Value NapiDtmfSender::GetCanInsertDTMF(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    auto result = dtmfSender_->CanInsertDtmf();
    return Napi::Boolean::New(info.Env(), result);
}

Napi::Value NapiDtmfSender::GetToneBuffer(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    auto tones = dtmfSender_->tones();
    return Napi::String::New(info.Env(), tones);
}

Napi::Value NapiDtmfSender::GetEventHandler(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    Function func;
    if (!NapiEventTarget<NapiDtmfSender>::GetEventHandler(type, func)) {
        return info.Env().Null();
    }

    return func;
}

void NapiDtmfSender::SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    if (value.IsFunction()) {
        auto fn = value.As<Function>();
        NapiEventTarget<NapiDtmfSender>::SetEventHandler(type, fn);
    } else if (value.IsNull()) {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " value is null";
        RemoveEventHandler(type);
    } else {
        NAPI_THROW_VOID(Error::New(info.Env(), "value is error"));
    }
}

Napi::Value NapiDtmfSender::InsertDTMF(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() == 0) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsString()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument types"), info.Env().Undefined());
    }

    if (!dtmfSender_->CanInsertDtmf()) {
        NAPI_THROW(Error::New(info.Env(), "InvalidStateError"), info.Env().Undefined());
    }

    const std::string tones = info[0].As<String>();

    int duration = kDefaultDurationMs;
    int interToneGap = kDefaultInterToneGapMs;

    if (info.Length() >= callbackInfoLen) {
        // second parameter is optional
        duration = info[1].As<Number>();
        if (duration > kMaxDurationMs) {
            RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " The value of duration is greater than 6000";
            duration = kMaxDurationMs;
        }
        if (duration < kMinDurationMs) {
            RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " The value of duration is less than 40";
            duration = kMinDurationMs;
        }
    }

    if (info.Length() == 3) {
        // third parameter is optional
        interToneGap = info[2].As<Number>();
        if (interToneGap < kMinInterToneGapMs) {
            RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " The value of interToneGap is less than 30";
            interToneGap = kMinInterToneGapMs;
        }
        if (interToneGap > kMaxInterToneGapMs) {
            RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " The value of interToneGap is greater than 6000";
            interToneGap = kMaxInterToneGapMs;
        }
    }

    bool result = dtmfSender_->InsertDtmf(tones, duration, interToneGap);
    if (result) {
        return Napi::Boolean::New(info.Env(), result);
    } else {
        NAPI_THROW(Error::New(info.Env(), "Failed to insert DTMF"), info.Env().Undefined());
    }
}

Napi::Value NapiDtmfSender::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiDtmfSender");
#endif

    return json;
}

void NapiDtmfSender::OnToneChange(const std::string& tone, const std::string& tone_buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Dispatch(CallbackEvent<NapiDtmfSender>::Create([this, tone](NapiDtmfSender& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("tone", String::New(env, tone));
        target.MakeCallback(kEventNameToneChange, {jsEvent});
    }));
}

} // namespace webrtc
