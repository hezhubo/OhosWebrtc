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

#ifndef WEBRTC_TRACK_MEDIA_CONSTRAINTS_H
#define WEBRTC_TRACK_MEDIA_CONSTRAINTS_H

#include "user_media/media_constraints.h"
#include "utils/marcos.h"

#include <string>
#include <vector>

#include "napi.h"

#include "api/audio_options.h"

namespace webrtc {

class NapiMediaConstraints {
public:
    NAPI_ATTRIBUTE_NAME_DECLARE(Width, width);
    NAPI_ATTRIBUTE_NAME_DECLARE(Height, height);
    NAPI_ATTRIBUTE_NAME_DECLARE(AspectRatio, aspectRatio);
    NAPI_ATTRIBUTE_NAME_DECLARE(FrameRate, frameRate);
    NAPI_ATTRIBUTE_NAME_DECLARE(FacingMode, facingMode);
    NAPI_ATTRIBUTE_NAME_DECLARE(ResizeMode, resizeMode);
    NAPI_ATTRIBUTE_NAME_DECLARE(SampleRate, sampleRate);
    NAPI_ATTRIBUTE_NAME_DECLARE(SampleSize, sampleSize);
    NAPI_ATTRIBUTE_NAME_DECLARE(EchoCancellation, echoCancellation);
    NAPI_ATTRIBUTE_NAME_DECLARE(AutoGainControl, autoGainControl);
    NAPI_ATTRIBUTE_NAME_DECLARE(NoiseSuppression, noiseSuppression);
    NAPI_ATTRIBUTE_NAME_DECLARE(Latency, latency);
    NAPI_ATTRIBUTE_NAME_DECLARE(ChannelCount, ChannelCount);
    NAPI_ATTRIBUTE_NAME_DECLARE(DeviceId, deviceId);
    NAPI_ATTRIBUTE_NAME_DECLARE(GroupId, groupId);
    NAPI_ATTRIBUTE_NAME_DECLARE(BackgroundBlur, backgroundBlur);
    NAPI_ATTRIBUTE_NAME_DECLARE(DisplaySurface, displaySurface);
    NAPI_ATTRIBUTE_NAME_DECLARE(GoogEchoCancellation, googEchoCancellation);
    NAPI_ATTRIBUTE_NAME_DECLARE(GoogAutoGainControl, googAutoGainControl);
    NAPI_ATTRIBUTE_NAME_DECLARE(GoogNoiseSuppression, googNoiseSuppression);
    NAPI_ATTRIBUTE_NAME_DECLARE(GoogHighpassFilter, googHighpassFilter);
    NAPI_ATTRIBUTE_NAME_DECLARE(GoogAudioMirroring, googAudioMirroring);
    NAPI_ATTRIBUTE_NAME_DECLARE(OhosScreenCaptureMode, ohosScreenCaptureMode);
    NAPI_ATTRIBUTE_NAME_DECLARE(OhosScreenCaptureDisplayId, ohosScreenCaptureDisplayId);
    NAPI_ATTRIBUTE_NAME_DECLARE(OhosScreenCaptureMissionId, ohosScreenCaptureMissionId);
    NAPI_ATTRIBUTE_NAME_DECLARE(OhosScreenCaptureWindowFilter, ohosScreenCaptureWindowFilter);
    NAPI_ATTRIBUTE_NAME_DECLARE(OhosScreenCaptureAudioFilter, ohosScreenCaptureAudioFilter);
    NAPI_ATTRIBUTE_NAME_DECLARE(OhosScreenCaptureSkipPrivacyMode, ohosScreenCaptureSkipPrivacyMode);
    NAPI_ATTRIBUTE_NAME_DECLARE(OhosScreenCaptureAutoRotation, ohosScreenCaptureAutoRotation);

    static std::vector<std::string> GetSupportedConstraints();
    static bool IsConstraintSupported(const std::string& name);
    static void JsToNative(const Napi::Value& jsTrackConstraints, MediaTrackConstraints& nativeTrackConstraints);
};

} // namespace webrtc

#endif // WEBRTC_TRACK_MEDIA_CONSTRAINTS_H
