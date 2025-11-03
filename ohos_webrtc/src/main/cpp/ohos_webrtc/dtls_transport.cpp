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

#include "dtls_transport.h"
#include "ice_transport.h"
#include "peer_connection.h"
#include "peer_connection_factory.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

const char kEnumDtlsTransportStateNew[] = "new";
const char kEnumDtlsTransportStateConnecting[] = "connecting";
const char kEnumDtlsTransportStateConnected[] = "connected";
const char kEnumDtlsTransportStateClosed[] = "closed";
const char kEnumDtlsTransportStateFailed[] = "failed";

const char kClassName[] = "RTCDtlsTransport";

const char kAttributeNameIceTransport[] = "iceTransport";
const char kAttributeNameState[] = "state";
const char kAttributeNameOnStateChange[] = "onstatechange";
const char kAttributeNameOnError[] = "onerror";

const char kMethodNameGetRemoteCertificates[] = "getRemoteCertificates";
const char kMethodNameToJson[] = "toJSON";

const char kEventNameStateChange[] = "statechange";
const char kEventNameError[] = "error";

FunctionReference NapiDtlsTransport::constructor_;
static constexpr int callbackInfoLen = 2;

void NapiDtlsTransport::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiDtlsTransport::GetIceTransport>(kAttributeNameIceTransport),
            InstanceAccessor<&NapiDtlsTransport::GetState>(kAttributeNameState),
            InstanceAccessor<&NapiDtlsTransport::GetEventHandler, &NapiDtlsTransport::SetEventHandler>(
                kAttributeNameOnStateChange, napi_default, (void*)kEventNameStateChange),
            InstanceAccessor<&NapiDtlsTransport::GetEventHandler, &NapiDtlsTransport::SetEventHandler>(
                kAttributeNameOnError, napi_default, (void*)kEventNameError),
            InstanceMethod<&NapiDtlsTransport::GetRemoteCertificates>(kMethodNameGetRemoteCertificates),
            InstanceMethod<&NapiDtlsTransport::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiDtlsTransport::~NapiDtlsTransport()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
}

Napi::Object NapiDtlsTransport::NewInstance(
    Napi::Env env, std::shared_ptr<PeerConnectionFactoryWrapper> factory,
    rtc::scoped_refptr<DtlsTransportInterface> dtlsTransport)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!dtlsTransport || !factory) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object::New(env));
    }

    auto externalFactory = External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory);
    auto externalTransport = External<DtlsTransportInterface>::New(
        env, dtlsTransport.release(), [](Napi::Env env, DtlsTransportInterface* transport) { transport->Release(); });
    return constructor_.New({externalFactory, externalTransport});
}

NapiDtlsTransport::NapiDtlsTransport(const Napi::CallbackInfo& info) : NapiEventTarget<NapiDtlsTransport>(info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() != callbackInfoLen || !info[0].IsExternal() || !info[1].IsExternal()) {
        // You cannot construct an RTCDtlsTransport
        NAPI_THROW(Napi::Error::New(info.Env(), "Invalid Operation"));
        return;
    }

    factory_ = *info[0].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();

    auto dtlsTransport = info[1].As<External<DtlsTransportInterface>>().Data();
    dtlsTransport_ = rtc::scoped_refptr<DtlsTransportInterface>(dtlsTransport);

    factory_->GetNetworkThread()->PostTask([this] { dtlsTransport_->RegisterObserver(this); });
}

Napi::Value NapiDtlsTransport::GetIceTransport(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return NapiIceTransport::NewInstance(info.Env(), factory_, dtlsTransport_->ice_transport());
}

Napi::Value NapiDtlsTransport::GetState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto information = dtlsTransport_->Information();

    switch (information.state()) {
        case DtlsTransportState::kNew:
            return String::New(info.Env(), kEnumDtlsTransportStateNew);
        case DtlsTransportState::kConnecting:
            return String::New(info.Env(), kEnumDtlsTransportStateConnecting);
        case DtlsTransportState::kConnected:
            return String::New(info.Env(), kEnumDtlsTransportStateConnected);
        case DtlsTransportState::kClosed:
            return String::New(info.Env(), kEnumDtlsTransportStateClosed);
        case DtlsTransportState::kFailed:
            return String::New(info.Env(), kEnumDtlsTransportStateFailed);
        default:
            break;
    }
    NAPI_THROW(Error::New(info.Env(), "Invalid state"), info.Env().Undefined());
}

Napi::Value NapiDtlsTransport::GetEventHandler(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    Function func;
    if (!NapiEventTarget<NapiDtlsTransport>::GetEventHandler(type, func)) {
        return info.Env().Null();
    }

    return func;
}

void NapiDtlsTransport::SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    if (value.IsFunction()) {
        auto fn = value.As<Function>();
        NapiEventTarget<NapiDtlsTransport>::SetEventHandler(type, fn);
    } else if (value.IsNull()) {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " value is null";
        RemoveEventHandler(type);
    } else {
        NAPI_THROW_VOID(Error::New(info.Env(), "value is error"));
    }
}

Napi::Value NapiDtlsTransport::GetRemoteCertificates(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto information = dtlsTransport_->Information();

    auto certChain = information.remote_ssl_certificates();
    if (!certChain || certChain->GetSize() == 0) {
        RTC_DLOG(LS_ERROR) << "Certificate chain is empty!";
        return info.Env().Null();
    }

    Napi::ArrayBuffer certArray = Napi::ArrayBuffer::New(info.Env(), certChain->GetSize());

    for (size_t i = 0; i < certChain->GetSize(); i++) {
        const rtc::SSLCertificate& certificate = certChain->Get(i);
        certArray.Set(i, Napi::String::New(info.Env(), certificate.ToPEMString()));
    }

    return certArray;
}

Napi::Value NapiDtlsTransport::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiDtlsTransport");
#endif

    return json;
}

void NapiDtlsTransport::OnStateChange(DtlsTransportInformation info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto dtlsTransportState = info.state();

    Dispatch(CallbackEvent<NapiDtlsTransport>::Create([this, dtlsTransportState](NapiDtlsTransport& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventNameStateChange));
        target.MakeCallback(kEventNameStateChange, {jsEvent});

        if (dtlsTransportState == DtlsTransportState::kClosed) {
            target.Stop();
        }
    }));
}

void NapiDtlsTransport::OnError(RTCError error)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Dispatch(CallbackEvent<NapiDtlsTransport>::Create([this](NapiDtlsTransport& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventNameError));
        target.MakeCallback(kEventNameError, {jsEvent});
    }));
}

} // namespace webrtc
