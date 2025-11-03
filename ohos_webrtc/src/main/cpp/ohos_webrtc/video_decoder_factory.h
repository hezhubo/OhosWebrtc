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

#ifndef WEBRTC_VIDEO_CODEC_VIDEO_DECODER_FACTORY_H
#define WEBRTC_VIDEO_CODEC_VIDEO_DECODER_FACTORY_H

#include "render/egl_env.h"
#include "utils/marcos.h"

#include <napi.h>

#include <api/video_codecs/video_decoder_factory.h>

namespace webrtc {

class NapiHardwareVideoDecoderFactory : public Napi::ObjectWrap<NapiHardwareVideoDecoderFactory> {
public:
    NAPI_CLASS_NAME_DECLARE(HardwareVideoDecoderFactory);
    NAPI_ATTRIBUTE_NAME_DECLARE(SharedContext, sharedContext);
    NAPI_METHOD_NAME_DECLARE(ToJson, toJSON);
    NAPI_TYPE_TAG_DECLARE(0xd8917b4837764a46, 0xb69d705ec2e65b37);

    static void Init(Napi::Env env, Napi::Object exports);

    ~NapiHardwareVideoDecoderFactory();

    std::shared_ptr<EglContext> GetSharedContext()
    {
        return sharedContext_;
    }

protected:
    friend class ObjectWrap;

    explicit NapiHardwareVideoDecoderFactory(const Napi::CallbackInfo& info);

    Napi::Value GetSharedContext(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;

    std::shared_ptr<EglContext> sharedContext_;
};

class NapiSoftwareVideoDecoderFactory : public Napi::ObjectWrap<NapiSoftwareVideoDecoderFactory> {
public:
    NAPI_CLASS_NAME_DECLARE(SoftwareVideoDecoderFactory);
    NAPI_METHOD_NAME_DECLARE(ToJson, toJSON);
    NAPI_TYPE_TAG_DECLARE(0x6c59721271134289, 0xa4a93236ff9897df);

    static void Init(Napi::Env env, Napi::Object exports);

    ~NapiSoftwareVideoDecoderFactory();

protected:
    friend class ObjectWrap;

    explicit NapiSoftwareVideoDecoderFactory(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;
};

std::unique_ptr<VideoDecoderFactory> CreateVideoDecoderFactory(const Napi::Object& jsVideoDecoderFactory);

std::unique_ptr<VideoDecoderFactory> CreateDefaultVideoDecoderFactory();

} // namespace webrtc

#endif // WEBRTC_VIDEO_CODEC_VIDEO_DECODER_FACTORY_H
