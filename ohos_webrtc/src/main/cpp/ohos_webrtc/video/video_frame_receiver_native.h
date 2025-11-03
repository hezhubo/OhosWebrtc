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

#ifndef WEBRTC_VIDEO_FRAME_RECEIVER_RASTER_H
#define WEBRTC_VIDEO_FRAME_RECEIVER_RASTER_H

#include "video_frame_receiver.h"

#include <map>
#include <memory>
#include <cstdint>

#include <multimedia/image_framework/image/image_receiver_native.h>

#include "rtc_base/thread.h"

namespace webrtc {

class VideoFrameReceiverNative : public VideoFrameReceiver {
public:
    static std::unique_ptr<VideoFrameReceiverNative> Create(const std::string& threadName);

    // Do not use this constructor directly, use 'Create' above.
    explicit VideoFrameReceiverNative(const std::string& threadName);
    ~VideoFrameReceiverNative() override;

    uint64_t GetSurfaceId() const override;
    void SetVideoFrameSize(int32_t width, int32_t height) override;

protected:
    void CreateImageReceiver();
    void ReleaseImageReceiver();

    static void OnImageReceiverCallback1(OH_ImageReceiverNative* receiver);
    void OnImageReceiverCallback();

private:
    static std::map<OH_ImageReceiverNative*, VideoFrameReceiverNative*> receiverMap_;

    std::unique_ptr<rtc::Thread> thread_;
    int32_t width_{};
    int32_t height_{};
    OH_ImageReceiverNative* imageReceiver_{nullptr};
};

} // namespace webrtc

#endif // WEBRTC_VIDEO_FRAME_RECEIVER_RASTER_H
