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

#include "sctp_transport.h"
#include "dtls_transport.h"
#include "peer_connection.h"
#include "peer_connection_factory.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

const char kEnumRTCSctpTransportStateConnecting[] = "connecting";
const char kEnumRTCSctpTransportStateConnected[] = "connected";
const char kEnumRTCSctpTransportStateClosed[] = "closed";

const char kAttributeNameMaxChannels[] = "maxChannels";
const char kAttributeNameMaxMessageSize[] = "maxMessageSize";
const char kAttributeNameTransport[] = "transport";
const char kAttributeNameState[] = "state";
const char kMethodNameToJson[] = "toJSON";

const char kEventStateChange[] = "statechange";

FunctionReference NapiSctpTransport::constructor_;

void NapiSctpTransport::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, "RTCSctpTransport",
        {
            InstanceAccessor<&NapiSctpTransport::GetEventHandler, &NapiSctpTransport::SetEventHandler>(
                "onstatechange", napi_default, (void*)kEventStateChange),
            InstanceAccessor<&NapiSctpTransport::GetState>(kAttributeNameState),
            InstanceAccessor<&NapiSctpTransport::GetTransport>(kAttributeNameTransport),
            InstanceAccessor<&NapiSctpTransport::GetMaxChannels>(kAttributeNameMaxChannels),
            InstanceAccessor<&NapiSctpTransport::GetMaxMessageSize>(kAttributeNameMaxMessageSize),
            InstanceMethod<&NapiSctpTransport::ToJson>(kMethodNameToJson),
        });
    exports.Set("RTCSctpTransport", func);

    constructor_ = Persistent(func);
}

Napi::Object NapiSctpTransport::NewInstance(
    std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<SctpTransportInterface> transport)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto env = constructor_.Env();
    if (!factory || !transport) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object());
    }

    return constructor_.New(
        {External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory),
         External<rtc::scoped_refptr<SctpTransportInterface>>::New(env, &transport)});
}

NapiSctpTransport::NapiSctpTransport(const Napi::CallbackInfo& info) : NapiEventTarget<NapiSctpTransport>(info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    // Create from native with 2 paramters, and SHOULD NOT create from ArkTS
    if (info.Length() != 2 || !info[0].IsExternal() || !info[1].IsExternal()) {
        NAPI_THROW_VOID(Napi::Error::New(info.Env(), "Invalid Operation"));
    }

    factory_ = *info[0].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();
    sctpTransport_ = *info[1].As<External<rtc::scoped_refptr<SctpTransportInterface>>>().Data();

    factory_->GetNetworkThread()->BlockingCall([this] { sctpTransport_->RegisterObserver(this); });
}

NapiSctpTransport::~NapiSctpTransport()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    factory_->GetNetworkThread()->BlockingCall([this] { sctpTransport_->UnregisterObserver(); });
}

rtc::scoped_refptr<SctpTransportInterface> NapiSctpTransport::Get() const
{
    return sctpTransport_;
}

Napi::Value NapiSctpTransport::GetMaxChannels(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    auto maxChannels = sctpTransport_->Information().MaxChannels();
    if (maxChannels.has_value()) {
        return Number::New(info.Env(), maxChannels.value());
    }
    return info.Env().Undefined();
}

Napi::Value NapiSctpTransport::GetMaxMessageSize(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    auto maxMessageSize = sctpTransport_->Information().MaxMessageSize();
    if (maxMessageSize.has_value()) {
        return Number::New(info.Env(), maxMessageSize.value());
    }
    return info.Env().Undefined();
}

Napi::Value NapiSctpTransport::GetState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    switch (sctpTransport_->Information().state()) {
        case SctpTransportState::kConnecting:
            return String::New(info.Env(), kEnumRTCSctpTransportStateConnecting);
        case SctpTransportState::kConnected:
            return String::New(info.Env(), kEnumRTCSctpTransportStateConnected);
        case SctpTransportState::kClosed:
            return String::New(info.Env(), kEnumRTCSctpTransportStateClosed);
        default:
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid state"), info.Env().Undefined());
}

Napi::Value NapiSctpTransport::GetTransport(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto transport = sctpTransport_->Information().dtls_transport();
    if (!transport) {
        NAPI_THROW(Error::New(info.Env(), "No transport"), info.Env().Undefined());
    }

    return NapiDtlsTransport::NewInstance(info.Env(), factory_, transport);
}

Napi::Value NapiSctpTransport::GetEventHandler(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    Function func;
    if (!NapiEventTarget<NapiSctpTransport>::GetEventHandler(type, func)) {
        return info.Env().Null();
    }

    return func;
}

void NapiSctpTransport::SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    if (value.IsFunction()) {
        auto fn = value.As<Function>();
        NapiEventTarget<NapiSctpTransport>::SetEventHandler(type, fn);
    } else if (value.IsNull()) {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " value is null";
        RemoveEventHandler(type);
    } else {
        NAPI_THROW_VOID(Error::New(info.Env(), "value is error"));
    }
}

void NapiSctpTransport::OnStateChange(SctpTransportInformation info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto sctpTransportState = info.state();

    Dispatch(CallbackEvent<NapiSctpTransport>::Create([this, sctpTransportState](NapiSctpTransport& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventStateChange));
        target.MakeCallback(kEventStateChange, {jsEvent});

        if (sctpTransportState == SctpTransportState::kClosed) {
            target.Stop();
        }
    }));
}

Napi::Value NapiSctpTransport::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiSctpTransport"));
#endif

    return json;
}

} // namespace webrtc
