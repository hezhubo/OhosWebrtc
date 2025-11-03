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

#ifndef WEBRTC_AUDIO_COMMON_H
#define WEBRTC_AUDIO_COMMON_H

#include <bits/alltypes.h>
#include <cstddef>
#include <cstdint>

#include <ohaudio/native_audiostream_base.h>

namespace webrtc {

constexpr int kAudioChannelCount_Mono = 1;
constexpr int kAudioChannelCount_Stereo = 2;

constexpr int kAudioSampleRate_16000 = 16000;
constexpr int kAudioSampleRate_48000 = 48000;
constexpr int kAudioSampleRate_Default = kAudioSampleRate_16000;

constexpr float kHalfSec = 0.5;

const int kLowLatencyModeDelayEstimateInMilliseconds = 25;
const int kHighLatencyModeDelayEstimateInMilliseconds = 75;

#define OH_RESULT_CHECK(op, ...)                                                                                       \
    do {                                                                                                               \
        OH_AudioStream_Result result = (op);                                                                           \
        if (result != AUDIOSTREAM_SUCCESS) {                                                                           \
            RTC_LOG(LS_ERROR) << #op << ": " << result;                                                                \
            return __VA_ARGS__;                                                                                        \
        }                                                                                                              \
    } while (0)

inline const char* StateToString(OH_AudioStream_State state)
{
    switch (state) {
        case AUDIOSTREAM_STATE_INVALID:
            return "INVALID";
        case AUDIOSTREAM_STATE_PREPARED:
            return "PREPARED";
        case AUDIOSTREAM_STATE_RUNNING:
            return "RUNNING";
        case AUDIOSTREAM_STATE_STOPPED:
            return "STOPPED";
        case AUDIOSTREAM_STATE_RELEASED:
            return "RELEASED";
        case AUDIOSTREAM_STATE_PAUSED:
            return "PAUSED";
        default:
            return "UNKNOWN";
    }
}

enum class AudioErrorType {
    INIT,
    START_EXCEPTION,
    START_STATE_MISMATCH,
    GENERAL
};

enum class AudioStateType {
    START,
    STOP
};

} // namespace webrtc

#endif // WEBRTC_AUDIO_COMMON_H
