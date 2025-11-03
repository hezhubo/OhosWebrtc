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

#ifndef WEBRTC_VIDEO_CODEC_MEDIA_CODEC_UTILS_H
#define WEBRTC_VIDEO_CODEC_MEDIA_CODEC_UTILS_H

#include <vector>
#include <optional>

#include <multimedia/player_framework/native_avcodec_base.h>
#include <multimedia/player_framework/native_avcapability.h>

namespace ohos {

class MediaCodecUtils {
public:
    // OH_AVPixelFormat
    static const std::vector<int32_t> DECODER_PIXEL_FORMATS;
    static const std::vector<int32_t> ENCODER_PIXEL_FORMATS;

    static std::optional<int32_t>
    SelectPixelFormat(const std::vector<int32_t>& pixelFormats, OH_AVCapability* capability);
};

} // namespace ohos

#endif // WEBRTC_VIDEO_CODEC_MEDIA_CODEC_UTILS_H
