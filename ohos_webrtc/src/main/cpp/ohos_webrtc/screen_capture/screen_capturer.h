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

#ifndef WEBRTC_SCREEN_CAPTURER_H
#define WEBRTC_SCREEN_CAPTURER_H

#include "screen_capture_options.h"
#include "../video/video_info.h"
#include "../video/video_capturer.h"
#include "../video/video_frame_receiver.h"
#include "../helper/screen_capture.h"
#include "../helper/display_manager.h"

#include <memory>
#include <cstdint>
#include <mutex>

#include <api/video/video_rotation.h>
#include "api/video/video_frame_buffer.h"
#include "rtc_base/thread.h"

namespace webrtc {

class SystemAudioReceiver;

class ScreenCapturer : public VideoCapturer,
                       public VideoFrameReceiver::Callback,
                       public ohos::DisplayManager::ChangeObserver {
public:
    class AudioObserver {
    public:
        virtual ~AudioObserver() = default;
        virtual void OnStarted(bool success) = 0;
        virtual void OnStopped() = 0;
        virtual void OnData(void* buffer, int32_t length, int64_t timestampUs) = 0;
        virtual void OnError(int32_t errorCode, const std::string& message) = 0;
    };

    static std::unique_ptr<ScreenCapturer> Create(
        ScreenCaptureOptions options, std::shared_ptr<SystemAudioReceiver> systemAudioReceiver = nullptr);

    // Do not use this constructor directly, use 'Create' above.
    ScreenCapturer(ScreenCaptureOptions options, std::shared_ptr<SystemAudioReceiver> systemAudioReceiver);
    ~ScreenCapturer() override;

    void Init(std::unique_ptr<VideoFrameReceiver>, Observer* observer) override;
    void Release() override;

    void Start() override;
    void Stop() override;

    bool IsScreencast() override;

protected:
    void SetupConfig(int32_t width, int32_t height, OH_AVScreenCaptureConfig& config);
    bool SetupContentFilter(ohos::AVScreenCaptureContentFilter& contentFilter);

    void NotifyCaptureStart(bool success);
    void NotifyCaptureStop();

    int32_t GetVideoFrameWidth() const;
    uint64_t GetVideoFrameHeight() const;
    uint64_t GetDisplayId() const;
    bool IsAutoRotation() const;

    uint32_t GetDefaultDisplayRotation() const;

protected:
    void OnFrameAvailable(
        rtc::scoped_refptr<VideoFrameBuffer> buffer, int64_t timestampUs, VideoRotation rotation) override;

    static void OnError1(OH_AVScreenCapture* capture, int32_t errorCode, void* userData);
    static void OnStateChange1(
        struct OH_AVScreenCapture* capture, OH_AVScreenCaptureStateCode stateCode, void* userData);
    static void OnBufferAvailable1(OH_AVScreenCapture* capture, OH_AVBuffer* buffer,
        OH_AVScreenCaptureBufferType bufferType, int64_t timestamp, void* userData);

    void OnError(OH_AVScreenCapture* capture, int32_t errorCode);
    void OnStateChange(struct OH_AVScreenCapture* capture, OH_AVScreenCaptureStateCode stateCode);
    void OnBufferAvailable(
        OH_AVScreenCapture* capture, OH_AVBuffer* buffer, OH_AVScreenCaptureBufferType bufferType, int64_t timestamp);

    // ohos::DisplayManager::ChangeObserver implementation.
    void OnDisplayChange(uint64_t displayId) override;

private:
    const ScreenCaptureOptions options_;
    const std::shared_ptr<SystemAudioReceiver> systemAudioReceiver_;

    bool isInitialized_{false};
    bool isStarted_{false};

    uint32_t initDisplayRotation_{0};
    std::atomic<uint32_t> displayRotation_{0};

    ohos::AVScreenCapture screenCapture_;

    std::unique_ptr<VideoFrameReceiver> dataReceiver_;

    Observer* observer_{};
    mutable std::mutex obsMutex_;
};

} // namespace webrtc

#endif // WEBRTC_SCREEN_CAPTURER_H
