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

#ifndef WEBRTC_VIDEO_CODEC_DEFAULT_VIDEO_DECODER_FACTORY_H
#define WEBRTC_VIDEO_CODEC_DEFAULT_VIDEO_DECODER_FACTORY_H

#include "../render/egl_context.h"

#include "api/video_codecs/video_decoder.h"
#include "api/video_codecs/video_decoder_factory.h"

namespace webrtc {
namespace adapter {

class DefaultVideoDecoderFactory : public VideoDecoderFactory {
public:
    explicit DefaultVideoDecoderFactory(std::shared_ptr<EglContext> sharedEglContext);

    explicit DefaultVideoDecoderFactory(std::unique_ptr<VideoDecoderFactory> hardwareVideoDecoderFactory);

    std::vector<SdpVideoFormat> GetSupportedFormats() const override;

    std::unique_ptr<VideoDecoder> CreateVideoDecoder(const SdpVideoFormat& format) override;

private:
    std::unique_ptr<VideoDecoderFactory> hardwareVideoDecoderFactory_;
    std::unique_ptr<VideoDecoderFactory> softwareVideoDecoderFactory_;
};

} // namespace adapter
} // namespace webrtc

#endif // WEBRTC_VIDEO_CODEC_DEFAULT_VIDEO_DECODER_FACTORY_H
