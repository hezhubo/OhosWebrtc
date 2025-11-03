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

#ifndef WEBRTC_DTMF_SENDER_H
#define WEBRTC_DTMF_SENDER_H

#include "napi.h"

#include "api/dtmf_sender_interface.h"
#include "event/event_target.h"

namespace webrtc {

class NapiDtmfSender : public NapiEventTarget<NapiDtmfSender>, public DtmfSenderObserverInterface {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(Napi::Env env, rtc::scoped_refptr<DtmfSenderInterface> dtmfSender);

    ~NapiDtmfSender() override;

protected:
    friend class ObjectWrap;

    explicit NapiDtmfSender(const Napi::CallbackInfo& info);

protected:
    // JS
    Napi::Value GetCanInsertDTMF(const Napi::CallbackInfo& info);
    Napi::Value GetToneBuffer(const Napi::CallbackInfo& info);

    Napi::Value GetEventHandler(const Napi::CallbackInfo& info);
    void SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value);

    Napi::Value InsertDTMF(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

    void OnToneChange(const std::string& tone, const std::string& tone_buffer) override;

private:
    static Napi::FunctionReference constructor_;

    rtc::scoped_refptr<DtmfSenderInterface> dtmfSender_;
};

} // namespace webrtc

#endif // WEBRTC_DTMF_SENDER_H
