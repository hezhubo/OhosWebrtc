/*
# Copyright (c) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/
#ifndef OH_WEB_RTC_OHOS_VIDEO_COMMON_H
#define OH_WEB_RTC_OHOS_VIDEO_COMMON_H
#include <multimedia/player_framework/native_avcodec_videoencoder.h>
#include <multimedia/player_framework/native_avbuffer_info.h>
#include <multimedia/player_framework/native_avformat.h>
#include <string>
#include "commom/ohos_video_buffer.h"

namespace webrtc {
namespace ohos {

struct StrideInfo {
    int32_t wStride{0};
    int32_t hStride{0};
};

struct FormatInfo {
    std::string codecMime;
    int32_t videoWidth{0};
    int32_t videoHeight{0};
    double frameRate{30.0};
    int64_t bitrate{3000000};
    OH_AVPixelFormat pixelFormat{AV_PIXEL_FORMAT_YUVI420};
    bool rangeFlag{false};
    int32_t profile{OH_AVCProfile::AVC_PROFILE_BASELINE};
    int32_t rateMode{OH_VideoEncodeBitrateMode::CBR};
    int32_t frameInterval{0};
    int32_t qpMax{-1};
    int32_t qpMin{-1};
};

class CodecBufferInfo {
public:
    CodecBufferInfo() = default;
    CodecBufferInfo(const CodecBufferInfo &codecBufferInfo) = default;
    CodecBufferInfo& operator=(const CodecBufferInfo &codecBufferInfo) = default;
    explicit CodecBufferInfo(uint32_t argBufferIndex, OH_AVBuffer *argBuffer);
    explicit CodecBufferInfo(OHOSVideoBuffer::TextureBuffer tBuffer);
    explicit CodecBufferInfo(const OH_AVCodecBufferAttr &attr);
    ~CodecBufferInfo();
    void Destory();
    int32_t GetBufferIndex();
    void SetBufferIndex(int32_t index);
    uint8_t *GetBuff();
    OH_AVBuffer *GetAVBuff();
    int32_t SetAttr(OH_AVCodecBufferAttr *attr);
    int32_t GetAttr(OH_AVCodecBufferAttr *attr);
private:
    bool createAVBuff_{false};
    int32_t bufferIndex_{-1};
    OH_AVBuffer *buff_{nullptr};
};

}
}

#endif //OH_WEB_RTC_OHOS_VIDEO_COMMON_H
