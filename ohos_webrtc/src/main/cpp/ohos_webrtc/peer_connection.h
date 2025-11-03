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

#ifndef WEBRTC_PEER_CONNECTION_H
#define WEBRTC_PEER_CONNECTION_H

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "napi.h"

#include "api/peer_connection_interface.h"

#include "event/event_target.h"

namespace webrtc {

class PeerConnectionFactoryWrapper;

class NapiPeerConnection : public NapiEventTarget<NapiPeerConnection>, public PeerConnectionObserver {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Value NewInstance(Napi::Value configuration, std::shared_ptr<PeerConnectionFactoryWrapper> factory);

    ~NapiPeerConnection() override;

protected:
    friend class ObjectWrap;
    explicit NapiPeerConnection(const Napi::CallbackInfo& info);

    Napi::Value GetCanTrickleIceCandidates(const Napi::CallbackInfo& info);
    Napi::Value GetSignalingState(const Napi::CallbackInfo& info);
    Napi::Value GetIceGatheringState(const Napi::CallbackInfo& info);
    Napi::Value GetIceConnectionState(const Napi::CallbackInfo& info);
    Napi::Value GetConnectionState(const Napi::CallbackInfo& info);
    Napi::Value GetLocalDescription(const Napi::CallbackInfo& info);
    Napi::Value GetRemoteDescription(const Napi::CallbackInfo& info);
    Napi::Value GetCurrentLocalDescription(const Napi::CallbackInfo& info);
    Napi::Value GetCurrentRemoteDescription(const Napi::CallbackInfo& info);
    Napi::Value GetPendingLocalDescription(const Napi::CallbackInfo& info);
    Napi::Value GetPendingRemoteDescription(const Napi::CallbackInfo& info);
    Napi::Value GetSctp(const Napi::CallbackInfo& info);

    Napi::Value GetEventHandler(const Napi::CallbackInfo& info);
    void SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value);

    Napi::Value AddTrack(const Napi::CallbackInfo& info);
    Napi::Value RemoveTrack(const Napi::CallbackInfo& info);
    Napi::Value SetLocalDescription(const Napi::CallbackInfo& info);
    Napi::Value SetRemoteDescription(const Napi::CallbackInfo& info);
    Napi::Value CreateOffer(const Napi::CallbackInfo& info);
    Napi::Value CreateAnswer(const Napi::CallbackInfo& info);
    Napi::Value CreateDataChannel(const Napi::CallbackInfo& info);
    Napi::Value AddIceCandidate(const Napi::CallbackInfo& info);
    Napi::Value GetSenders(const Napi::CallbackInfo& info);
    Napi::Value GetReceivers(const Napi::CallbackInfo& info);
    Napi::Value GetTransceivers(const Napi::CallbackInfo& info);
    Napi::Value GetConfiguration(const Napi::CallbackInfo& info);
    Napi::Value SetConfiguration(const Napi::CallbackInfo& info);
    Napi::Value RestartIce(const Napi::CallbackInfo& info);
    Napi::Value AddTransceiver(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);
    Napi::Value GetStats(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);
    Napi::Value SetAudioRecording(const Napi::CallbackInfo& info);
    Napi::Value SetAudioPlayout(const Napi::CallbackInfo& info);

    static Napi::Value GenerateCertificate(const Napi::CallbackInfo& info);

protected:
    void OnIceCandidate(const IceCandidateInterface* candidate) override;
    void OnIceCandidateError(
        const std::string& address, int port, const std::string& url, int errorCode,
        const std::string& errorText) override;
    void OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) override;
    void OnSignalingChange(PeerConnectionInterface::SignalingState newState) override;
    void OnIceConnectionChange(PeerConnectionInterface::IceConnectionState newState) override;
    void OnStandardizedIceConnectionChange(PeerConnectionInterface::IceConnectionState newState) override;
    void OnConnectionChange(PeerConnectionInterface::PeerConnectionState newState) override;
    void OnIceConnectionReceivingChange(bool receiving) override;
    void OnIceGatheringChange(PeerConnectionInterface::IceGatheringState newState) override;
    void OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event) override;
    void OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream) override;
    void OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface> stream) override;
    void OnDataChannel(rtc::scoped_refptr<DataChannelInterface> channel) override;
    void OnRenegotiationNeeded() override;
    void OnNegotiationNeededEvent(uint32_t eventId) override;
    void OnAddTrack(
        rtc::scoped_refptr<RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<MediaStreamInterface>>& streams) override;
    void OnTrack(rtc::scoped_refptr<RtpTransceiverInterface> transceiver) override;
    void OnRemoveTrack(rtc::scoped_refptr<RtpReceiverInterface> receiver) override;

private:
    static Napi::FunctionReference constructor_;

    std::shared_ptr<PeerConnectionFactoryWrapper> factory_;
    rtc::scoped_refptr<PeerConnectionInterface> pc_;

    Napi::ObjectReference sctpTransportRef_;
};

} // namespace webrtc

#endif // WEBRTC_PEER_CONNECTION_H
