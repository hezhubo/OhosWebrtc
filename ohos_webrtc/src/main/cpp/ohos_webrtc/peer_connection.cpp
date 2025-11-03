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

#include "peer_connection.h"
#include "peer_connection_factory.h"

#include <future>
#include <thread>

#include <hilog/log.h>

#include "rtc_base/logging.h"

#include "configuration.h"
#include "data_channel.h"
#include "ice_candidate.h"
#include "media_stream.h"
#include "media_stream_track.h"
#include "rtp_receiver.h"
#include "rtp_sender.h"
#include "rtp_transceiver.h"
#include "sctp_transport.h"
#include "session_description.h"
#include "utils/marcos.h"
#include "async_work/async_worker_certificate.h"
#include "async_work/async_worker_get_stats.h"
#include "audio_device/ohos_local_audio_source.h"

namespace webrtc {

using namespace Napi;

const char kEnumSignalingStateStable[] = "stable";
const char kEnumSignalingStateHaveLocalOffer[] = "have-local-offer";
const char kEnumSignalingStateHaveLocalPranswer[] = "have-local-pranswer";
const char kEnumSignalingStateHaveRemoteOffer[] = "have-remote-offer";
const char kEnumSignalingStateHaveRemotePranswer[] = "have-remote-pranswer";
const char kEnumSignalingStateClosed[] = "closed";

const char kEnumIceGatheringStateNew[] = "new";
const char kEnumIceGatheringStateGathering[] = "gathering";
const char kEnumIceGatheringStateComplete[] = "complete";

const char kEnumIceConnectionStateNew[] = "new";
const char kEnumIceConnectionStateChecking[] = "checking";
const char kEnumIceConnectionStateCompleted[] = "completed";
const char kEnumIceConnectionStateConnected[] = "connected";
const char kEnumIceConnectionStateDisconnected[] = "disconnected";
const char kEnumIceConnectionStateFailed[] = "failed";
const char kEnumIceConnectionStateClosed[] = "closed";

const char kEnumPeerConnectionStateNew[] = "new";
const char kEnumPeerConnectionStateConnecting[] = "connecting";
const char kEnumPeerConnectionStateConnected[] = "connected";
const char kEnumPeerConnectionStateDisconnected[] = "disconnected";
const char kEnumPeerConnectionStateFailed[] = "failed";
const char kEnumPeerConnectionStateClosed[] = "closed";

const char kClassName[] = "RTCPeerConnection";

const char kAttributeNameCanTrickleIceCandidates[] = "canTrickleIceCandidates";
const char kAttributeNameSignalingState[] = "signalingState";
const char kAttributeNameIceGatheringState[] = "iceGatheringState";
const char kAttributeNameIceConnectionState[] = "iceConnectionState";
const char kAttributeNameConnectionState[] = "connectionState";
const char kAttributeNameLocalDescription[] = "localDescription";
const char kAttributeNameRemoteDescription[] = "remoteDescription";
const char kAttributeNameCurrentLocalDescription[] = "currentLocalDescription";
const char kAttributeNameCurrentRemoteDescription[] = "currentRemoteDescription";
const char kAttributeNamePendingLocalDescription[] = "pendingLocalDescription";
const char kAttributeNamePendingRemoteDescription[] = "pendingRemoteDescription";
const char kAttributeNameSctp[] = "sctp";
const char kAttributeNameOnConnectionStateChange[] = "onconnectionstatechange";
const char kAttributeNameOnIceCandidate[] = "onicecandidate";
const char kAttributeNameOnIceCandidateError[] = "onicecandidateerror";
const char kAttributeNameOnIceConnectionStateChange[] = "oniceconnectionstatechange";
const char kAttributeNameOnIceGatheringStateChange[] = "onicegatheringstatechange";
const char kAttributeNameOnNegotiationNeeded[] = "onnegotiationneeded";
const char kAttributeNameOnSignalingStateChange[] = "onsignalingstatechange";
const char kAttributeNameOnTrack[] = "ontrack";
const char kAttributeNameOnDataChannel[] = "ondatachannel";

const char kMethodNameAddTrack[] = "addTrack";
const char kMethodNameRemoveTrack[] = "removeTrack";
const char kMethodNameSetLocalDescription[] = "setLocalDescription";
const char kMethodNameSetRemoteDescription[] = "setRemoteDescription";
const char kMethodNameCreateOffer[] = "createOffer";
const char kMethodNameCreateAnswer[] = "createAnswer";
const char kMethodNameCreateDataChannel[] = "createDataChannel";
const char kMethodNameAddIceCandidate[] = "addIceCandidate";
const char kMethodNameGetSenders[] = "getSenders";
const char kMethodNameGetReceivers[] = "getReceivers";
const char kMethodNameGetTransceivers[] = "getTransceivers";
const char kMethodNameGetConfiguration[] = "getConfiguration";
const char kMethodNameRestartIce[] = "restartIce";
const char kMethodNameSetConfiguration[] = "setConfiguration";
const char kMethodNameAddTransceiver[] = "addTransceiver";
const char kMethodNameClose[] = "close";
const char kMethodNameGetStats[] = "getStats";
const char kMethodNameToJson[] = "toJSON";
const char kMethodNameSetAudioRecording[] = "setAudioRecording";
const char kMethodNameSetAudioPlayout[] = "setAudioPlayout";

const char kStaticMethodNameGenerateCertificate[] = "generateCertificate";

const char kEventConnectionStateChange[] = "connectionstatechange";
const char kEventIceCandidate[] = "icecandidate";
const char kEventIceCandidateError[] = "icecandidateerror";
const char kEventIceConnectionStateChange[] = "iceconnectionstatechange";
const char kEventIceGatheringStateChange[] = "icegatheringstatechange";
const char kEventNegotiationNeeded[] = "negotiationneeded";
const char kEventSignalingStateChange[] = "signalingstatechange";
const char kEventTrack[] = "track";
const char kEventDataChannel[] = "datachannel";

template <typename T>
class BaseSdpObserver : public T {
public:
    virtual ~BaseSdpObserver()
    {
        tsfn_.Release();
    }

    Promise GetPromise()
    {
        return deferred_.Promise();
    }

protected:
    explicit BaseSdpObserver(Promise::Deferred deferred) : deferred_(deferred) {}

    ThreadSafeFunction tsfn_;
    Promise::Deferred deferred_;
};

class CreateSdpObserver : public BaseSdpObserver<CreateSessionDescriptionObserver> {
public:
    explicit CreateSdpObserver(Napi::Env env) : CreateSdpObserver(env, Promise::Deferred::New(env)) {}

    CreateSdpObserver(Napi::Env env, Promise::Deferred deferred)
        : BaseSdpObserver<CreateSessionDescriptionObserver>(deferred)
    {
        tsfn_ = ThreadSafeFunction::New(
            env,
            Function::New(
                env,
                [deferred](const CallbackInfo& info) {
                    auto success = info[0].As<Boolean>().Value();
                    if (success) {
                        auto desc = info[1].As<External<SessionDescriptionInterface>>().Data();

                        std::string sdp;
                        desc->ToString(&sdp);
                        RTC_DLOG(LS_VERBOSE) << "sdp: " << sdp;

                        auto result = Napi::Object::New(info.Env());
                        result.Set("sdp", Napi::String::New(info.Env(), sdp));
                        result.Set("type", Napi::String::New(info.Env(), webrtc::SdpTypeToString(desc->GetType())));
                        deferred.Resolve(result);
                    } else {
                        auto error = info[1].As<External<RTCError>>().Data();
                        auto message = error->message();
                        deferred.Reject(
                            Error::New(info.Env(), (message && strlen(message) > 0) ? message : "unknown error")
                                .Value());
                    }
                }),
            "CreateSdpObserver", 0, 1);
    }

protected:
    void OnSuccess(SessionDescriptionInterface* desc) override
    {
        RTC_LOG(LS_INFO) << "CreateSessionDescription success: " << desc;

        tsfn_.BlockingCall([desc](Napi::Env env, Napi::Function jsCallback) {
            auto externalDesc = External<SessionDescriptionInterface>::New(
                env, desc, [](Napi::Env, SessionDescriptionInterface* desc) { delete desc; });
            jsCallback.Call({Boolean::New(env, true), externalDesc});
        });
    }

    void OnFailure(RTCError error) override
    {
        RTC_LOG(LS_ERROR) << "CreateSessionDescription failed";

        tsfn_.BlockingCall([error = new RTCError(std::move(error))](Napi::Env env, Napi::Function jsCallback) {
            auto externalError =
                External<RTCError>::New(env, new RTCError(RTCError::OK()), [](Napi::Env, RTCError* e) { delete e; });
            jsCallback.Call({Boolean::New(env, false), externalError});
        });
    }
};

class SetLocalSdpObserver : public BaseSdpObserver<SetLocalDescriptionObserverInterface> {
public:
    explicit SetLocalSdpObserver(Napi::Env env) : SetLocalSdpObserver(env, Promise::Deferred::New(env)) {}

    SetLocalSdpObserver(Napi::Env env, Promise::Deferred deferred)
        : BaseSdpObserver<SetLocalDescriptionObserverInterface>(deferred)
    {
        tsfn_ = ThreadSafeFunction::New(
            env,
            Function::New(
                env,
                [deferred](const CallbackInfo& info) {
                    auto error = info[0].As<External<RTCError>>().Data();
                    if (error->ok()) {
                        deferred.Resolve(info.Env().Undefined());
                    } else {
                        auto message = error->message();
                        deferred.Reject(
                            Error::New(info.Env(), (message && strlen(message) > 0) ? message : "unknown error")
                                .Value());
                    }
                }),
            "SetLocalSdpObserver", 0, 1);
    }

protected:
    void OnSetLocalDescriptionComplete(RTCError error) override
    {
        RTC_DLOG(LS_INFO) << __FUNCTION__;
        if (!error.ok()) {
            RTC_LOG(LS_ERROR) << "Error: " << error.type() << ", " << error.message();
        }

        tsfn_.BlockingCall([error = new RTCError(std::move(error))](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({External<RTCError>::New(env, error, [](Napi::Env, RTCError* e) { delete e; })});
        });
    }
};

class SetRemoteSdpObserver : public BaseSdpObserver<SetRemoteDescriptionObserverInterface> {
public:
    explicit SetRemoteSdpObserver(Napi::Env env) : SetRemoteSdpObserver(env, Promise::Deferred::New(env)) {}

    SetRemoteSdpObserver(Napi::Env env, Promise::Deferred deferred)
        : BaseSdpObserver<SetRemoteDescriptionObserverInterface>(deferred)
    {
        tsfn_ = ThreadSafeFunction::New(
            env,
            Function::New(
                env,
                [deferred](const CallbackInfo& info) {
                    auto error = info[0].As<External<RTCError>>().Data();
                    if (error->ok()) {
                        deferred.Resolve(info.Env().Undefined());
                    } else {
                        auto message = error->message();
                        deferred.Reject(
                            Error::New(info.Env(), (message && strlen(message) > 0) ? message : "unknown error")
                                .Value());
                    }
                }),
            "SetRemoteSdpObserver", 0, 1);
    }

protected:
    void OnSetRemoteDescriptionComplete(RTCError error) override
    {
        RTC_DLOG(LS_INFO) << __FUNCTION__;
        if (!error.ok()) {
            RTC_LOG(LS_ERROR) << "Error: " << error.type() << ", " << error.message();
        }

        tsfn_.BlockingCall([error = new RTCError(std::move(error))](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({External<RTCError>::New(env, error, [](Napi::Env, RTCError* e) { delete e; })});
        });
    }
};

/// NapiPeerConnection
FunctionReference NapiPeerConnection::constructor_;

void NapiPeerConnection::Init(class Env env, Object exports)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiPeerConnection::GetCanTrickleIceCandidates>(kAttributeNameCanTrickleIceCandidates),
            InstanceAccessor<&NapiPeerConnection::GetSignalingState>(kAttributeNameSignalingState),
            InstanceAccessor<&NapiPeerConnection::GetIceGatheringState>(kAttributeNameIceGatheringState),
            InstanceAccessor<&NapiPeerConnection::GetIceConnectionState>(kAttributeNameIceConnectionState),
            InstanceAccessor<&NapiPeerConnection::GetConnectionState>(kAttributeNameConnectionState),
            InstanceAccessor<&NapiPeerConnection::GetLocalDescription>(kAttributeNameLocalDescription),
            InstanceAccessor<&NapiPeerConnection::GetRemoteDescription>(kAttributeNameRemoteDescription),
            InstanceAccessor<&NapiPeerConnection::GetCurrentLocalDescription>(kAttributeNameCurrentLocalDescription),
            InstanceAccessor<&NapiPeerConnection::GetCurrentRemoteDescription>(kAttributeNameCurrentRemoteDescription),
            InstanceAccessor<&NapiPeerConnection::GetPendingLocalDescription>(kAttributeNamePendingLocalDescription),
            InstanceAccessor<&NapiPeerConnection::GetPendingRemoteDescription>(kAttributeNamePendingRemoteDescription),
            InstanceAccessor<&NapiPeerConnection::GetSctp>(kAttributeNameSctp),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnIceCandidate, napi_default, (void*)kEventIceCandidate),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnIceCandidateError, napi_default, (void*)kEventIceCandidateError),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnIceConnectionStateChange, napi_default, (void*)kEventIceConnectionStateChange),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnIceGatheringStateChange, napi_default, (void*)kEventIceGatheringStateChange),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnConnectionStateChange, napi_default, (void*)kEventConnectionStateChange),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnSignalingStateChange, napi_default, (void*)kEventSignalingStateChange),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnNegotiationNeeded, napi_default, (void*)kEventNegotiationNeeded),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnTrack, napi_default, (void*)kEventTrack),
            InstanceAccessor<&NapiPeerConnection::GetEventHandler, &NapiPeerConnection::SetEventHandler>(
                kAttributeNameOnDataChannel, napi_default, (void*)kEventDataChannel),
            InstanceMethod<&NapiPeerConnection::AddTrack>(kMethodNameAddTrack),
            InstanceMethod<&NapiPeerConnection::RemoveTrack>(kMethodNameRemoveTrack),
            InstanceMethod<&NapiPeerConnection::SetLocalDescription>(kMethodNameSetLocalDescription),
            InstanceMethod<&NapiPeerConnection::SetRemoteDescription>(kMethodNameSetRemoteDescription),
            InstanceMethod<&NapiPeerConnection::CreateOffer>(kMethodNameCreateOffer),
            InstanceMethod<&NapiPeerConnection::CreateAnswer>(kMethodNameCreateAnswer),
            InstanceMethod<&NapiPeerConnection::CreateDataChannel>(kMethodNameCreateDataChannel),
            InstanceMethod<&NapiPeerConnection::AddIceCandidate>(kMethodNameAddIceCandidate),
            InstanceMethod<&NapiPeerConnection::GetSenders>(kMethodNameGetSenders),
            InstanceMethod<&NapiPeerConnection::GetReceivers>(kMethodNameGetReceivers),
            InstanceMethod<&NapiPeerConnection::GetTransceivers>(kMethodNameGetTransceivers),
            InstanceMethod<&NapiPeerConnection::GetConfiguration>(kMethodNameGetConfiguration),
            InstanceMethod<&NapiPeerConnection::RestartIce>(kMethodNameRestartIce),
            InstanceMethod<&NapiPeerConnection::SetConfiguration>(kMethodNameSetConfiguration),
            InstanceMethod<&NapiPeerConnection::AddTransceiver>(kMethodNameAddTransceiver),
            InstanceMethod<&NapiPeerConnection::Close>(kMethodNameClose),
            InstanceMethod<&NapiPeerConnection::GetStats>(kMethodNameGetStats),
            InstanceMethod<&NapiPeerConnection::ToJson>(kMethodNameToJson),
            InstanceMethod<&NapiPeerConnection::SetAudioRecording>(kMethodNameSetAudioRecording),
            InstanceMethod<&NapiPeerConnection::SetAudioPlayout>(kMethodNameSetAudioPlayout),
            StaticMethod<&NapiPeerConnection::GenerateCertificate>(kStaticMethodNameGenerateCertificate),
        });
    if (func.IsEmpty() || func.IsUndefined()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, "NapiPeerConnection", "func is empty or undefined");
    }
    constructor_ = Persistent(func);
    OH_LOG_Print(
        LOG_APP, LOG_INFO, LOG_DOMAIN, "NapiPeerConnection", "constructor_=%{public}p", (napi_ref)constructor_);

    exports.Set(kClassName, func);
}

Napi::Value NapiPeerConnection::NewInstance(Napi::Value configuration,
    std::shared_ptr<PeerConnectionFactoryWrapper> factory)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    auto env = constructor_.Env();
    if (!factory || !factory->GetFactory()) {
        NAPI_THROW(Error::New(env, "Invalid argument"), Object());
    }

    return constructor_.New(
        {configuration, External<std::shared_ptr<PeerConnectionFactoryWrapper>>::New(env, &factory)});
}

NapiPeerConnection::NapiPeerConnection(const CallbackInfo& info) : NapiEventTarget<NapiPeerConnection>(info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!info.IsConstructCall()) {
        NAPI_THROW_VOID(TypeError::New(info.Env(), "Use the new operator to construct the RTCPeerConnection"));
        return;
    }

    PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = SdpSemantics::kUnifiedPlan;

    if (info.Length() > 0 && info[0].IsObject()) {
        JsToNativeConfiguration(info[0].As<Object>(), config);
    }

    if (info.Length() > 1 && info[1].IsExternal()) {
        factory_ = *info[1].As<External<std::shared_ptr<PeerConnectionFactoryWrapper>>>().Data();
    } else {
        factory_ = PeerConnectionFactoryWrapper::GetDefault();
    }

    PeerConnectionDependencies deps(this);
    auto result = factory_->GetFactory()->CreatePeerConnectionOrError(config, std::move(deps));
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to create PeerConnection: " << result.error().message();
        NAPI_THROW_VOID(TypeError::New(info.Env(), result.error().message()));
    }

    pc_ = result.MoveValue();
}

NapiPeerConnection::~NapiPeerConnection()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

Napi::Value NapiPeerConnection::GetCanTrickleIceCandidates(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (pc_->can_trickle_ice_candidates()) {
        return Boolean::New(info.Env(), pc_->can_trickle_ice_candidates().value());
    }

    return info.Env().Undefined();
}

Napi::Value NapiPeerConnection::GetSignalingState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    switch (pc_->signaling_state()) {
        case PeerConnectionInterface::kStable:
            return String::New(info.Env(), kEnumSignalingStateStable);
        case PeerConnectionInterface::kHaveLocalOffer:
            return String::New(info.Env(), kEnumSignalingStateHaveLocalOffer);
        case PeerConnectionInterface::kHaveLocalPrAnswer:
            return String::New(info.Env(), kEnumSignalingStateHaveLocalPranswer);
        case PeerConnectionInterface::kHaveRemoteOffer:
            return String::New(info.Env(), kEnumSignalingStateHaveRemoteOffer);
        case PeerConnectionInterface::kHaveRemotePrAnswer:
            return String::New(info.Env(), kEnumSignalingStateHaveRemotePranswer);
        case PeerConnectionInterface::kClosed:
            return String::New(info.Env(), kEnumSignalingStateClosed);
        default:
            RTC_LOG(LS_WARNING) << "Invalid value of signalingState";
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
}

Napi::Value NapiPeerConnection::GetIceGatheringState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    switch (pc_->ice_gathering_state()) {
        case PeerConnectionInterface::kIceGatheringNew:
            return String::New(info.Env(), kEnumIceGatheringStateNew);
        case PeerConnectionInterface::kIceGatheringGathering:
            return String::New(info.Env(), kEnumIceGatheringStateGathering);
        case PeerConnectionInterface::kIceGatheringComplete:
            return String::New(info.Env(), kEnumIceGatheringStateComplete);
        default:
            RTC_LOG(LS_WARNING) << "Invalid value of iceGatheringState";
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
}

Napi::Value NapiPeerConnection::GetIceConnectionState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    switch (pc_->ice_connection_state()) {
        case PeerConnectionInterface::kIceConnectionNew:
            return String::New(info.Env(), kEnumIceConnectionStateNew);
        case PeerConnectionInterface::kIceConnectionChecking:
            return String::New(info.Env(), kEnumIceConnectionStateChecking);
        case PeerConnectionInterface::kIceConnectionConnected:
            return String::New(info.Env(), kEnumIceConnectionStateConnected);
        case PeerConnectionInterface::kIceConnectionCompleted:
            return String::New(info.Env(), kEnumIceConnectionStateCompleted);
        case PeerConnectionInterface::kIceConnectionFailed:
            return String::New(info.Env(), kEnumIceConnectionStateFailed);
        case PeerConnectionInterface::kIceConnectionDisconnected:
            return String::New(info.Env(), kEnumIceConnectionStateDisconnected);
        case PeerConnectionInterface::kIceConnectionClosed:
            return String::New(info.Env(), kEnumIceConnectionStateClosed);
        default:
            RTC_LOG(LS_WARNING) << "Invalid value of iceConnectionState";
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
}

Napi::Value NapiPeerConnection::GetConnectionState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    switch (pc_->peer_connection_state()) {
        case PeerConnectionInterface::PeerConnectionState::kNew:
            return String::New(info.Env(), kEnumPeerConnectionStateNew);
        case PeerConnectionInterface::PeerConnectionState::kConnecting:
            return String::New(info.Env(), kEnumPeerConnectionStateConnecting);
        case PeerConnectionInterface::PeerConnectionState::kConnected:
            return String::New(info.Env(), kEnumPeerConnectionStateConnected);
        case PeerConnectionInterface::PeerConnectionState::kDisconnected:
            return String::New(info.Env(), kEnumPeerConnectionStateDisconnected);
        case PeerConnectionInterface::PeerConnectionState::kFailed:
            return String::New(info.Env(), kEnumPeerConnectionStateFailed);
        case PeerConnectionInterface::PeerConnectionState::kClosed:
            return String::New(info.Env(), kEnumPeerConnectionStateClosed);
        default:
            RTC_LOG(LS_WARNING) << "Invalid value of connectionState";
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
}

Napi::Value NapiPeerConnection::GetLocalDescription(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // It's only safe to operate on SessionDescriptionInterface on the signaling thread
    std::string sdp;
    std::string type;
    pc_->signaling_thread()->BlockingCall([pc = pc_, &sdp, &type] {
        const SessionDescriptionInterface* desc = pc->local_description();
        if (desc) {
            if (desc->ToString(&sdp)) {
                type = desc->type();
            }
        }
    });

    if (sdp.empty()) {
        return info.Env().Undefined();
    }

    auto sdpType = SdpTypeFromString(type);
    if (!sdpType) {
        NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
    }

    return NapiSessionDescription::NewInstance(info.Env(), sdp, sdpType.value());
}

Napi::Value NapiPeerConnection::GetRemoteDescription(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // It's only safe to operate on SessionDescriptionInterface on the signaling thread
    std::string sdp;
    std::string type;
    pc_->signaling_thread()->BlockingCall([pc = pc_, &sdp, &type] {
        const SessionDescriptionInterface* desc = pc->remote_description();
        if (desc) {
            if (desc->ToString(&sdp)) {
                type = desc->type();
            }
        }
    });

    if (sdp.empty()) {
        return info.Env().Undefined();
    }

    auto sdpType = SdpTypeFromString(type);
    if (!sdpType) {
        NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
    }

    return NapiSessionDescription::NewInstance(info.Env(), sdp, sdpType.value());
}

Napi::Value NapiPeerConnection::GetCurrentLocalDescription(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // It's only safe to operate on SessionDescriptionInterface on the signaling thread
    std::string sdp;
    std::string type;
    pc_->signaling_thread()->BlockingCall([pc = pc_, &sdp, &type] {
        const SessionDescriptionInterface* desc = pc->current_local_description();
        if (desc) {
            if (desc->ToString(&sdp)) {
                type = desc->type();
            }
        }
    });

    if (sdp.empty()) {
        return info.Env().Undefined();
    }

    auto sdpType = SdpTypeFromString(type);
    if (!sdpType) {
        NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
    }

    return NapiSessionDescription::NewInstance(info.Env(), sdp, sdpType.value());
}

Napi::Value NapiPeerConnection::GetCurrentRemoteDescription(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // It's only safe to operate on SessionDescriptionInterface on the signaling thread
    std::string sdp;
    std::string type;
    pc_->signaling_thread()->BlockingCall([pc = pc_, &sdp, &type] {
        const SessionDescriptionInterface* desc = pc->current_remote_description();
        if (desc) {
            if (desc->ToString(&sdp)) {
                type = desc->type();
            }
        }
    });

    if (sdp.empty()) {
        return info.Env().Undefined();
    }

    auto sdpType = SdpTypeFromString(type);
    if (!sdpType) {
        NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
    }

    return NapiSessionDescription::NewInstance(info.Env(), sdp, sdpType.value());
}

Napi::Value NapiPeerConnection::GetPendingLocalDescription(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // It's only safe to operate on SessionDescriptionInterface on the signaling thread
    std::string sdp;
    std::string type;
    pc_->signaling_thread()->BlockingCall([pc = pc_, &sdp, &type] {
        const SessionDescriptionInterface* desc = pc->pending_local_description();
        if (desc) {
            if (desc->ToString(&sdp)) {
                type = desc->type();
            }
        }
    });

    if (sdp.empty()) {
        return info.Env().Undefined();
    }

    auto sdpType = SdpTypeFromString(type);
    if (!sdpType) {
        NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
    }

    return NapiSessionDescription::NewInstance(info.Env(), sdp, sdpType.value());
}

Napi::Value NapiPeerConnection::GetPendingRemoteDescription(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // It's only safe to operate on SessionDescriptionInterface on the signaling thread
    std::string sdp;
    std::string type;
    pc_->signaling_thread()->BlockingCall([pc = pc_, &sdp, &type] {
        const SessionDescriptionInterface* desc = pc->pending_remote_description();
        if (desc) {
            if (desc->ToString(&sdp)) {
                type = desc->type();
            }
        }
    });

    if (sdp.empty()) {
        return info.Env().Undefined();
    }

    auto sdpType = SdpTypeFromString(type);
    if (!sdpType) {
        NAPI_THROW(Error::New(info.Env(), "Invalid value"), info.Env().Undefined());
    }

    return NapiSessionDescription::NewInstance(info.Env(), sdp, sdpType.value());
}

Napi::Value NapiPeerConnection::GetSctp(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!sctpTransportRef_.IsEmpty()) {
        return sctpTransportRef_.Value();
    }

    auto transport = pc_->GetSctpTransport();
    if (!transport) {
        return info.Env().Undefined();
    }

    auto sctpTransport = NapiSctpTransport::NewInstance(factory_, transport);
    sctpTransportRef_ = Persistent(sctpTransport);

    return sctpTransport;
}

Napi::Value NapiPeerConnection::GetEventHandler(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    Function fn;
    if (!NapiEventTarget<NapiPeerConnection>::GetEventHandler(type, fn)) {
        return info.Env().Null();
    }

    return fn;
}

void NapiPeerConnection::SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    if (value.IsFunction()) {
        auto fn = value.As<Function>();
        NapiEventTarget<NapiPeerConnection>::SetEventHandler(type, fn);
    } else if (value.IsNull()) {
        RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " value is null";
        RemoveEventHandler(type);
    } else {
        NAPI_THROW_VOID(Error::New(info.Env(), "value is error"));
    }
}

Napi::Value NapiPeerConnection::AddTrack(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsObject()) {
        NAPI_THROW(Error::New(info.Env(), "First argument is not object"), info.Env().Undefined());
    }

    auto jsTrack = info[0].As<Object>();
    if (jsTrack.Has("type")) {
        auto type = jsTrack.Get("type").As<String>().Utf8Value();
        RTC_DLOG(LS_VERBOSE) << "type=" << type;
    }

    std::vector<std::string> streamIds;
    if (info.Length() >= 2) {
        // optional parameters from 2
        for (uint32_t i = 1; i < info.Length(); i++) {
            if (!info[i].IsObject()) {
                NAPI_THROW(Error::New(info.Env(), "The argument is not object"), info.Env().Undefined());
            }

            auto jsStream = info[i].As<Object>();
            auto napiStream = NapiMediaStream::Unwrap(jsStream);
            if (!napiStream) {
                NAPI_THROW(Error::New(info.Env(), "The argument is not MediaStream"), info.Env().Undefined());
            }

            auto stream = napiStream->Get();
            if (stream) {
                streamIds.push_back(stream->id());
            }
        }
    } else {
        streamIds.push_back("stream_id");
    }

    auto napiTrack = NapiMediaStreamTrack::Unwrap(jsTrack);
    auto track = napiTrack->Get();
    auto result = pc_->AddTrack(track, streamIds);
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: " << result.error().message();
        NAPI_THROW(Error::New(info.Env(), result.error().message()), info.Env().Undefined());
    }

    if (track && track->kind() == MediaStreamTrackInterface::kAudioKind) {
        // Add audio input to adm
        auto source = factory_->GetAudioSource(track);
        auto adm = factory_->GetAudioDeviceModule();
        if (source && adm) {
            adm->AddAudioInput(source->GetAudioInput());
        }
    }

    return NapiRtpSender::NewInstance(factory_, pc_, result.value());
}

Napi::Value NapiPeerConnection::RemoveTrack(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsObject()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    auto jsSender = info[0].As<Object>();
    auto sender = NapiRtpSender::Unwrap(jsSender);

    auto error = pc_->RemoveTrackOrError(sender->Get());
    if (!error.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to remove track: " << error.message();
        std::string message;
        switch (error.type()) {
            case RTCErrorType::INVALID_PARAMETER:
                message = "Invalid argument";
                break;
            case RTCErrorType::INVALID_STATE:
                message = "Invalid state";
                break;
            default:
                message = "unknown error";
                break;
        }
        NAPI_THROW(Error::New(info.Env(), message), info.Env().Undefined());
    }

    auto track = sender->Get()->track();
    if (track && track->kind() == MediaStreamTrackInterface::kAudioKind) {
        // Remove audio input from adm
        auto source = factory_->GetAudioSource(track);
        auto adm = factory_->GetAudioDeviceModule();
        if (adm && source) {
            adm->RemoveAudioInput(source->GetAudioInput());
        }
    }

    return info.Env().Undefined();
}

Napi::Value NapiPeerConnection::SetLocalDescription(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    std::unique_ptr<SessionDescriptionInterface> desc;

    if (info.Length() > 0) {
        if (!info[0].IsObject()) {
            NAPI_THROW(TypeError::New(info.Env(), "first argument must be a object"), info.Env().Undefined());
        }

        Object jsSdp = info[0].As<Object>();
        std::string sdp = "";
        if (jsSdp.Has("sdp")) {
            sdp = jsSdp.Get("sdp").As<String>();
        }
        std::string type = jsSdp.Get("type").As<String>();

        auto sdpType = SdpTypeFromString(type);
        if (!sdpType) {
            NAPI_THROW(Error::New(info.Env(), "invalid argument"), info.Env().Undefined());
        }

        SdpParseError error;
        desc = CreateSessionDescription(*sdpType, sdp, &error);
        if (!desc) {
            RTC_DLOG(LS_WARNING) << "Can't parse received session description message. SdpParseError was: "
                                 << error.description;
            NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
        }
    }

    auto observer = rtc::make_ref_counted<SetLocalSdpObserver>(info.Env());
    if (desc) {
        pc_->SetLocalDescription(std::move(desc), observer);
    } else {
        pc_->SetLocalDescription(observer);
    }

    return observer->GetPromise();
}

Napi::Value NapiPeerConnection::SetRemoteDescription(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of argument"), info.Env().Undefined());
    }

    if (!info[0].IsObject()) {
        NAPI_THROW(TypeError::New(info.Env(), "First argument is not object"), info.Env().Undefined());
    }

    Object jsSdp = info[0].As<Object>();
    std::string sdp = "";
    if (jsSdp.Has("sdp")) {
        sdp = jsSdp.Get("sdp").As<String>();
    }
    std::string type = jsSdp.Get("type").As<String>();

    auto sdpType = SdpTypeFromString(type);
    if (!sdpType) {
        NAPI_THROW(Error::New(info.Env(), "Invalid sdp type"), info.Env().Undefined());
    }

    SdpParseError error;
    auto desc = CreateSessionDescription(*sdpType, sdp, &error);
    if (!desc) {
        RTC_DLOG(LS_WARNING) << "Can't parse received session description message. SdpParseError was: "
                             << error.description;
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    auto observer = rtc::make_ref_counted<SetRemoteSdpObserver>(info.Env());
    pc_->SetRemoteDescription(std::move(desc), observer);

    return observer->GetPromise();
}

Napi::Value NapiPeerConnection::CreateOffer(const CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    PeerConnectionInterface::RTCOfferAnswerOptions options;
    if (info.Length() > 0 && info[0].IsObject()) {
        auto jsOptions = info[0].As<Object>();
        if (jsOptions.Has("iceRestart")) {
            options.ice_restart = jsOptions.Get("iceRestart").As<Boolean>();
        }
    }

    auto observer = rtc::make_ref_counted<CreateSdpObserver>(info.Env());
    pc_->CreateOffer(observer.get(), options);

    return observer->GetPromise();
}

Napi::Value NapiPeerConnection::CreateAnswer(const CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    // ignore the argument, RTCAnswerOptions is empty
    PeerConnectionInterface::RTCOfferAnswerOptions options;

    auto observer = rtc::make_ref_counted<CreateSdpObserver>(info.Env());
    pc_->CreateAnswer(observer.get(), options);

    return observer->GetPromise();
}

Napi::Value NapiPeerConnection::CreateDataChannel(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsString()) {
        NAPI_THROW(TypeError::New(info.Env(), "First argument is not string"), info.Env().Undefined());
    }

    auto label = info[0].As<String>().Utf8Value();

    if (info.Length() < 2) {
        // second parameter is optional
        auto result = pc_->CreateDataChannelOrError(label, nullptr);
        if (!result.ok()) {
            auto& error = result.error();
            RTC_LOG(LS_ERROR) << "create data channel error: " << error.type() << ", " << error.message();
            NAPI_THROW(
                Error::New(info.Env(), error.message() ? error.message() : "unknown error"), info.Env().Undefined());
        }

        auto observer = std::make_unique<DataChannelObserverTemp>(result.value());
        return NapiDataChannel::NewInstance(info.Env(), std::move(observer));
    }

    if (!info[1].IsObject()) {
        NAPI_THROW(TypeError::New(info.Env(), "Second argument is not object"), info.Env().Undefined());
    }

    DataChannelInit options;
    JsToNativeDataChannelInit(info[1].As<Object>(), options);

    auto result = pc_->CreateDataChannelOrError(label, &options);
    if (!result.ok()) {
        auto& error = result.error();
        RTC_LOG(LS_ERROR) << "create data channel error: " << error.type() << ", " << error.message();
        NAPI_THROW(Error::New(info.Env(), error.message() ? error.message() : "unknown error"), info.Env().Undefined());
    }

    auto observer = std::make_unique<DataChannelObserverTemp>(result.value());
    return NapiDataChannel::NewInstance(info.Env(), std::move(observer));
}

Napi::Value NapiPeerConnection::AddIceCandidate(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    std::string sdp;
    std::string sdpMid;
    int sdpMLineIndex = 0;

    if (info.Length() > 0) {
        auto jsCandidate = info[0].As<Object>();
        sdp = jsCandidate.Get("candidate").As<String>().Utf8Value();

        if (jsCandidate.Has("sdpMid")) {
            sdpMid = jsCandidate.Get("sdpMid").As<String>().Utf8Value();
        }

        if (jsCandidate.Has("sdpMLineIndex")) {
            sdpMLineIndex = jsCandidate.Get("sdpMLineIndex").As<Number>().Uint32Value();
        }

        // ignore usernameFragment
    }

    auto deferred = Promise::Deferred::New(info.Env());

    SdpParseError error;
    auto candidate = CreateIceCandidate(sdpMid, sdpMLineIndex, sdp, &error);
    if (!candidate) {
        RTC_LOG(LS_ERROR) << "Can't parse received candidate message. SdpParseError was: " << error.line << ", "
                          << error.description;
        deferred.Reject(Error::New(info.Env(), "Invalid argument").Value());
        return deferred.Promise();
    }

    auto tsfn = ThreadSafeFunction::New(
        info.Env(),
        Function::New(
            info.Env(),
            [deferred](const CallbackInfo& info) {
                auto error = info[0].As<External<RTCError>>().Data();
                if (error->ok()) {
                    deferred.Resolve(info.Env().Undefined());
                } else {
                    auto type = error->type();
                    auto message = error->message();
                    RTC_LOG(LS_ERROR) << "AddIceCandidate failed: " << type << ", " << message;
                    deferred.Reject(
                        Error::New(info.Env(), (message && strlen(message) > 0) ? message : "unknown error").Value());
                }
            }),
        "AddIceCandidate", 0, 1);

    pc_->AddIceCandidate(std::unique_ptr<IceCandidateInterface>(candidate), [tsfn](RTCError error) {
        RTC_DLOG(LS_INFO) << "AddIceCandidate complete: " << error.ok();
        tsfn.BlockingCall([error = new RTCError(std::move(error))](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({External<RTCError>::New(env, error, [](Napi::Env, RTCError* e) { delete e; })});
        });
        tsfn.Release();
    });

    return deferred.Promise();
}

Napi::Value NapiPeerConnection::GetSenders(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto senders = pc_->GetSenders();
    auto jsSenders = Array::New(info.Env(), senders.size());
    for (uint32_t i = 0; i < senders.size(); i++) {
        jsSenders.Set(i, NapiRtpSender::NewInstance(factory_, pc_, senders[i]));
    }

    return jsSenders;
}

Napi::Value NapiPeerConnection::GetReceivers(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto receivers = pc_->GetReceivers();
    auto jsReceivers = Array::New(info.Env(), receivers.size());
    for (uint32_t i = 0; i < receivers.size(); i++) {
        jsReceivers.Set(i, NapiRtpReceiver::NewInstance(factory_, pc_, receivers[i]));
    }

    return jsReceivers;
}

Napi::Value NapiPeerConnection::GetTransceivers(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto transceivers = pc_->GetTransceivers();
    auto jsTransceivers = Array::New(info.Env(), transceivers.size());
    for (uint32_t i = 0; i < transceivers.size(); i++) {
        jsTransceivers.Set(i, NapiRtpTransceiver::NewInstance(factory_, pc_, transceivers[i]));
    }

    return jsTransceivers;
}

Napi::Value NapiPeerConnection::GetConfiguration(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto configuration = pc_->GetConfiguration();
    auto jsConfiguration = Object::New(info.Env());

    if (!NativeToJsConfiguration(configuration, jsConfiguration)) {
        RTC_LOG(LS_ERROR) << "NativeToJsConfiguration failed";
    }

    return jsConfiguration;
}

Napi::Value NapiPeerConnection::SetConfiguration(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    PeerConnectionInterface::RTCConfiguration config;
    if (info.Length() > 0) {
        auto jsConfiguration = info[0].As<Object>();
        if (!JsToNativeConfiguration(jsConfiguration, config)) {
            RTC_LOG(LS_ERROR) << "JsToNativeConfiguration failed";
        }
    }

    auto error = pc_->SetConfiguration(config);
    if (!error.ok()) {
        std::string message;
        switch (error.type()) {
            case RTCErrorType::INVALID_STATE:
                message = "Invalid state";
                break;
            case RTCErrorType::INVALID_MODIFICATION:
                message = "Invalid modification";
                break;
            case RTCErrorType::INVALID_RANGE:
                message = "Invalid range";
                break;
            case RTCErrorType::SYNTAX_ERROR:
                message = "Syntax error";
                break;
            case RTCErrorType::INVALID_PARAMETER:
                message = "Invalid argument";
                break;
            case RTCErrorType::INTERNAL_ERROR:
                message = "Internal error";
                break;
            default:
                message = "Unknown error";
                break;
        }
        NAPI_THROW(Error::New(info.Env(), message), info.Env().Undefined());
    }

    return info.Env().Undefined();
}

Napi::Value NapiPeerConnection::RestartIce(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;
    pc_->RestartIce();
    return info.Env().Undefined();
}

Napi::Value NapiPeerConnection::AddTransceiver(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;
    if (info.Length() < 1) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (!info[0].IsObject() && !info[0].IsString()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
    }

    if (info[0].IsObject()) {
        auto jsTrack = info[0].As<Object>();
        void* unwrapped;
        NAPI_THROW_IF_FAILED(info.Env(), napi_unwrap(info.Env(), jsTrack, &unwrapped), info.Env().Undefined());
        auto track = static_cast<NapiMediaStreamTrack*>(unwrapped);

        if (info.Length() > 1) {
            auto jsInit = info[1].As<Object>();
            webrtc::RtpTransceiverInit init;
            PopulateTransceiverInit(jsInit, init);
            auto result_or_error = pc_->AddTransceiver(track->Get(), init);
            if (!result_or_error.ok()) {
                NAPI_THROW(Error::New(info.Env(), result_or_error.error().message()), info.Env().Undefined());
            }
            return NapiRtpTransceiver::NewInstance(factory_, pc_, result_or_error.value());
        } else {
            auto result_or_error = pc_->AddTransceiver(track->Get());
            if (!result_or_error.ok()) {
                NAPI_THROW(Error::New(info.Env(), result_or_error.error().message()), info.Env().Undefined());
            }
            return NapiRtpTransceiver::NewInstance(factory_, pc_, result_or_error.value());
        }
    } else {
        std::string jsMediatype = info[0].As<String>().Utf8Value();
        cricket::MediaType mediaType;
        if (jsMediatype == "audio") {
            mediaType = cricket::MEDIA_TYPE_AUDIO;
        } else if (jsMediatype == "video") {
            mediaType = cricket::MEDIA_TYPE_VIDEO;
        } else {
            NAPI_THROW(Error::New(info.Env(), "Media type is not audio or video"), info.Env().Undefined());
        }

        if (info.Length() > 1) {
            auto jsInit = info[1].As<Object>();
            webrtc::RtpTransceiverInit init;
            PopulateTransceiverInit(jsInit, init);
            auto result = pc_->AddTransceiver(mediaType, init);
            if (!result.ok()) {
                NAPI_THROW(Error::New(info.Env(), result.error().message()), info.Env().Undefined());
            }
            return NapiRtpTransceiver::NewInstance(factory_, pc_, result.value());
        } else {
            auto result = pc_->AddTransceiver(mediaType);
            if (!result.ok()) {
                NAPI_THROW(Error::New(info.Env(), result.error().message()), info.Env().Undefined());
            }
            return NapiRtpTransceiver::NewInstance(factory_, pc_, result.value());
        }
    }
    NAPI_THROW(Error::New(info.Env(), "Invalid argument type for the first parameter"), info.Env().Undefined());
}

Napi::Value NapiPeerConnection::Close(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    // Close操作可能耗时较长，放到信令线程执行以避免阻塞主线程
    pc_->signaling_thread()->PostTask([pc = pc_] {
        RTC_DLOG(LS_INFO) << "Do Close";
        pc->Close();
    });

    return info.Env().Undefined();
}

Napi::Value NapiPeerConnection::GetStats(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() == 0 || info[0].IsNull() || info[0].IsUndefined()) {
        auto asyncWorker = AsyncWorkerGetStats::Create(info.Env(), "GetStats");
        pc_->GetStats(asyncWorker->GetCallback().get());
        return asyncWorker->GetPromise();
    }

    if (!info[0].IsObject()) {
        auto deferred = Promise::Deferred::New(info.Env());
        deferred.Reject(Error::New(info.Env(), "Invalid argument").Value());
        return deferred.Promise();
    }

    auto napiTrack = NapiMediaStreamTrack::Unwrap(info[0].As<Object>());
    if (!napiTrack) {
        auto deferred = Promise::Deferred::New(info.Env());
        deferred.Reject(Error::New(info.Env(), "Invalid argument").Value());
        return deferred.Promise();
    }

    auto track = napiTrack->Get();
    auto senders = pc_->GetSenders();
    auto receivers = pc_->GetReceivers();

    auto numOfSenders = std::count_if(senders.cbegin(), senders.cend(), [track](const auto& sender) {
        auto t = sender->track();
        return t && t->id() == track->id();
    });
    auto numOfReceivers = std::count_if(receivers.cbegin(), receivers.cend(), [track](const auto& receiver) {
        return receiver->track()->id() == track->id();
    });

    if ((numOfSenders + numOfReceivers) != 1) {
        // reject with InvalidAccessError
        auto deferred = Promise::Deferred::New(info.Env());
        deferred.Reject(Error::New(info.Env(), "Invalid access").Value());
        return deferred.Promise();
    }

    auto asyncWorker = AsyncWorkerGetStats::Create(info.Env(), "GetStats");
    if (numOfSenders == 1) {
        auto sender = std::find_if(senders.cbegin(), senders.cend(), [track](const auto& sender) {
            auto t = sender->track();
            return t && t->id() == track->id();
        });
        pc_->GetStats(*sender, asyncWorker->GetCallback());
    } else {
        auto receiver = std::find_if(receivers.cbegin(), receivers.cend(), [track](const auto& receiver) {
            return receiver->track()->id() == track->id();
        });
        pc_->GetStats(*receiver, asyncWorker->GetCallback());
    }

    return asyncWorker->GetPromise();
}

Napi::Value NapiPeerConnection::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiPeerConnection"));
#endif

    return json;
}

Napi::Value NapiPeerConnection::GenerateCertificate(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto asyncWorker = new AsyncWorkerCertificate(info.Env(), "GenerateCertificateWorker");
    auto deferred = asyncWorker->GetDeferred();

    if (info.Length() < 1 || info[0].IsObject()) {
        deferred.Reject(Error::New(info.Env(), "Invalid argument").Value());
        return asyncWorker->GetPromise();
    }

    rtc::KeyParams key_params;
    auto keyParamsName = info[0].As<Napi::String>().Utf8Value();

    if (keyParamsName == "RSA") {
        key_params = rtc::KeyParams::RSA();
    } else if (keyParamsName == "ECDSA") {
        key_params = rtc::KeyParams::ECDSA();
    } else {
        RTC_DLOG(LS_ERROR) << "Unsupported key algorithm";
    }

    asyncWorker->start(key_params, absl::nullopt);
    return asyncWorker->GetPromise();
}

Napi::Value NapiPeerConnection::SetAudioRecording(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (info.Length() < 1 || !info[0].IsBoolean()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
        return info.Env().Undefined();
    }

    bool recording = info[0].As<Boolean>().Value();
    pc_->SetAudioRecording(recording);

    return info.Env().Undefined();
}

Napi::Value NapiPeerConnection::SetAudioPlayout(const Napi::CallbackInfo& info)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (info.Length() < 1 || !info[0].IsBoolean()) {
        NAPI_THROW(Error::New(info.Env(), "Invalid argument"), info.Env().Undefined());
        return info.Env().Undefined();
    }

    bool playout = info[0].As<Napi::Boolean>();
    pc_->SetAudioPlayout(playout);

    return info.Env().Undefined();
}

void NapiPeerConnection::OnIceCandidate(const IceCandidateInterface* candidate)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!candidate) {
        RTC_LOG(LS_ERROR) << "The candidate is nullptr";
        return;
    }

    std::string sdp;
    if (!candidate->ToString(&sdp)) {
        RTC_LOG(LS_ERROR) << "Failed to convert candidate to string, got so far: " << sdp;
        return;
    }

    auto sdpMid = candidate->sdp_mid();
    auto sdpMLineIndex = candidate->sdp_mline_index();
    auto can = candidate->candidate();

    Dispatch(
        CallbackEvent<NapiPeerConnection>::Create([this, sdp, sdpMid, sdpMLineIndex, can](NapiPeerConnection& target) {
            RTC_DCHECK_EQ(this, &target);

            auto env = target.Env();
            Napi::HandleScope scope(env);
            auto jsEvent = Object::New(env);
            jsEvent.Set("type", String::New(env, kEventIceCandidate));
            jsEvent.Set("candidate", NativeToJsCandidate(env, sdpMid, sdpMLineIndex, sdp, can));
            target.MakeCallback(kEventIceCandidate, {jsEvent});
        }));
}

void NapiPeerConnection::OnIceCandidateError(
    const std::string& address, int port, const std::string& url, int errorCode, const std::string& errorText)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Dispatch(CallbackEvent<NapiPeerConnection>::Create(
        [this, address, port, url, errorCode, errorText](NapiPeerConnection& target) {
            RTC_DCHECK_EQ(this, &target);

            auto env = target.Env();
            Napi::HandleScope scope(env);
            auto jsEvent = Object::New(env);
            jsEvent.Set("type", String::New(env, kEventIceCandidateError));
            jsEvent.Set("address", String::New(env, address));
            jsEvent.Set("port", Number::New(env, port));
            jsEvent.Set("url", String::New(env, url));
            jsEvent.Set("errorCode", Number::New(env, errorCode));
            jsEvent.Set("errorText", String::New(env, errorText));
            target.MakeCallback(kEventIceCandidateError, {jsEvent});
        }));
}

void NapiPeerConnection::OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

void NapiPeerConnection::OnSignalingChange(PeerConnectionInterface::SignalingState newState)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " newState=" << newState;

    Dispatch(CallbackEvent<NapiPeerConnection>::Create([this, newState](NapiPeerConnection& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventSignalingStateChange));
        target.MakeCallback(kEventSignalingStateChange, {jsEvent});

        if (newState == webrtc::PeerConnectionInterface::kClosed) {
            Stop();
        }
    }));
}

void NapiPeerConnection::OnIceConnectionChange(PeerConnectionInterface::IceConnectionState newState)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " newState=" << newState;
    // use OnStandardizedIceConnectionChange
}

void NapiPeerConnection::OnStandardizedIceConnectionChange(PeerConnectionInterface::IceConnectionState newState)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " newState=" << newState;

    Dispatch(CallbackEvent<NapiPeerConnection>::Create([this](NapiPeerConnection& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventIceConnectionStateChange));
        target.MakeCallback(kEventIceConnectionStateChange, {jsEvent});
    }));
}

void NapiPeerConnection::OnConnectionChange(PeerConnectionInterface::PeerConnectionState newState)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " newState=" << newState;

    Dispatch(CallbackEvent<NapiPeerConnection>::Create([this](NapiPeerConnection& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventConnectionStateChange));
        target.MakeCallback(kEventConnectionStateChange, {jsEvent});
    }));
}

void NapiPeerConnection::OnIceConnectionReceivingChange(bool receiving)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

void NapiPeerConnection::OnIceGatheringChange(PeerConnectionInterface::IceGatheringState newState)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " newState=" << newState;

    Dispatch(CallbackEvent<NapiPeerConnection>::Create([this](NapiPeerConnection& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventIceGatheringStateChange));
        target.MakeCallback(kEventIceGatheringStateChange, {jsEvent});
    }));
}

void NapiPeerConnection::OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

void NapiPeerConnection::OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

void NapiPeerConnection::OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface> stream)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

void NapiPeerConnection::OnDataChannel(rtc::scoped_refptr<DataChannelInterface> channel)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!channel) {
        RTC_LOG(LS_ERROR) << "The channel is nullptr";
        return;
    }

    auto observer = std::make_unique<DataChannelObserverTemp>(channel);
    Dispatch(CallbackEvent<NapiPeerConnection>::Create([this, obs = observer.release()](NapiPeerConnection& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventDataChannel));
        jsEvent.Set("channel", NapiDataChannel::NewInstance(env, std::unique_ptr<DataChannelObserverTemp>(obs)));
        target.MakeCallback(kEventDataChannel, {jsEvent});
    }));
}

void NapiPeerConnection::OnRenegotiationNeeded()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Dispatch(CallbackEvent<NapiPeerConnection>::Create([this](NapiPeerConnection& target) {
        RTC_DCHECK_EQ(this, &target);

        auto env = target.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventNegotiationNeeded));
        target.MakeCallback(kEventNegotiationNeeded, {jsEvent});
    }));
}

void NapiPeerConnection::OnNegotiationNeededEvent(uint32_t eventId)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " eventId=" << eventId;
}

void NapiPeerConnection::OnAddTrack(
    rtc::scoped_refptr<RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<MediaStreamInterface>>& streams)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    // use OnTrack
}

void NapiPeerConnection::OnTrack(rtc::scoped_refptr<RtpTransceiverInterface> transceiver)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Dispatch(CallbackEvent<NapiPeerConnection>::Create([this, transceiver](NapiPeerConnection& target) {
        RTC_DCHECK_EQ(this, &target);

        auto receiver = transceiver->receiver();
        if (!receiver) {
            RTC_LOG(LS_ERROR) << "No receiver in the transceiver";
            return;
        }

        auto env = target.Env();
        Napi::HandleScope scope(env);

        auto streams = receiver->streams();
        auto jsStreams = Array::New(env, streams.size());
        for (uint32_t i = 0; i < streams.size(); i++) {
            jsStreams[i] = NapiMediaStream::NewInstance(factory_, streams[i]);
        }

        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventTrack));
        jsEvent.Set("streams", jsStreams);
        jsEvent.Set("track", NapiMediaStreamTrack::NewInstance(factory_, receiver->track()));
        jsEvent.Set("receiver", NapiRtpReceiver::NewInstance(factory_, pc_, receiver));
        jsEvent.Set("transceiver", NapiRtpTransceiver::NewInstance(factory_, pc_, transceiver));

        target.MakeCallback(kEventTrack, {jsEvent});
    }));
}

void NapiPeerConnection::OnRemoveTrack(rtc::scoped_refptr<RtpReceiverInterface> receiver)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

} // namespace webrtc
