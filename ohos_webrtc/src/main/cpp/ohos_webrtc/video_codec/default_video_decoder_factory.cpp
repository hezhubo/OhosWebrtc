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

#include "default_video_decoder_factory.h"
#include "hardware_video_decoder_factory.h"
#include "software_video_decoder_factory.h"

#include <api/video_codecs/video_decoder_software_fallback_wrapper.h>
#include <rtc_base/logging.h>

namespace webrtc {
namespace adapter {

DefaultVideoDecoderFactory::DefaultVideoDecoderFactory(std::shared_ptr<EglContext> sharedContext)
    : DefaultVideoDecoderFactory(std::make_unique<HardwareVideoDecoderFactory>(sharedContext))
{
}

DefaultVideoDecoderFactory::DefaultVideoDecoderFactory(std::unique_ptr<VideoDecoderFactory> hardwareVideoDecoderFactory)
    : hardwareVideoDecoderFactory_(std::move(hardwareVideoDecoderFactory)),
      softwareVideoDecoderFactory_(std::make_unique<SoftwareVideoDecoderFactory>())
{
}

std::vector<SdpVideoFormat> DefaultVideoDecoderFactory::GetSupportedFormats() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto hardwareSupportedFormats = hardwareVideoDecoderFactory_->GetSupportedFormats();
    auto softwareSupportedFormats = softwareVideoDecoderFactory_->GetSupportedFormats();

    std::vector<SdpVideoFormat> supportedFormats;
    supportedFormats.insert(supportedFormats.end(), hardwareSupportedFormats.begin(), hardwareSupportedFormats.end());
    supportedFormats.insert(supportedFormats.end(), softwareSupportedFormats.begin(), softwareSupportedFormats.end());

    RTC_DLOG(LS_VERBOSE) << "Supported formats: ";
    for (auto& format : supportedFormats) {
        RTC_DLOG(LS_VERBOSE) << "\t format: " << format.ToString();
    }

    return supportedFormats;
}

std::unique_ptr<VideoDecoder> DefaultVideoDecoderFactory::CreateVideoDecoder(const SdpVideoFormat& format)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto hardwareDecoder = hardwareVideoDecoderFactory_->CreateVideoDecoder(format);
    auto softwareDecoder = softwareVideoDecoderFactory_->CreateVideoDecoder(format);
    if (hardwareDecoder && softwareDecoder) {
        // Both hardware and software supported, wrap it in a software fallback
        return CreateVideoDecoderSoftwareFallbackWrapper(std::move(softwareDecoder), std::move(hardwareDecoder));
    }

    return hardwareDecoder ? std::move(hardwareDecoder) : std::move(softwareDecoder);
}

} // namespace adapter
} // namespace webrtc
