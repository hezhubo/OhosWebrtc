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

#ifndef WEBRTC_SCTP_TRANSPORT_H
#define WEBRTC_SCTP_TRANSPORT_H

#include "napi.h"

#include "api/sctp_transport_interface.h"

#include "event/event_target.h"

namespace webrtc {

class PeerConnectionFactoryWrapper;

class NapiSctpTransport : public NapiEventTarget<NapiSctpTransport>, public SctpTransportObserverInterface {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(
        std::shared_ptr<PeerConnectionFactoryWrapper> factory, rtc::scoped_refptr<SctpTransportInterface> transport);

    ~NapiSctpTransport() override;

    explicit NapiSctpTransport(const Napi::CallbackInfo& info);

    rtc::scoped_refptr<SctpTransportInterface> Get() const;

protected:
    // JS
    Napi::Value GetMaxChannels(const Napi::CallbackInfo& info);
    Napi::Value GetMaxMessageSize(const Napi::CallbackInfo& info);
    Napi::Value GetState(const Napi::CallbackInfo& info);
    Napi::Value GetTransport(const Napi::CallbackInfo& info);

    Napi::Value GetEventHandler(const Napi::CallbackInfo& info);
    void SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value);

    Napi::Value ToJson(const Napi::CallbackInfo& info);
    void OnStateChange(SctpTransportInformation info) override;

private:
    static Napi::FunctionReference constructor_;

    std::shared_ptr<PeerConnectionFactoryWrapper> factory_;
    rtc::scoped_refptr<SctpTransportInterface> sctpTransport_;
};

} // namespace webrtc

#endif // WEBRTC_SCTP_TRANSPORT_H
