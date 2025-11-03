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
#ifndef OH_WEB_RTC_OHOS_DESKTOP_H
#define OH_WEB_RTC_OHOS_DESKTOP_H
#include <cstdint>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "api/video/video_frame.h"
#include "api/video/video_source_interface.h"
#include <multimedia/player_framework/native_avscreen_capture.h>
#include <multimedia/player_framework/native_avscreen_capture_base.h>
#include <native_image/native_image.h>
#include <thread>

#include "commom/ohos_video_buffer.h"
#include "surface_helper/egl_render_context.h"


struct BufferData {
    uint8_t *buffer;
    int32_t length;
    int32_t stride;
    int32_t width;
    int32_t height;
    int64_t timestamp;
};

namespace webrtc {
namespace ohos {

class OhosDesktop {
public:
    enum class CaptureType {
        SURFACE = 0,
        BUFFER
    };
    int32_t Create(CaptureType captureType = CaptureType::SURFACE, int32_t queueSize = 1);
    int32_t Config(int32_t width = 1200, int32_t height = 2592);
    int32_t Start();
    int32_t Stop();

    void RegisterCaptureDataCallback(rtc::VideoSinkInterface<webrtc::VideoFrame> *dataCallback);
    void UnregisterCaptureDataCallback();
    void DesktopBufferAvailable();
    void DesktopBufferOn(uint8_t *buffer,int32_t bufferLength, int32_t stride, int32_t width, int32_t height,
                         int64_t timestamp);
    OhosDesktop() = default;
    ~OhosDesktop();

private:
    int32_t Release();
    static void OnError(OH_AVScreenCapture *capture, int32_t errorCode, void *userData);
    static void OnStateChange(struct OH_AVScreenCapture *capture, OH_AVScreenCaptureStateCode stateCode,
                              void *userData);
    // 录屏回调: surface模式时OnBufferAvailable不触发
    static void OnBufferAvailable(OH_AVScreenCapture *capture, OH_AVBuffer *buffer,
                                  OH_AVScreenCaptureBufferType bufferType, int64_t timestamp, void *userData);
    void SetStateCode(OH_AVScreenCaptureStateCode stateCode);
    rtc::VideoSinkInterface<webrtc::VideoFrame> *dataCallback_ {nullptr};
    OH_AVScreenCapture *capture_{nullptr};
    OH_AVScreenCaptureStateCode stateCode_{OH_SCREEN_CAPTURE_STATE_CANCELED};
    bool isStarted_{false};
    bool isConfig_{false};
    std::atomic_bool isRunning_{false};
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<BufferData> desktopAVBufferQueue_;
    size_t maxQueueSize_{1};
    CaptureType captureType_{CaptureType::SURFACE};
    std::unique_ptr<std::thread> bufferAvailableThread_{nullptr};

    // RenderContext(EglContext EglDisplay)创建和销毁
    bool InitRenderContext();
    int32_t DestroyRenderContext();
    std::unique_ptr<EglRenderContext> renderContext_{nullptr};

    // NativeWindow创建和销毁
    bool CreateNativeWindow();
    int32_t DestroyNativeWindow();
    OHNativeWindow *nativeWindow_{nullptr};

    // NativeImage创建和销毁，需要使用eglSurface,textureID
    bool CreateNativeImage();
    int32_t DestroyNativeImage();
    OH_NativeImage *nativeImage_{nullptr};
    GLuint nativeImageTextureID_{0U};
    EGLSurface eglSurface_{EGL_NO_SURFACE};

    // NativeImage回调函数
    static void OnNativeImageFrameAvailable(void *data);
    int32_t FrameAvailable();
    OH_OnFrameAvailableListener nativeImageFrameAvailableListener_{nullptr, nullptr};

    int32_t width_{0};
    int32_t height_{0};
};

} // namespace ohos
} // namespace webrtc
#endif //OH_WEB_RTC_OHOS_DESKTOP_H
