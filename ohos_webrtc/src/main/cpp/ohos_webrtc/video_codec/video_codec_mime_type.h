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

#ifndef WEBRTC_VIDEO_CODEC_VIDEO_CODEC_MIME_TYPE_H
#define WEBRTC_VIDEO_CODEC_VIDEO_CODEC_MIME_TYPE_H

#include <string_view>

namespace webrtc {

struct VideoCodecMimeType {
    struct VideoCodecMimeTypeInit {
        std::string_view name;
        std::string_view mimeType;
        constexpr operator VideoCodecMimeType() const
        {
            return VideoCodecMimeType{name, mimeType};
        }
    };

    static constexpr VideoCodecMimeTypeInit VP8{"VP8", "video/x-vnd.on2.vp8"};
    static constexpr VideoCodecMimeTypeInit VP9{"VP9", "video/x-vnd.on2.vp9"};
    static constexpr VideoCodecMimeTypeInit AV1{"AV1", "video/av01"};
    static constexpr VideoCodecMimeTypeInit H264{"H264", "video/avc"};
    static constexpr VideoCodecMimeTypeInit H265{"H265", "video/hevc"};

    constexpr VideoCodecMimeType(std::string_view name, std::string_view mimeType) : name_{name}, mimeType_(mimeType) {}

    const char* name() const
    {
        return name_.data();
    }

    const char* mimeType() const
    {
        return mimeType_.data();
    }

    static VideoCodecMimeType valueOf(const std::string_view name)
    {
        if (name == VP8.name) {
            return VP8;
        } else if (name == VP9.name) {
            return VP9;
        } else if (name == AV1.name) {
            return AV1;
        } else if (name == H264.name) {
            return H264;
        } else if (name == H265.name) {
            return H265;
        }

        return VideoCodecMimeType("", "");
    }

    constexpr bool operator==(const VideoCodecMimeType& other) const noexcept
    {
        return name_ == other.name_ && mimeType_ == other.mimeType_;
    }

private:
    std::string_view name_;
    std::string_view mimeType_;
};

} // namespace webrtc

#endif // WEBRTC_VIDEO_CODEC_VIDEO_CODEC_MIME_TYPE_H
