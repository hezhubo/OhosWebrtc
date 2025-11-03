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

#ifndef WEBRTC_DATA_CHANNEL_H
#define WEBRTC_DATA_CHANNEL_H

#include <map>
#include <atomic>
#include <mutex>

#include "napi.h"

#include "api/data_channel_interface.h"

#include "event/event_target.h"

namespace webrtc {

class NapiDataChannel;

class DataChannelObserverTemp : public EventQueue<NapiDataChannel>, public DataChannelObserver {
public:
    explicit DataChannelObserverTemp(rtc::scoped_refptr<DataChannelInterface> dataChannel);
    ~DataChannelObserverTemp() override;

    rtc::scoped_refptr<DataChannelInterface> Get() const
    {
        return dataChannel_;
    }

protected:
    void OnStateChange() override;
    void OnMessage(const DataBuffer& buffer) override;
    void OnBufferedAmountChange(uint64_t sentDataSize) override;

private:
    rtc::scoped_refptr<DataChannelInterface> dataChannel_{};
};

class NapiDataChannel : public NapiEventTarget<NapiDataChannel>, public DataChannelObserver {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    static Napi::Object NewInstance(Napi::Env env, std::unique_ptr<DataChannelObserverTemp> observer);

    explicit NapiDataChannel(const Napi::CallbackInfo& info);
    ~NapiDataChannel() override;

    rtc::scoped_refptr<DataChannelInterface> Get() const;

protected:
    // Js
    Napi::Value GetLabel(const Napi::CallbackInfo& info);
    Napi::Value GetOrdered(const Napi::CallbackInfo& info);
    Napi::Value GetMaxPacketLifeTime(const Napi::CallbackInfo& info);
    Napi::Value GetMaxRetransmits(const Napi::CallbackInfo& info);
    Napi::Value GetProtocol(const Napi::CallbackInfo& info);
    Napi::Value GetNegotiated(const Napi::CallbackInfo& info);
    Napi::Value GetId(const Napi::CallbackInfo& info);
    Napi::Value GetReadyState(const Napi::CallbackInfo& info);
    Napi::Value GetBufferedAmount(const Napi::CallbackInfo& info);
    Napi::Value GetBufferedAmountLowThreshold(const Napi::CallbackInfo& info);
    void SetBufferedAmountLowThreshold(const Napi::CallbackInfo& info, const Napi::Value& value);
    Napi::Value GetBinaryType(const Napi::CallbackInfo& info);
    void SetBinaryType(const Napi::CallbackInfo& info, const Napi::Value& value);

    Napi::Value GetEventHandler(const Napi::CallbackInfo& info);
    void SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value);

    Napi::Value Close(const Napi::CallbackInfo& info);
    Napi::Value Send(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

protected:
    friend class DataChannelObserverTemp;

    void OnStateChange() override;
    void OnMessage(const DataBuffer& buffer) override;
    void OnBufferedAmountChange(uint64_t sentDataSize) override;

    void HandleStateChange(DataChannelInterface::DataState state);
    void HandleMessage(const DataBuffer& buffer);

private:
    static Napi::FunctionReference constructor_;

    std::unique_ptr<DataChannelObserverTemp> observerTemp_;
    rtc::scoped_refptr<DataChannelInterface> dataChannel_;

    std::string binaryType_;
    std::atomic<uint64_t> bufferedAmountLowThreshold_{0};
};

void JsToNativeDataChannelInit(const Napi::Object& jsDataChannelInit, DataChannelInit& init);

} // namespace webrtc

#endif // WEBRTC_DATA_CHANNEL_H
