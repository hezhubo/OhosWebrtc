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

#ifndef WEBRTC_SCREEN_CAPTURE_OPTIONS_H
#define WEBRTC_SCREEN_CAPTURE_OPTIONS_H

#include <string>
#include <vector>
#include <optional>

namespace webrtc {

struct ScreenCaptureOptions {
    static std::optional<int32_t> CaptureModeFromString(std::string captureMode);
    static std::vector<int32_t> FilterableAudioContentFromString(std::vector<std::string> audioContents);

    ScreenCaptureOptions();

    std::string ToString() const;

    // See OH_CaptureMode
    std::optional<int32_t> captureMode;
    // Screen id, should be set while captureMode = CAPTURE_SPECIFIED_SCREEN
    std::optional<uint64_t> displayId;
    // The ids of mission, should be set while captureMode = CAPTURE_SPECIFIED_WINDOW
    std::vector<int32_t> missionIds;
    // Video frame width of avscreeencapture
    std::optional<int32_t> videoFrameWidth;
    // Video frame height of avscreeencapture
    std::optional<int32_t> videoFrameHeight;
    // Audio capture source type, See OH_AudioCaptureSourceType
    std::optional<int32_t> audioSource;
    // The ids of window to be added to the screen capture content filter
    std::vector<int32_t> filteredWindowIds;
    // The audio contents to be added to the screen capture content filter
    std::vector<int32_t> filteredAudioContents;
    // The ids of window whose privacy mode of current app would be skipped during the screen recording
    std::vector<int32_t> skipPrivacyModeWindowIds;
    // Auto rotaion follow device display
    std::optional<bool> autoRotation;
};

} // namespace webrtc

#endif // WEBRTC_SCREEN_CAPTURE_OPTIONS_H
