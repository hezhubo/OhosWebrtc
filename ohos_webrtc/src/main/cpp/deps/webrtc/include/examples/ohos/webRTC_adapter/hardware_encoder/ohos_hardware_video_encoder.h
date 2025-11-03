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
#ifndef OH_WEB_RTC_OHOS_HARDWARE_VIDEO_ENCODER_H
#define OH_WEB_RTC_OHOS_HARDWARE_VIDEO_ENCODER_H

#include "api/video_codecs/video_encoder.h"
#include "media/base/codec.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "hardware_encoder/ohos_video_encoder.h"
#include <thread>
#include "ohos_video_common.h"
#include <src/codec/api/wels/codec_api.h>
#include "surface_helper/egl_render_context.h"
#include "surface_helper/ohos_gl_drawer.h"
#include "surface_helper/ohos_eglContext_manage.h"

namespace webrtc {
namespace ohos {

class OhosHardwareVideoEncoder : public H264Encoder {
public:
    explicit OhosHardwareVideoEncoder(const cricket::VideoCodec& codec);
    ~OhosHardwareVideoEncoder();
    int32_t InitEncode(const VideoCodec *codec_settings, int32_t number_of_cores, size_t max_payload_size) override;
    int InitEncode(const VideoCodec *codec_settings, const VideoEncoder::Settings &settings) override;
    int32_t RegisterEncodeCompleteCallback(EncodedImageCallback *callback) override;
    int32_t Encode(const VideoFrame& frame, const std::vector<VideoFrameType>* frame_types) override;
    void SetRates(const RateControlParameters &parameters) override;
    int32_t Release() override;
    EncoderInfo GetEncoderInfo() const override;
    static std::unique_ptr<OhosHardwareVideoEncoder> Create(const cricket::VideoCodec &codec);
    static std::unique_ptr<OhosHardwareVideoEncoder> Create();
    void SetEncoderInfo();
private:
    void OutputThread();
    int32_t EncodeTextureBuffer(const VideoFrame &frame);
    int32_t EncodeByteBuffer(const VideoFrame &frame);
    bool SurfeceModeInit(OHOSVideoBuffer::VideoSourceType type);

    std::unique_ptr<OhosVideoEncoder> encoder_;
    std::unique_ptr<std::thread> outputThread_;
    int64_t nextPTS_{0};
    double fps_{0};
    EncodedImageCallback* encoded_image_callback_{nullptr};
    std::vector<ISVCEncoder*> encoders_;
    CodecData codecData_;
    FormatInfo formatInfo_;

    EncoderInfo encoderInfo_;
    std::unique_ptr<EglRenderContext> eglRenderContext_;
    std::unique_ptr<OhosGLDrawer> glDrawer_;
    EGLSurface eglSurface_{EGL_NO_SURFACE};
    std::atomic<bool> running_{false};
    uint32_t width_{0};
    uint32_t height_{0};
    // 用于给编码器动态配置比特率
    uint32_t adjustedBitrate_{0};
    uint32_t curBitrate_{0};
    std::shared_ptr<YuvConverter> converter_;
    float matrix_[16]{1.0,  0.0, 0.0, 0.0,
                      0.0, -1.0, 0.0, 0.0,
                      0.0,  0.0, 1.0, 0.0,
                      0.0,  0.0, 0.0, 1.0};
};
}
}
#endif //OH_WEB_RTC_OHOS_HARDWARE_VIDEO_ENCODER_H
