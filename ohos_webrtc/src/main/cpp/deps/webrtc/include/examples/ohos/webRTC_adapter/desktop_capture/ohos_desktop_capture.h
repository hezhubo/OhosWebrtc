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
#ifndef OH_WEB_RTC_OHOS_DESKTOP_CAPTURE_H
#define OH_WEB_RTC_OHOS_DESKTOP_CAPTURE_H

#include <memory>
#include <algorithm>
#include "api/video/video_frame.h"
#include "api/video/video_source_interface.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "ohos_desktop.h"
#include "rtc_base/synchronization/mutex.h"
#include "api/video/video_sink_interface.h"
#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"

namespace webrtc {
namespace ohos {

class OhosDesktopCapture : public rtc::VideoSourceInterface<VideoFrame>,
                           public rtc::VideoSinkInterface<VideoFrame> {
public:
    OhosDesktopCapture();
    ~OhosDesktopCapture();
    class FramePreprocessor {
    public:
        virtual ~FramePreprocessor() = default;
        virtual VideoFrame Preprocess(const VideoFrame &frame) = 0;
    };
    static OhosDesktopCapture *Create(OhosDesktop::CaptureType captureType = OhosDesktop::CaptureType::SURFACE);
    void AddOrUpdateSink(VideoSinkInterface<VideoFrame>* sink,const rtc::VideoSinkWants& wants) override;
    void RemoveSink(rtc::VideoSinkInterface<VideoFrame> *sink) override;
    void OnFrame(const VideoFrame &frame) override;
    void SetFramePreprocessor(std::unique_ptr<FramePreprocessor> preprocessor){};
private:
    bool Init(OhosDesktop::CaptureType captureType = OhosDesktop::CaptureType::SURFACE);
    void Destroy();
    void UpdateVideoAdapter();
    VideoFrame MaybePreprocess(const VideoFrame &frame);

    Mutex lock_;
    std::shared_ptr<OhosDesktop> desktop_{nullptr};
    rtc::VideoBroadcaster broadcaster_;
    cricket::VideoAdapter video_adapter_;
    bool enable_adaptation_ RTC_GUARDED_BY(lock_){false};
    std::unique_ptr<FramePreprocessor> preprocessor_ RTC_GUARDED_BY(lock_);
};
}
}


#endif //OH_WEB_RTC_OHOS_DESKTOP_CAPTURE_H
