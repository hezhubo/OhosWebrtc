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

#include "ice_transport.h"
#include "peer_connection.h"
#include "peer_connection_factory.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

// https://www.w3.org/TR/webrtc/#dfn-candidate-attribute
// https://www.rfc-editor.org/rfc/rfc5245#section-15.1
constexpr int32_t kComponentIdRtp = 1;
constexpr int32_t kComponentIdRtcp = 2;

const char kEnumRTCIceRoleUnKnown[] = "unknown";
const char kEnumRTCIceRoleControlling[] = "controlling";
const char kEnumRTCIceRoleControlled[] = "controlled";

const char kEnumRTCIceTransportStateChecking[] = "checking";
const char kEnumRTCIceTransportStateClosed[] = "closed";
const char kEnumRTCIceTransportStateCompleted[] = "completed";
const char kEnumRTCIceTransportStateConnected[] = "connected";
const char kEnumRTCIceTransportStateDisconnected[] = "disconnected";
const char kEnumRTCIceTransportStateFailed[] = "failed";
const char kEnumRTCIceTransportStateNew[] = "new";

const char kEnumRTCIceComponentRtp[] = "rtp";
const char kEnumRTCIceComponentRtcp[] = "rtcp";

const char kEnumRTCIceGathererStateComplete[] = "complete";
const char kEnumRTCIceGathererStateGathering[] = "gathering";
const char kEnumRTCIceGathererStateNew[] = "new";

const char kClassName[] = "RTCIceTransport";

const char kAttributeNameRole[] = "role";
const char kAttributeNameComponent[] = "component";
const char kAttributeNameState[] = "state";
const char kAttributeNameGatheringState[] = "gatheringState";
const char kAttributeNameOnStateChange[] = "onstatechange";
const char kAttributeNameOnGatheringStateChange[] = "ongatheringstatechange";
const char kAttributeNameOnSelectedCandidatePairChange[] = "onselectedcandidatepairchange";

const char kEventNameStateChange[] = "statechange";
const char kEventNameGatheringStateChange[] = "gatheringstatechange";
const char kEventNameSelectedCandidatePairChange[] = "selectedcandidatepairchange";

const char kMethodNameGetSelectedCandidatePair[] = "getSelectedCandidatePair";
const char kMethodNameToJson[] = "toJSON";

FunctionReference NapiIceTransport::constructor_;
static constexpr int callbackInfoLen = 2;

void NapiIceTransport::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiIceTransport::GetRole>(kAttributeNameRole),
            InstanceAccessor<&NapiIceTransport::GetComponent>(kAttributeNameComponent),
            InstanceAccessor<&NapiIceTransport::GetState>(kAttributeNameState),
            InstanceAccessor<&NapiIceTransport::GetGatheringState>(kAttributeNameGatheringState),
            InstanceAccessor<&NapiIceTransport::GetEventHandler, &NapiIceTransport::SetEventHandler>(
                kAttributeNameOnStateChange, napi_default, (void*)kEventNameStateChange),
            InstanceAccessor<&NapiIceTransport::GetEventHandler, &NapiIceTransport::SetEventHandler>(
                kAttributeNameOnGatheringStateChange, napi_default, (void*)kEventNameGatheringStateChange),
            InstanceAccessor<&NapiIceTransport::GetEventHandler, &NapiIceTransport::SetEventHandler>(
                kAttributeNameOnSelectedCandidatePairChange, napi_default,
                (void*)kEventNameSelectedCandidatePairChange),
            InstanceMethod<&NapiIceTransport::GetSelectedCandidatePair>(kMethodNameGetSelectedCandidatePair),
            InstanceMethod<&NapiIceTransport::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiIceTransport::~NapiIceTransport()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
}

Napi::Object NapiIceTransport::NewInstance(
    Napi::Env env, std::shared_ptr<PeerConnectionFactoryWrapper> factory,
    rtc::scoped_refptr<IceTransportInterface> iceTransport)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!iceTransport || !factory) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object::New(env));
    }

    auto externalFactory = External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory);
    auto externalTransport = External<IceTransportInterface>::New(
        env, iceTransport.release(), [](Napi::Env env, IceTransportInterface* transport) { transport->Release(); });
    return constructor_.New({externalFactory, externalTransport});
}

NapiIceTransport::NapiIceTransport(const Napi::CallbackInfo& info) : NapiEventTarget<NapiIceTransport>(info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() != callbackInfoLen || !info[0].IsExternal() || !info[1].IsExternal()) {
        // You cannot construct an RTCIceTransport
        NAPI_THROW(Napi::Error::New(info.Env(), "Invalid Operation"));
        return;
    }

    factory_ = *info[0].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();

    auto iceTransport = info[1].As<External<IceTransportInterface>>().Data();
    iceTransport_ = rtc::scoped_refptr<IceTransportInterface>(iceTransport);

    factory_->GetNetworkThread()->BlockingCall([this] {
        auto internal = iceTransport_->internal();
        if (internal) {
            internal->SignalIceTransportStateChanged.connect(this, &NapiIceTransport::OnStateChange);
            internal->SignalGatheringState.connect(this, &NapiIceTransport::OnGatheringStateChange);
            internal->SignalCandidatePairChanged.connect(this, &NapiIceTransport::OnSelectedCandidatePairChange);

            iceTransportState_ = internal->GetIceTransportState();
            iceGatheringState_ = internal->gathering_state();
        } else {
            iceTransportState_ = IceTransportState::kClosed;
            iceGatheringState_ = cricket::kIceGatheringComplete;
        }

        if (iceTransportState_ == IceTransportState::kClosed) {
            Stop();
        }
    });
}

Napi::Value NapiIceTransport::GetRole(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    cricket::IceRole iceRole;

    factory_->GetNetworkThread()->BlockingCall([&iceRole, this] { iceRole = iceTransport_->internal()->GetIceRole(); });

    switch (iceRole) {
        case cricket::ICEROLE_CONTROLLING:
            return String::New(info.Env(), kEnumRTCIceRoleControlling);
        case cricket::ICEROLE_CONTROLLED:
            return String::New(info.Env(), kEnumRTCIceRoleControlled);
        case cricket::ICEROLE_UNKNOWN:
            return String::New(info.Env(), kEnumRTCIceRoleUnKnown);
        default:
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid role"), info.Env().Undefined());
}

Napi::Value NapiIceTransport::GetComponent(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    int component = 0;

    factory_->GetNetworkThread()->BlockingCall(
        [&component, this] { component = iceTransport_->internal()->component(); });

    if (component == kComponentIdRtp) {
        return String::New(info.Env(), kEnumRTCIceComponentRtp);
    }
    if (component == kComponentIdRtcp) {
        return String::New(info.Env(), kEnumRTCIceComponentRtcp);
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid component"), info.Env().Undefined());
}

Napi::Value NapiIceTransport::GetState(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    IceTransportState iceTransportState = iceTransportState_;
    switch (iceTransportState) {
        case IceTransportState::kNew:
            return String::New(info.Env(), kEnumRTCIceTransportStateNew);
        case IceTransportState::kChecking:
            return String::New(info.Env(), kEnumRTCIceTransportStateChecking);
        case IceTransportState::kConnected:
            return String::New(info.Env(), kEnumRTCIceTransportStateConnected);
        case IceTransportState::kCompleted:
            return String::New(info.Env(), kEnumRTCIceTransportStateCompleted);
        case IceTransportState::kFailed:
            return String::New(info.Env(), kEnumRTCIceTransportStateFailed);
        case IceTransportState::kDisconnected:
            return String::New(info.Env(), kEnumRTCIceTransportStateDisconnected);
        case IceTransportState::kClosed:
            return String::New(info.Env(), kEnumRTCIceTransportStateClosed);
        default:
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid state"), info.Env().Undefined());
}

Napi::Value NapiIceTransport::GetGatheringState(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    cricket::IceGatheringState iceGatheringState = iceGatheringState_;
    switch (iceGatheringState) {
        case cricket::kIceGatheringNew:
            return String::New(info.Env(), kEnumRTCIceGathererStateNew);
        case cricket::kIceGatheringGathering:
            return String::New(info.Env(), kEnumRTCIceGathererStateGathering);
        case cricket::kIceGatheringComplete:
            return String::New(info.Env(), kEnumRTCIceGathererStateComplete);
        default:
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid gathering state"), info.Env().Undefined());
}

Napi::Value NapiIceTransport::GetEventHandler(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    Function func;
    if (!NapiEventTarget<NapiIceTransport>::GetEventHandler(type, func)) {
        return info.Env().Null();
    }

    return func;
}

void NapiIceTransport::SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    if (value.IsFunction()) {
        auto fn = value.As<Function>();
        NapiEventTarget<NapiIceTransport>::SetEventHandler(type, fn);
    } else if (value.IsNull()) {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " value is null";
        RemoveEventHandler(type);
    } else {
        NAPI_THROW_VOID(Error::New(info.Env(), "value is error"));
    }
}

Napi::Value NapiIceTransport::GetSelectedCandidatePair(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    absl::optional<cricket::CandidatePair> candidatePair;
    factory_->GetNetworkThread()->BlockingCall(
        [&candidatePair, this] { candidatePair = iceTransport_->internal()->GetSelectedCandidatePair(); });

    if (!candidatePair) {
        return info.Env().Null();
    }

    auto obj = Object::New(info.Env());
    obj.Set("local", NativeToJsCandidate(info.Env(), candidatePair->local_candidate()));
    obj.Set("remote", NativeToJsCandidate(info.Env(), candidatePair->remote_candidate()));
    return obj;
}

Napi::Value NapiIceTransport::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiIceTransport"));
#endif

    return json;
}

void NapiIceTransport::OnStateChange(cricket::IceTransportInternal* iceTransport)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    iceTransportState_ = iceTransport_->internal()->GetIceTransportState();

    Dispatch(
        CallbackEvent<NapiIceTransport>::Create([this, state = iceTransportState_.load()](NapiIceTransport& target) {
            RTC_DCHECK_EQ(this, &target);

            auto env = target.Env();
            Napi::HandleScope scope(env);
            auto jsEvent = Object::New(env);
            jsEvent.Set("type", String::New(env, kEventNameStateChange));
            target.MakeCallback(kEventNameStateChange, {jsEvent});

            if (state == IceTransportState::kClosed) {
                target.Stop();
            }
        }));
}

void NapiIceTransport::OnGatheringStateChange(cricket::IceTransportInternal* iceTransport)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    iceGatheringState_ = iceTransport->gathering_state();

    Dispatch(
        CallbackEvent<NapiIceTransport>::Create([this, state = iceGatheringState_.load()](NapiIceTransport& target) {
            RTC_DCHECK_EQ(this, &target);

            auto env = target.Env();
            Napi::HandleScope scope(env);
            auto jsEvent = Object::New(env);
            jsEvent.Set("type", String::New(env, kEventNameGatheringStateChange));
            target.MakeCallback(kEventNameGatheringStateChange, {jsEvent});
        }));
}

void NapiIceTransport::OnSelectedCandidatePairChange(const cricket::CandidatePairChangeEvent& event)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Dispatch(
        CallbackEvent<NapiIceTransport>::Create([this, state = iceGatheringState_.load()](NapiIceTransport& target) {
            RTC_DCHECK_EQ(this, &target);

            auto env = target.Env();
            Napi::HandleScope scope(env);
            auto jsEvent = Object::New(env);
            jsEvent.Set("type", String::New(env, kEventNameSelectedCandidatePairChange));
            target.MakeCallback(kEventNameSelectedCandidatePairChange, {jsEvent});
        }));
}

} // namespace webrtc
