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

#include "software_video_encoder_factory.h"

#include "api/video_codecs/video_encoder_factory_template.h"
#include "api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h"
#ifdef WEBRTC_USE_H264
#include <api/video_codecs/video_encoder_factory_template_open_h264_adapter.h>
#endif

namespace webrtc {
namespace adapter {

namespace {

using BuiltinVideoEncoderFactory = VideoEncoderFactoryTemplate<
#ifdef WEBRTC_USE_H264
    OpenH264EncoderTemplateAdapter,
#endif
    LibvpxVp8EncoderTemplateAdapter, LibaomAv1EncoderTemplateAdapter, LibvpxVp9EncoderTemplateAdapter>;

} // namespace

SoftwareVideoEncoderFactory::SoftwareVideoEncoderFactory() : internal_(std::make_unique<BuiltinVideoEncoderFactory>())
{
}

std::vector<SdpVideoFormat> SoftwareVideoEncoderFactory::GetSupportedFormats() const
{
    return internal_->GetSupportedFormats();
}

std::unique_ptr<VideoEncoder> SoftwareVideoEncoderFactory::CreateVideoEncoder(const SdpVideoFormat& format)
{
    auto original_format = FuzzyMatchSdpVideoFormat(internal_->GetSupportedFormats(), format);
    return original_format ? internal_->CreateVideoEncoder(*original_format) : nullptr;
}

VideoEncoderFactory::CodecSupport SoftwareVideoEncoderFactory::QueryCodecSupport(
    const SdpVideoFormat& format, absl::optional<std::string> scalability_mode) const
{
    auto original_format = FuzzyMatchSdpVideoFormat(internal_->GetSupportedFormats(), format);
    return original_format ? internal_->QueryCodecSupport(*original_format, scalability_mode)
                           : VideoEncoderFactory::CodecSupport{.is_supported = false};
}

} // namespace adapter
} // namespace webrtc
