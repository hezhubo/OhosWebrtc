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

#ifndef WEBRTC_VIDEO_BUFFER_RECEIVER_GL_H
#define WEBRTC_VIDEO_BUFFER_RECEIVER_GL_H

#include "texture_buffer.h"
#include "video_frame_receiver.h"
#include "../helper/native_image.h"
#include "../render/egl_env.h"

#include <cstdint>
#include <memory>

#include <GLES3/gl3.h>

#include "rtc_base/thread.h"

namespace webrtc {

class VideoFrameReceiverGl : public VideoFrameReceiver {
public:
    static std::unique_ptr<VideoFrameReceiverGl>
    Create(const std::string& threadName, std::shared_ptr<EglContext> sharedContext);

    // Do not use this constructor directly, use 'Create' above.
    VideoFrameReceiverGl(const std::string& threadName, std::shared_ptr<EglContext> sharedContext);
    ~VideoFrameReceiverGl() override;

    uint64_t GetSurfaceId() const override;
    void SetVideoFrameSize(int32_t width, int32_t height) override;

protected:
    bool CreateNativeImage();
    void ReleaseNativeImage();
    void SetTextureSize(int textureWidth, int textureHeight);

    static void OnNativeImageFrameAvailable1(void* data);
    void OnNativeImageFrameAvailable();

private:
    std::unique_ptr<rtc::Thread> thread_;
    std::unique_ptr<EglEnv> eglEnv_;
    uint32_t width_{0};
    uint32_t height_{0};
    ohos::NativeImage nativeImage_;
    std::shared_ptr<TextureData> textureData_;
    std::unique_ptr<YuvConverter> yuvConverter_;
};

} // namespace webrtc

#endif // WEBRTC_VIDEO_BUFFER_RECEIVER_GL_H
