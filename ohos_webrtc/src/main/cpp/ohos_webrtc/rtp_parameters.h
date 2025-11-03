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

#ifndef WEBRTC_RTP_PARAMETERS_H
#define WEBRTC_RTP_PARAMETERS_H

#include "napi.h"

#include "api/rtp_parameters.h"

namespace webrtc {

struct NapiRtpSendParameters {
    static void JsToNative(const Napi::Object& js, RtpParameters& native);
    static void NativeToJs(const RtpParameters& native, Napi::Object& js);
};

struct NapiRtpReceiveParameters {
    static void JsToNative(const Napi::Object& js, RtpParameters& native);
    static void NativeToJs(const RtpParameters& native, Napi::Object& js);
};

struct NapiRtpCapabilities {
    static void JsToNative(const Napi::Object& js, RtpCapabilities& native);
    static void NativeToJs(const RtpCapabilities& native, Napi::Object& js);
};

struct NapiRtpCodecCapability {
    static void JsToNative(const Napi::Object& js, RtpCodecCapability& native);
    static void NativeToJs(const RtpCodecCapability& native, Napi::Object& js);
};

struct NapiRtpEncodingParameters {
    constexpr static char kAttributeNameSsrc[] = "ssrc";

    static void JsToNative(const Napi::Object& js, RtpEncodingParameters& native);
    static void NativeToJs(const RtpEncodingParameters& native, Napi::Object& js);
};

} // namespace webrtc

#endif // WEBRTC_RTP_PARAMETERS_H
