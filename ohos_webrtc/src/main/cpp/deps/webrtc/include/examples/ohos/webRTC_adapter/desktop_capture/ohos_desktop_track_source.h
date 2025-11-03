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
#ifndef OH_WEB_RTC_DESKTOP_TRACK_SOURCE_H
#define OH_WEB_RTC_DESKTOP_TRACK_SOURCE_H

#include "ohos_desktop_capture.h"
#include <absl/memory/memory.h>
#include "pc/video_track_source.h"
#include <api/make_ref_counted.h>

namespace webrtc {
namespace ohos {
class DesktopTrackSource : public webrtc::VideoTrackSource {
public:
    static rtc::scoped_refptr<DesktopTrackSource> Create(OhosDesktop::CaptureType captureType
                                                         = OhosDesktop::CaptureType::SURFACE);
    virtual ~DesktopTrackSource();

protected:
    explicit DesktopTrackSource(std::unique_ptr<webrtc::ohos::OhosDesktopCapture> capturer)
      : webrtc::VideoTrackSource(false), capturer_(std::move(capturer)) {}


private:
    rtc::VideoSourceInterface<webrtc::VideoFrame> *source() override;
    std::unique_ptr<OhosDesktopCapture> capturer_;
};
}   // namespace ohos
}   // namespace webrtc

#endif // OH_WEB_RTC_DESKTOP_TRACK_SOURCE_H
