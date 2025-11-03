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

#ifndef WEBRTC_CAMERA_CAPTURER_H
#define WEBRTC_CAMERA_CAPTURER_H

#include "camera_device_info.h"
#include "../video/video_capturer.h"
#include "../video/video_frame_receiver.h"
#include "../render/egl_context.h"
#include "../helper/camera.h"

#include <memory>
#include <cstdint>
#include <mutex>

#include <ohcamera/camera_manager.h>

#include "api/video/video_frame_buffer.h"
#include "rtc_base/thread.h"

namespace webrtc {

class CameraCapturer : public VideoCapturer,
                       public VideoFrameReceiver::Callback,
                       public ohos::CameraPreviewOutput::Observer,
                       public ohos::CameraVideoOutput::Observer {
public:
    static std::unique_ptr<CameraCapturer> Create(std::string deviceId, video::VideoProfile profile);

    // Do not use this constructor directly, use 'Create' above.
    CameraCapturer(std::string deviceId, video::VideoProfile profile);
    ~CameraCapturer() override;

    void Init(std::unique_ptr<VideoFrameReceiver>, VideoCapturer::Observer* observer) override;
    void Release() override;
    void Start() override;
    void Stop() override;
    bool IsScreencast() override;

protected:
    void NotifyCapturedStart(bool success);
    void NotifyCapturedStop();

protected:
    void StartInternal();
    void StopInternal();

    void OnCameraManagerStatusCallback(Camera_StatusInfo* status);
    void OnPreviewOutputFrameStart() override;
    void OnPreviewOutputFrameEnd(int32_t frameCount) override;
    void OnPreviewOutputError(Camera_ErrorCode errorCode) override;
    void OnVideoOutputFrameStart() override;
    void OnVideoOutputFrameEnd(int32_t frameCount) override;
    void OnVideoOutputError(Camera_ErrorCode errorCode) override;

    void
    OnFrameAvailable(rtc::scoped_refptr<VideoFrameBuffer> buffer, int64_t timestampUs, VideoRotation rotation) override;

private:
    const std::string deviceId_;
    const video::VideoProfile profile_;

    bool isInitialized_{false};
    bool isStarted_{false};

    bool useVideoSceneMode_{false};
    bool isCameraFrontFacing_{false};
    uint32_t cameraOrientation_{0};

    ohos::CameraInput input_;
    ohos::CameraPreviewOutput previewOutput_;
    ohos::CameraVideoOutput videoOutput_;
    ohos::CameraCaptureSession captureSession_;

    std::unique_ptr<VideoFrameReceiver> dataReceiver_;

    VideoCapturer::Observer* observer_{};
    std::mutex obsMutex_;
};

} // namespace webrtc

#endif // WEBRTC_CAMERA_CAPTURER_H
