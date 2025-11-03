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

#ifndef WEBRTC_RTP_TRANSCEIVER_H
#define WEBRTC_RTP_TRANSCEIVER_H

#include "napi.h"

#include "api/rtp_transceiver_interface.h"
#include "api/peer_connection_interface.h"

namespace webrtc {

class PeerConnectionFactoryWrapper;

class NapiRtpTransceiver : public Napi::ObjectWrap<NapiRtpTransceiver> {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(
        std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<PeerConnectionInterface> pc,
        rtc::scoped_refptr<RtpTransceiverInterface> transceiver);

    ~NapiRtpTransceiver() override;

    rtc::scoped_refptr<RtpTransceiverInterface> Get() const;

protected:
    friend class ObjectWrap;
    explicit NapiRtpTransceiver(const Napi::CallbackInfo& info);

    Napi::Value GetMid(const Napi::CallbackInfo& info);
    Napi::Value GetSender(const Napi::CallbackInfo& info);
    Napi::Value GetReceiver(const Napi::CallbackInfo& info);
    Napi::Value GetDirection(const Napi::CallbackInfo& info);
    void SetDirection(const Napi::CallbackInfo& info, const Napi::Value& value);
    Napi::Value GetCurrentDirection(const Napi::CallbackInfo& info);

    Napi::Value Stop(const Napi::CallbackInfo& info);
    Napi::Value SetCodecPreferences(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;

    std::shared_ptr<PeerConnectionFactoryWrapper> factory_;
    rtc::scoped_refptr<PeerConnectionInterface> pc_;
    rtc::scoped_refptr<RtpTransceiverInterface> rtpTransceiver_;
};

void PopulateTransceiverInit(const Napi::Object& jsInit, webrtc::RtpTransceiverInit& init);

} // namespace webrtc

#endif // WEBRTC_RTP_TRANSCEIVER_H
