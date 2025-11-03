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

#ifndef WEBRTC_MEDIA_CONSTRAINTS_UTIL_H
#define WEBRTC_MEDIA_CONSTRAINTS_UTIL_H

#include "media_constraints.h"
#include "../camera/camera_device_info.h"
#include "../screen_capture/screen_capture_options.h"

#include <string>

#include "api/audio_options.h"
#include "rtc_base/checks.h"

namespace webrtc {

constexpr int32_t kDefaultWidth = 640;
constexpr int32_t kDefaultHeight = 480;
constexpr int32_t kDefaultFrameRate = 30;

struct CameraCaptureSettings {
    std::string ToString() const;

    std::string deviceId;
    video::VideoProfile profile;
};

bool SelectSettingsForVideo(
    const std::vector<CameraDeviceInfo>& devices, const MediaTrackConstraints& constraints, int defaultWidth,
    int defaultHeight, double defaultFrameRate, CameraCaptureSettings& setting, std::string& failedConstraintName);

// Copy all relevant constraints into an AudioOptions object.
void CopyConstraintsIntoAudioOptions(const MediaTrackConstraints& constraints, cricket::AudioOptions& options);

void GetScreenCaptureOptionsFromConstraints(const MediaTrackConstraints& constraints, ScreenCaptureOptions& options);

} // namespace webrtc

#endif // WEBRTC_MEDIA_CONSTRAINTS_UTIL_H
