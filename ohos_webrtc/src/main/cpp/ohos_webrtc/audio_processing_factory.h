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

#ifndef WEBRTC_AUDIO_PROCESSING_FACTORY_H
#define WEBRTC_AUDIO_PROCESSING_FACTORY_H

#include "utils/marcos.h"

#include "modules/audio_processing/include/audio_processing.h"

#include "napi.h"

namespace webrtc {

class NapiAudioProcessing : public Napi::ObjectWrap<NapiAudioProcessing> {
public:
    NAPI_CLASS_NAME_DECLARE(AudioProcessing);
    NAPI_METHOD_NAME_DECLARE(ToJson, toJSON);

    static void Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(Napi::Env env, rtc::scoped_refptr<AudioProcessing> audioProcessing);

    rtc::scoped_refptr<AudioProcessing> Get() const;

protected:
    friend class ObjectWrap;
    explicit NapiAudioProcessing(const Napi::CallbackInfo& info);

    Napi::Value ToJson(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;

    rtc::scoped_refptr<AudioProcessing> audioProcessing_;
};

class NapiAudioProcessingFactory : public Napi::ObjectWrap<NapiAudioProcessingFactory> {
public:
    NAPI_CLASS_NAME_DECLARE(AudioProcessingFactory);
    NAPI_METHOD_NAME_DECLARE(Create, create);
    NAPI_METHOD_NAME_DECLARE(ToJson, toJSON);

    static void Init(Napi::Env env, Napi::Object exports);

protected:
    friend class ObjectWrap;
    explicit NapiAudioProcessingFactory(const Napi::CallbackInfo& info);

    Napi::Value Create(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;
};

} // namespace webrtc

#endif // WEBRTC_AUDIO_PROCESSING_FACTORY_H
