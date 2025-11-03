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

#include "hardware_video_decoder_factory.h"
#include "hardware_video_decoder.h"
#include "media_codec_utils.h"
#include "video_codec_mime_type.h"

#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>

#include <api/video_codecs/h264_profile_level_id.h>
#include <modules/video_coding/codecs/h264/include/h264.h>
#include <rtc_base/logging.h>

namespace webrtc {
namespace adapter {

HardwareVideoDecoderFactory::HardwareVideoDecoderFactory(std::shared_ptr<EglContext> sharedContext)
    : sharedContext_(sharedContext)
{
}

HardwareVideoDecoderFactory::~HardwareVideoDecoderFactory() {}

std::vector<SdpVideoFormat> HardwareVideoDecoderFactory::GetSupportedFormats() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    std::vector<SdpVideoFormat> supportedFormats;

    for (const VideoCodecMimeType type : {VideoCodecMimeType::H264, VideoCodecMimeType::H265}) {
        OH_AVCapability* capability = OH_AVCodec_GetCapabilityByCategory(type.mimeType(), false, HARDWARE);
        if (!capability) {
            RTC_LOG(LS_WARNING) << "No capability for mime type: " << type.mimeType();
            continue;
        }

        const char* name = OH_AVCapability_GetName(capability);
        RTC_DLOG(LS_VERBOSE) << "capability codec name: " << name;

        uint32_t pixelFormatNum = 0;
        const int32_t* pixelFormats = nullptr;
        OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormats, &pixelFormatNum);
        for (uint32_t i = 0; i < pixelFormatNum; i++) {
            RTC_DLOG(LS_VERBOSE) << "supported pixel format: " << pixelFormats[i];
        }

        if (type == VideoCodecMimeType::H264) {
            if (OH_AVCapability_AreProfileAndLevelSupported(capability, AVC_PROFILE_HIGH, AVC_LEVEL_31)) {
                supportedFormats.push_back(
                    CreateH264Format(H264Profile::kProfileConstrainedHigh, H264Level::kLevel3_1, "1"));
            }
            supportedFormats.push_back(
                CreateH264Format(H264Profile::kProfileConstrainedBaseline, H264Level::kLevel3_1, "1"));
        } else {
            supportedFormats.push_back(SdpVideoFormat(type.name()));
        }
    }

    RTC_DLOG(LS_VERBOSE) << "supported formats: " << supportedFormats.size();

    return supportedFormats;
}

std::unique_ptr<VideoDecoder> HardwareVideoDecoderFactory::CreateVideoDecoder(const SdpVideoFormat& format)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    RTC_DLOG(LS_VERBOSE) << "format: " << format.ToString();

    VideoCodecMimeType type = VideoCodecMimeType::valueOf(format.name);
    OH_AVCapability* capability = OH_AVCodec_GetCapabilityByCategory(type.mimeType(), false, HARDWARE);
    if (!capability) {
        return nullptr;
    }

    const char* codecName = OH_AVCapability_GetName(capability);
    RTC_DLOG(LS_VERBOSE) << "codec name: " << codecName;

    std::optional<int32_t> pixelFormat =
        ohos::MediaCodecUtils::SelectPixelFormat(ohos::MediaCodecUtils::DECODER_PIXEL_FORMATS, capability);
    if (!pixelFormat) {
        RTC_LOG(LS_ERROR) << "No supported pixel format";
        return nullptr;
    }
    RTC_DLOG(LS_VERBOSE) << "selected pixel format: " << *pixelFormat;

    return HardwareVideoDecoder::Create(codecName, *pixelFormat, format, sharedContext_);
}

} // namespace adapter
} // namespace webrtc
