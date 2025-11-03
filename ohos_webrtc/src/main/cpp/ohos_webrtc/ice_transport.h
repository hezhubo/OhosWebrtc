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

#ifndef WEBRTC_ICE_TRANSPORT_H
#define WEBRTC_ICE_TRANSPORT_H

#include <map>

#include "napi.h"

#include "api/ice_transport_interface.h"
#include "p2p/base/ice_transport_internal.h"

#include "event/event_target.h"
#include "ice_candidate.h"

namespace webrtc {

class PeerConnectionFactoryWrapper;

class NapiIceTransport : public NapiEventTarget<NapiIceTransport>, public sigslot::has_slots<> {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(
        Napi::Env env, std::shared_ptr<PeerConnectionFactoryWrapper> factory,
        rtc::scoped_refptr<IceTransportInterface> iceTransport);

    ~NapiIceTransport() override;

protected:
    friend class ObjectWrap;

    explicit NapiIceTransport(const Napi::CallbackInfo& info);

protected:
    // JS
    Napi::Value GetRole(const Napi::CallbackInfo& info);
    Napi::Value GetComponent(const Napi::CallbackInfo& info);
    Napi::Value GetState(const Napi::CallbackInfo& info);
    Napi::Value GetGatheringState(const Napi::CallbackInfo& info);

    Napi::Value GetEventHandler(const Napi::CallbackInfo& info);
    void SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value);

    Napi::Value GetLocalCandidates(const Napi::CallbackInfo& info);
    Napi::Value GetLocalParameters(const Napi::CallbackInfo& info);
    Napi::Value GetRemoteCandidates(const Napi::CallbackInfo& info);
    Napi::Value GetRemoteParameters(const Napi::CallbackInfo& info);
    Napi::Value GetSelectedCandidatePair(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

    void OnStateChange(cricket::IceTransportInternal* iceTransport);
    void OnGatheringStateChange(cricket::IceTransportInternal* iceTransport);
    void OnSelectedCandidatePairChange(const cricket::CandidatePairChangeEvent& event);

private:
    static Napi::FunctionReference constructor_;

    std::shared_ptr<PeerConnectionFactoryWrapper> factory_;
    rtc::scoped_refptr<IceTransportInterface> iceTransport_;

    std::atomic<IceTransportState> iceTransportState_{IceTransportState::kNew};
    std::atomic<cricket::IceGatheringState> iceGatheringState_{cricket::kIceGatheringNew};
};

} // namespace webrtc

#endif // WEBRTC_ICE_TRANSPORT_H
