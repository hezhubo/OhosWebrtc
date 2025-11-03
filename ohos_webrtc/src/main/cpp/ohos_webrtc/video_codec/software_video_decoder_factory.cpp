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

#include "software_video_decoder_factory.h"

#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template_dav1d_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h"
#ifdef WEBRTC_USE_H264
#include <api/video_codecs/video_decoder_factory_template_open_h264_adapter.h>
#endif

namespace webrtc {
namespace adapter {

namespace {

using BuiltinVideoDecoderFactory = VideoDecoderFactoryTemplate<
#ifdef WEBRTC_USE_H264
    OpenH264DecoderTemplateAdapter,
#endif
    LibvpxVp8DecoderTemplateAdapter, LibvpxVp9DecoderTemplateAdapter, Dav1dDecoderTemplateAdapter>;

} // namespace

SoftwareVideoDecoderFactory::SoftwareVideoDecoderFactory() : internal_(std::make_unique<BuiltinVideoDecoderFactory>())
{
}

std::vector<SdpVideoFormat> SoftwareVideoDecoderFactory::GetSupportedFormats() const
{
    return internal_->GetSupportedFormats();
}

std::unique_ptr<VideoDecoder> SoftwareVideoDecoderFactory::CreateVideoDecoder(const SdpVideoFormat& format)
{
    auto original_format = FuzzyMatchSdpVideoFormat(internal_->GetSupportedFormats(), format);
    return original_format ? internal_->CreateVideoDecoder(*original_format) : nullptr;
}

VideoDecoderFactory::CodecSupport
SoftwareVideoDecoderFactory::QueryCodecSupport(const SdpVideoFormat& format, bool reference_scaling) const
{
    auto original_format = FuzzyMatchSdpVideoFormat(internal_->GetSupportedFormats(), format);
    return original_format ? internal_->QueryCodecSupport(*original_format, reference_scaling)
                           : VideoDecoderFactory::CodecSupport{.is_supported = false};
}

} // namespace adapter
} // namespace webrtc
