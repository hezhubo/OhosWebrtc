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

#include <sstream>

#include <multimedia/player_framework/native_avscreen_capture_base.h>

#include "rtc_base/logging.h"

#include "screen_capture_options.h"

namespace webrtc {

namespace {

template <class T>
void ToStringIfSet(std::ostringstream& result, const char* key, const std::optional<T>& val)
{
    if (val) {
        result << key << ": " << *val << ", ";
    }
}

template <class T>
void ToStringIfNotEmpty(std::ostringstream& result, const char* key, const std::vector<T>& val)
{
    if (!val.empty()) {
        result << key << ": [";
        for (std::size_t i = 0; i < val.size(); i++) {
            if (i > 0) {
                result << ",";
            }
            result << val[i];
        }
        result << "], ";
    }
}

} // namespace

ScreenCaptureOptions::ScreenCaptureOptions() = default;

std::optional<int32_t> ScreenCaptureOptions::CaptureModeFromString(std::string captureMode)
{
    if (captureMode == "home-screen") {
        return OH_CAPTURE_HOME_SCREEN;
    } else if (captureMode == "specified-screen") {
        return OH_CAPTURE_SPECIFIED_SCREEN;
    } else if (captureMode == "specified-window") {
        return OH_CAPTURE_SPECIFIED_WINDOW;
    } else {
        RTC_LOG(LS_WARNING) << "Unknown capture mode: " << captureMode;
    }

    return std::nullopt;
}

std::vector<int32_t> ScreenCaptureOptions::FilterableAudioContentFromString(std::vector<std::string> audioContents)
{
    std::vector<int32_t> result;

    for (auto& audioContent : audioContents) {
        if (audioContent == "current-app") {
            result.push_back(OH_SCREEN_CAPTURE_CURRENT_APP_AUDIO);
        } else if (audioContent == "notification") {
            result.push_back(OH_SCREEN_CAPTURE_NOTIFICATION_AUDIO);
        } else {
            RTC_LOG(LS_WARNING) << "Unknown audio content: " << audioContent;
        }
    }

    return result;
}

std::string ScreenCaptureOptions::ToString() const
{
    std::ostringstream ss;

    ss << "ScreenCaptureOptions {";
    ToStringIfSet(ss, "captureMode", captureMode);
    ToStringIfSet(ss, "displayId", displayId);
    ToStringIfNotEmpty(ss, "missionIds", missionIds);
    ToStringIfSet(ss, "videoFrameWidth", videoFrameWidth);
    ToStringIfSet(ss, "videoFrameHeight", videoFrameHeight);
    ToStringIfSet(ss, "audioSource", audioSource);
    ToStringIfNotEmpty(ss, "filteredAudioContents", filteredAudioContents);
    ToStringIfNotEmpty(ss, "filteredWindowIds", filteredWindowIds);
    ToStringIfNotEmpty(ss, "skipPrivacyModeWindowIds", skipPrivacyModeWindowIds);
    ss << "}";

    return ss.str();
}

} // namespace webrtc
