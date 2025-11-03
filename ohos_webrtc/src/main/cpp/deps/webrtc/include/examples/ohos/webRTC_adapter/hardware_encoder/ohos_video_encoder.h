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
#ifndef OH_WEB_RTC_OHOS_VIDEO_ENCODER_H
#define OH_WEB_RTC_OHOS_VIDEO_ENCODER_H

#include "hardware_encoder/ohos_video_common.h"
#include "ohos_codec_data.h"

#include <multimedia/player_framework/native_avcodec_videoencoder.h>
#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>
#include <multimedia/player_framework/native_avformat.h>

namespace webrtc {
namespace ohos {

class OhosVideoEncoder {
public:
    enum class EncodeStat {
        NONE = 0,
        CONFIGUED,
        STOP,
        RUNNING,
        RELEASE,
    };
    OhosVideoEncoder() = default;
    ~OhosVideoEncoder();
    int32_t Create();
    int32_t Config(CodecData *codecData);
    int32_t Start();
    int32_t PushInputData(uint32_t bufferIndex);
    int32_t FreeOutPutData(uint32_t bufferIndex);
    int32_t Stop();
    void Release();
    const EncodeStat &Stat();
    bool CreatNativeWindow(int32_t width, int32_t height);
    OHNativeWindow *GetNativeWindow();
    bool IsHardware() const;
    OH_AVRange GetBitrateRange() const;
    const StrideInfo &GetEncoderStride();
    uint32_t UpdateBitrate(uint32_t bitrate);
private:
    int32_t SetCallback(CodecData *codecData);
    int32_t Configure(FormatInfo *formatInfo);
    bool NativeWindowDestroy();
    OH_AVCodec *encoder_{nullptr};
    EncodeStat stat_{EncodeStat::NONE};
    OHNativeWindow *nativeWindow_{nullptr};
    bool isHardware_{false};
    OH_AVRange bitrateRange_{0, 0};
    StrideInfo encoderStride_;
};

}
}

#endif //OH_WEB_RTC_OHOS_VIDEO_ENCODER_H