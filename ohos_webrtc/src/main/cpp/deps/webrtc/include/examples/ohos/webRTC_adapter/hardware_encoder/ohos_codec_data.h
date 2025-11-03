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
#ifndef OH_WEB_RTC_OHOS_CODEC_DATA_H
#define OH_WEB_RTC_OHOS_CODEC_DATA_H

#include <mutex>
#include <queue>
#include "ohos_video_common.h"

namespace webrtc {
namespace ohos {

class CodecData {
public:
    ~CodecData();
    FormatInfo *formatInfo_{nullptr};
    std::atomic<bool> isRunning_{false};
    void Start();
    void ShutDown();
    struct I420Info {
        const uint8_t* srcY{nullptr};
        int32_t srcStrideY{0};
        const uint8_t* srcU{nullptr};
        int32_t srcStrideU{0};
        const uint8_t* srcV{nullptr};
        int32_t srcStrideV{0};
        int32_t width{0};
        int32_t height{0};
    };
    class DataCallback {
    public:
        static void OnCodecError(OH_AVCodec *codec, int32_t errorCode, void *userData);
        static void OnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
        static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
        static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    };
    uint32_t inputFrameCount_{0};
    uint32_t outputFrameCount_{0};
    int32_t InputData(const CodecData::I420Info &i420Info, int32_t &bufferIndex, const StrideInfo &encoderStride,
        OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAGS_CODEC_DATA},
        std::chrono::milliseconds = std::chrono::milliseconds(10));
    int32_t OutputData(CodecBufferInfo &info);
private:
    inline int32_t I420Copy(const uint8_t* srcY, int srcStrideY, const uint8_t* srcU, int srcStrideU,
                            const uint8_t* srcV, int srcStrideV, uint8_t* dstY, int dstStrideY, uint8_t* dstU,
                            int dstStrideU, uint8_t* dstV, int dstStrideV, int width, int height);
    inline void CopyPlane(const uint8_t* srcData, int srcStride, uint8_t* dstData, int dstStride,
                          int width, int height);
    mutable std::mutex inputMutex_;
    std::condition_variable inputCond_;
    std::queue<CodecBufferInfo> inputBufferInfoQueue_;
    mutable std::mutex outputMutex_;
    std::condition_variable outputCond_;
    std::queue<CodecBufferInfo> outputBufferInfoQueue_;
};

}
}

#endif //OH_WEB_RTC_OHOS_CODEC_DATA_H
