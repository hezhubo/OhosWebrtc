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

#include "media_codec_utils.h"

#include <multimedia/player_framework/native_avformat.h>

namespace ohos {

const std::vector<int32_t> MediaCodecUtils::DECODER_PIXEL_FORMATS{
    AV_PIXEL_FORMAT_RGBA,
    AV_PIXEL_FORMAT_YUVI420,
    AV_PIXEL_FORMAT_NV12,
    AV_PIXEL_FORMAT_NV21,
};

const std::vector<int32_t> MediaCodecUtils::ENCODER_PIXEL_FORMATS{
    AV_PIXEL_FORMAT_RGBA,
    AV_PIXEL_FORMAT_YUVI420,
    AV_PIXEL_FORMAT_NV12,
    AV_PIXEL_FORMAT_NV21,
};

std::optional<int32_t>
MediaCodecUtils::SelectPixelFormat(const std::vector<int32_t>& supportedPixelFormats, OH_AVCapability* capability)
{
    if (capability) {
        const int32_t* pixelFormats = nullptr;
        uint32_t pixelFormatNum = 0;
        OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormats, &pixelFormatNum);

        for (int32_t supportedColorFormat : supportedPixelFormats) {
            for (uint32_t i = 0; i < pixelFormatNum; i++) {
                if (supportedColorFormat == pixelFormats[i]) {
                    return supportedColorFormat;
                }
            }
        }
    }

    return std::nullopt;
}

} // namespace ohos
