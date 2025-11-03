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
#ifndef OH_WEB_RTC_OHOS_VIDEO_BUFFER_H
#define OH_WEB_RTC_OHOS_VIDEO_BUFFER_H

#include <GLES3/gl31.h>
#include "api/scoped_refptr.h"
#include "api/video/video_frame_buffer.h"

namespace webrtc {
namespace ohos {

class YuvConverter;

class OHOSVideoBuffer : public VideoFrameBuffer {
public:
    enum class VideoSourceType : uint8_t {
        CAMERA = 0,
        DESKTOP
    };

    struct TextureBuffer {
        GLuint textureID{0};
        enum class Type : uint8_t {
            OES,
            RGB,
            YUV
        };
        Type type{Type::OES};
        float matrix[16]{1.0,  0.0, 0.0, 0.0,
                         0.0, -1.0, 0.0, 0.0,
                         0.0,  0.0, 1.0, 0.0,
                         0.0,  0.0, 0.0, 1.0};;
        int yuvTexture[3]{0, 0, 0};
    };
    
    static rtc::scoped_refptr<OHOSVideoBuffer> Creat(int width,
                                                     int height, const TextureBuffer& textureBuffer,
                                                     VideoSourceType sourceType);

    rtc::scoped_refptr<VideoFrameBuffer> CropAndScale(int crop_x, int crop_y, int crop_width, int crop_height,
                                                      int scale_width, int scale_height) override;

    TextureBuffer GetVideoFrameBuffer() const;
    VideoSourceType GetSourceType();
    
    ~OHOSVideoBuffer() = default;
    int width() const override;
    int height() const override;
    rtc::scoped_refptr<I420BufferInterface> ToI420() override;
    void SetConverter(std::shared_ptr<YuvConverter> converter);
protected:
    OHOSVideoBuffer(int width, int height, const TextureBuffer& textureBuffer, VideoSourceType sourceType);
private:
    Type type() const override;

    std::shared_ptr<YuvConverter> converter_;
    int width_{0};
    int height_{0};
    VideoSourceType sourceType_{VideoSourceType::CAMERA};
    TextureBuffer textureBuffer_;
};

}   // namespace ohos
}   // namepsace webrtc


#endif //OH_WEB_RTC_OHOS_VIDEO_BUFFER_H
