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

#include "data_channel.h"
#include "utils/marcos.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

const char kEnumBinaryTypeBlob[] = "blob";
const char kEnumBinaryTypeArrayBuffer[] = "arraybuffer";

const char kEnumDataChannelStateClosed[] = "closed";
const char kEnumDataChannelStateClosing[] = "closing";
const char kEnumDataChannelStateConnecting[] = "connecting";
const char kEnumDataChannelStateOpen[] = "open";

const char kClassName[] = "RTCDataChannel";

const char kAttributeNameLabel[] = "label";
const char kAttributeNameOrdered[] = "ordered";
const char kAttributeNameMaxPacketLifeTime[] = "maxPacketLifeTime";
const char kAttributeNameMaxRetransmits[] = "maxRetransmits";
const char kAttributeNameProtocol[] = "protocol";
const char kAttributeNameNegotiated[] = "negotiated";
const char kAttributeNameId[] = "id";
const char kAttributeNameReadyState[] = "readyState";
const char kAttributeNameBufferedAmount[] = "bufferedAmount";
const char kAttributeNameBufferedAmountLowThreshold[] = "bufferedAmountLowThreshold";
const char kAttributeNameBinaryType[] = "binaryType";
const char kAttributeNameOnBufferedAmountLow[] = "onbufferedamountlow";
const char kAttributeNameOnClose[] = "onclose";
const char kAttributeNameOnClosing[] = "onclosing";
const char kAttributeNameOnOpen[] = "onopen";
const char kAttributeNameOnMessage[] = "onmessage";
const char kAttributeNameOnError[] = "onerror";

const char kMethodNameClose[] = "close";
const char kMethodNameSend[] = "send";
const char kMethodNameToJson[] = "toJSON";

const char kEventNameBufferedAmountLow[] = "bufferedamountlow";
const char kEventNameClose[] = "close";
const char kEventNameClosing[] = "closing";
const char kEventNameOpen[] = "open";
const char kEventNameMessage[] = "message";
const char kEventNameError[] = "error";

DataChannelObserverTemp::DataChannelObserverTemp(rtc::scoped_refptr<DataChannelInterface> dataChannel)
    : dataChannel_(std::move(dataChannel))
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    dataChannel_->RegisterObserver(this);
}

DataChannelObserverTemp::~DataChannelObserverTemp()
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    dataChannel_->UnregisterObserver();
}

void DataChannelObserverTemp::OnStateChange()
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto state = dataChannel_->state();
    Enqueue(
        CallbackEvent<NapiDataChannel>::Create([state](NapiDataChannel& target) { target.HandleStateChange(state); }));
}

void DataChannelObserverTemp::OnMessage(const DataBuffer& buffer)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Enqueue(
        CallbackEvent<NapiDataChannel>::Create([buffer](NapiDataChannel& channel) { channel.HandleMessage(buffer); }));
}

void DataChannelObserverTemp::OnBufferedAmountChange(uint64_t sentDataSize)
{
    // should not reached
    (void)sentDataSize;
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;
}

FunctionReference NapiDataChannel::constructor_;

void NapiDataChannel::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiDataChannel::GetLabel>(kAttributeNameLabel),
            InstanceAccessor<&NapiDataChannel::GetOrdered>(kAttributeNameOrdered),
            InstanceAccessor<&NapiDataChannel::GetMaxPacketLifeTime>(kAttributeNameMaxPacketLifeTime),
            InstanceAccessor<&NapiDataChannel::GetMaxRetransmits>(kAttributeNameMaxRetransmits),
            InstanceAccessor<&NapiDataChannel::GetProtocol>(kAttributeNameProtocol),
            InstanceAccessor<&NapiDataChannel::GetNegotiated>(kAttributeNameNegotiated),
            InstanceAccessor<&NapiDataChannel::GetId>(kAttributeNameId),
            InstanceAccessor<&NapiDataChannel::GetReadyState>(kAttributeNameReadyState),
            InstanceAccessor<&NapiDataChannel::GetBufferedAmount>(kAttributeNameBufferedAmount),
            InstanceAccessor<
                &NapiDataChannel::GetBufferedAmountLowThreshold, &NapiDataChannel::SetBufferedAmountLowThreshold>(
                kAttributeNameBufferedAmountLowThreshold),
            InstanceAccessor<&NapiDataChannel::GetBinaryType, &NapiDataChannel::SetBinaryType>(
                kAttributeNameBinaryType),
            InstanceAccessor<&NapiDataChannel::GetEventHandler, &NapiDataChannel::SetEventHandler>(
                kAttributeNameOnBufferedAmountLow, napi_default, (void*)kEventNameBufferedAmountLow),
            InstanceAccessor<&NapiDataChannel::GetEventHandler, &NapiDataChannel::SetEventHandler>(
                kAttributeNameOnClose, napi_default, (void*)kEventNameClose),
            InstanceAccessor<&NapiDataChannel::GetEventHandler, &NapiDataChannel::SetEventHandler>(
                kAttributeNameOnClosing, napi_default, (void*)kEventNameClosing),
            InstanceAccessor<&NapiDataChannel::GetEventHandler, &NapiDataChannel::SetEventHandler>(
                kAttributeNameOnOpen, napi_default, (void*)kEventNameOpen),
            InstanceAccessor<&NapiDataChannel::GetEventHandler, &NapiDataChannel::SetEventHandler>(
                kAttributeNameOnMessage, napi_default, (void*)kEventNameMessage),
            InstanceAccessor<&NapiDataChannel::GetEventHandler, &NapiDataChannel::SetEventHandler>(
                kAttributeNameOnError, napi_default, (void*)kEventNameError),
            InstanceMethod<&NapiDataChannel::Close>(kMethodNameClose),
            InstanceMethod<&NapiDataChannel::Send>(kMethodNameSend),
            InstanceMethod<&NapiDataChannel::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

Napi::Object NapiDataChannel::NewInstance(Napi::Env env, std::unique_ptr<DataChannelObserverTemp> observer)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto externalObserver = External<DataChannelObserverTemp>::New(env, observer.release());
    return constructor_.New({externalObserver});
}

NapiDataChannel::NapiDataChannel(const CallbackInfo& info)
    : NapiEventTarget<NapiDataChannel>(info), binaryType_(kEnumBinaryTypeBlob)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (!info.IsConstructCall()) {
        NAPI_THROW_VOID(TypeError::New(info.Env(), "Use the new operator to construct the RTCDataChannel"));
    }

    if (info.Length() < 1 || !info[0].IsExternal()) {
        NAPI_THROW_VOID(TypeError::New(info.Env(), "Invalid argument"));
    }

    auto observer = std::unique_ptr<DataChannelObserverTemp>(info[0].As<External<DataChannelObserverTemp>>().Data());
    dataChannel_ = observer->Get();
    dataChannel_->RegisterObserver(this);

    RTC_DLOG(LS_VERBOSE) << "State: " << dataChannel_->state();

    // redispatch queued events
    while (auto event = observer->Dequeue()) {
        Dispatch(std::move(event));
    }

    // Hold the observer to avoid:
    // 1. Early 'UnregisterObserver'
    // 2. The gap of 'UnregisterObserver' and 'RegisterObserver'
    observerTemp_ = std::move(observer);
}

NapiDataChannel::~NapiDataChannel() {}

rtc::scoped_refptr<DataChannelInterface> NapiDataChannel::Get() const
{
    return dataChannel_;
}

// readonly label: string;
Napi::Value NapiDataChannel::GetLabel(const CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return String::New(info.Env(), dataChannel_->label());
}

// readonly ordered : boolean;
Napi::Value NapiDataChannel::GetOrdered(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return Boolean::New(info.Env(), dataChannel_->ordered());
}

// readonly maxPacketLifeTime ?: number;
Napi::Value NapiDataChannel::GetMaxPacketLifeTime(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    if (dataChannel_->maxPacketLifeTime()) {
        return Number::New(info.Env(), dataChannel_->maxPacketLifeTime().value());
    }
    return info.Env().Undefined();
}

// readonly maxRetransmits ?: number;
Napi::Value NapiDataChannel::GetMaxRetransmits(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    if (dataChannel_->maxRetransmitsOpt()) {
        return Number::New(info.Env(), dataChannel_->maxRetransmitsOpt().value());
    }
    return info.Env().Undefined();
}

// readonly protocol : string;
Napi::Value NapiDataChannel::GetProtocol(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return String::New(info.Env(), dataChannel_->protocol());
}

// readonly negotiated : boolean;
Napi::Value NapiDataChannel::GetNegotiated(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return Boolean::New(info.Env(), dataChannel_->negotiated());
}

// readonly id ?: number;
Napi::Value NapiDataChannel::GetId(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    if (dataChannel_->id() != -1) {
        return Number::New(info.Env(), dataChannel_->id());
    }
    return info.Env().Undefined();
}

// readonly readyState : DataChannelState;
Napi::Value NapiDataChannel::GetReadyState(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    switch (dataChannel_->state()) {
        case DataChannelInterface::kConnecting:
            return String::New(info.Env(), kEnumDataChannelStateConnecting);
        case DataChannelInterface::kOpen:
            return String::New(info.Env(), kEnumDataChannelStateOpen);
        case DataChannelInterface::kClosing:
            return String::New(info.Env(), kEnumDataChannelStateClosing);
        case DataChannelInterface::kClosed:
            return String::New(info.Env(), kEnumDataChannelStateClosed);
        default:
            break;
    }

    NAPI_THROW(Error::New(info.Env(), "Invalid state"), info.Env().Undefined());
}

// readonly bufferedAmount : number;
Napi::Value NapiDataChannel::GetBufferedAmount(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return Number::New(info.Env(), dataChannel_->buffered_amount());
}

// bufferedAmountLowThreshold : number;
Napi::Value NapiDataChannel::GetBufferedAmountLowThreshold(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return Number::New(info.Env(), bufferedAmountLowThreshold_.load());
}

void NapiDataChannel::SetBufferedAmountLowThreshold(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!value.IsNumber()) {
        NAPI_THROW_VOID(TypeError::New(info.Env(), "The argument is not number"));
    }

    auto bufferedAmountLowThreshold = value.As<Number>().Int64Value();
    if (bufferedAmountLowThreshold < 0) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Invalid argument"));
    }

    bufferedAmountLowThreshold_.store(bufferedAmountLowThreshold);
}

// binaryType : BinaryType;
Napi::Value NapiDataChannel::GetBinaryType(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return String::New(info.Env(), binaryType_);
}

void NapiDataChannel::SetBinaryType(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!value.IsString()) {
        NAPI_THROW_VOID(TypeError::New(info.Env(), "The argument is not string"));
    }

    auto binaryType = value.As<String>().Utf8Value();
    if (binaryType != kEnumBinaryTypeBlob && binaryType != kEnumBinaryTypeArrayBuffer) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Invalid argument"));
    }

    binaryType_ = binaryType;
}

Napi::Value NapiDataChannel::GetEventHandler(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    Function func;
    if (!NapiEventTarget<NapiDataChannel>::GetEventHandler(type, func)) {
        return info.Env().Null();
    }

    return func;
}

void NapiDataChannel::SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    if (value.IsFunction()) {
        auto fn = value.As<Function>();
        NapiEventTarget<NapiDataChannel>::SetEventHandler(type, fn);
    } else if (value.IsNull()) {
        RemoveEventHandler(type);
    } else {
        NAPI_THROW_VOID(Error::New(info.Env(), "value is error"));
    }
}

Napi::Value NapiDataChannel::Close(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    dataChannel_->Close();
    return info.Env().Undefined();
}

Napi::Value NapiDataChannel::Send(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (info.Length() == 0) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    if (dataChannel_->state() != DataChannelInterface::kOpen) {
        NAPI_THROW(Error::New(info.Env(), "Datachannel state is not open"), info.Env().Undefined());
    }

    if (info[0].IsString()) {
        RTC_DLOG(LS_VERBOSE) << "argument is string";
        auto jsStr = info[0].As<String>();
        dataChannel_->SendAsync(DataBuffer(jsStr.Utf8Value()), [&](RTCError err) {
            if (!err.ok()) {
                RTC_LOG(LS_ERROR) << "send array buffer error: " << err.type() << ", " << err.message();
            }
        });
    } else if (info[0].IsArrayBuffer()) {
        RTC_DLOG(LS_VERBOSE) << "argument is array buffer";
        auto jsArrayBuffer = info[0].As<ArrayBuffer>();

        auto data = jsArrayBuffer.Data();
        auto size = jsArrayBuffer.ByteLength();

        dataChannel_->SendAsync(DataBuffer(rtc::CopyOnWriteBuffer((uint8_t*)data, size), true), [&](RTCError err) {
            if (!err.ok()) {
                RTC_LOG(LS_ERROR) << "send array buffer error: " << err.type() << ", " << err.message();
            }
        });
    } else {
        RTC_LOG(LS_WARNING) << "unknown type of argument";
    }

    return info.Env().Undefined();
}

Napi::Value NapiDataChannel::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", "NapiDataChannel");
#endif
    json.Set(kAttributeNameLabel, GetLabel(info));
    json.Set(kAttributeNameId, GetId(info));
    json.Set(kAttributeNameProtocol, GetProtocol(info));
    json.Set(kAttributeNameOrdered, GetOrdered(info));

    return json;
}

void NapiDataChannel::OnStateChange()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto state = dataChannel_->state();
    Dispatch(CallbackEvent<NapiDataChannel>::Create(
        [state](NapiDataChannel& channel) { channel.HandleStateChange(state); }));
}

void NapiDataChannel::OnMessage(const DataBuffer& buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Dispatch(
        CallbackEvent<NapiDataChannel>::Create([buffer](NapiDataChannel& channel) { channel.HandleMessage(buffer); }));
}

void NapiDataChannel::OnBufferedAmountChange(uint64_t sentDataSize)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    (void)sentDataSize;

    auto bufferedAmount = dataChannel_->buffered_amount();
    auto bufferedAmountLowThreshold = bufferedAmountLowThreshold_.load();
    if (bufferedAmount > bufferedAmountLowThreshold) {
        RTC_LOG(LS_VERBOSE) << "Buffered amount is greater than threshold: " << bufferedAmount << " > "
                            << bufferedAmountLowThreshold;
        return;
    }

    Dispatch(CallbackEvent<NapiDataChannel>::Create([](NapiDataChannel& channel) {
        RTC_LOG(LS_VERBOSE) << "Dispatched: " << kEventNameBufferedAmountLow;

        auto env = channel.Env();
        Napi::HandleScope scope(env);
        auto jsEvent = Object::New(env);
        jsEvent.Set("type", String::New(env, kEventNameBufferedAmountLow));
        channel.MakeCallback(kEventNameBufferedAmountLow, {jsEvent});
    }));
}

void NapiDataChannel::HandleStateChange(DataChannelInterface::DataState state)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    static std::map<DataChannelInterface::DataState, std::string_view> STATE_EVENT_MAP = {
        {DataChannelInterface::kOpen, kEventNameOpen},
        {DataChannelInterface::kClosing, kEventNameClosing},
        {DataChannelInterface::kClosed, kEventNameClose},
    };

    if (state == DataChannelInterface::kOpen) {
        auto curState = dataChannel_->state();
        if (curState == DataChannelInterface::kClosing || curState == DataChannelInterface::kClosed) {
            // abort, see https://www.w3.org/TR/webrtc/#announcing-a-data-channel-as-open.
            return;
        }
    }

    auto eventType = STATE_EVENT_MAP[state];

    HandleScope scope(Env());

    auto object = Object::New(Env());
    object.Set("type", String::New(Env(), eventType.data()));
    MakeCallback(eventType.data(), {object});

    if (state == webrtc::DataChannelInterface::kClosed) {
        Stop();
    }
}

void NapiDataChannel::HandleMessage(const DataBuffer& buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    HandleScope scope(Env());

    Napi::Value value;
    if (buffer.binary) {
        auto externalData = new rtc::CopyOnWriteBuffer(buffer.data);
        auto arrayBuffer = ArrayBuffer::New(
            Env(), static_cast<void*>(externalData->MutableData()), externalData->size(),
            [](Napi::Env /*env*/, void* /*data*/, rtc::CopyOnWriteBuffer* hint) {
                RTC_DLOG(LS_VERBOSE) << "release rtc::CopyOnWriteBuffer";
                delete hint;
            },
            externalData);
        value = arrayBuffer;
    } else {
        // Should be a UTF-8 string
        auto str = String::New(Env(), reinterpret_cast<const char*>(buffer.data.data()), buffer.size());
        value = str;
    }

    auto jsEvent = Object::New(Env());
    jsEvent.Set("type", kEventNameMessage);
    jsEvent.Set("data", value);

    MakeCallback(kEventNameMessage, {jsEvent});
}

void JsToNativeDataChannelInit(const Napi::Object& jsDataChannelInit, DataChannelInit& init)
{
    if (jsDataChannelInit.Has(kAttributeNameOrdered)) {
        auto jsOrdered = jsDataChannelInit.Get(kAttributeNameOrdered);
        if (jsOrdered.IsBoolean()) {
            init.ordered = jsOrdered.As<Boolean>().Value();
        }
    }

    if (jsDataChannelInit.Has(kAttributeNameMaxPacketLifeTime)) {
        auto jsMaxPacketLifeTime = jsDataChannelInit.Get(kAttributeNameMaxPacketLifeTime);
        if (jsMaxPacketLifeTime.IsNumber()) {
            init.maxRetransmitTime = jsMaxPacketLifeTime.As<Number>().Uint32Value();
        }
    }

    if (jsDataChannelInit.Has(kAttributeNameMaxRetransmits)) {
        auto jsMaxRetransmits = jsDataChannelInit.Get(kAttributeNameMaxRetransmits);
        if (jsMaxRetransmits.IsNumber()) {
            init.maxRetransmits = jsMaxRetransmits.As<Number>().Uint32Value();
        }
    }

    if (jsDataChannelInit.Has(kAttributeNameProtocol)) {
        auto jsProtocol = jsDataChannelInit.Get(kAttributeNameProtocol);
        if (jsProtocol.IsString()) {
            init.protocol = jsProtocol.As<String>().Utf8Value();
        }
    }

    if (jsDataChannelInit.Has(kAttributeNameNegotiated)) {
        auto jsNegotiated = jsDataChannelInit.Get(kAttributeNameNegotiated);
        if (jsNegotiated.IsBoolean()) {
            init.negotiated = jsNegotiated.As<Boolean>().Value();
        }
    }

    if (jsDataChannelInit.Has(kAttributeNameId)) {
        auto jsId = jsDataChannelInit.Get(kAttributeNameId);
        if (jsId.IsNumber()) {
            init.id = jsId.As<Number>().Int32Value();
        }
    }
}

} // namespace webrtc
