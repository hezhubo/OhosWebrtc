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

#ifndef WEBRTC_AUDIO_OUTPUT_H
#define WEBRTC_AUDIO_OUTPUT_H

#include <string>

#include "absl/types/optional.h"
#include "modules/audio_device/include/audio_device.h"

#include "audio_common.h"

namespace webrtc {

class AudioDeviceBuffer;

struct AudioOutputOptions {
    std::optional<int32_t> sampleRate;
    std::optional<int32_t> channelCount;
    std::optional<int32_t> usage;
    std::optional<bool> useLowLatency;
};

class AudioOutput {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void OnAudioOutputError(AudioOutput* output, AudioErrorType type, const std::string& message) = 0;
        virtual void OnAudioOutputStateChange(AudioOutput* output, AudioStateType newState) = 0;
    };

    virtual ~AudioOutput() {}

    virtual int32_t Init() = 0;
    virtual int32_t Terminate() = 0;

    virtual int32_t InitPlayout() = 0;
    virtual bool PlayoutIsInitialized() const = 0;

    virtual int32_t StartPlayout() = 0;
    virtual int32_t StopPlayout() = 0;
    virtual bool Playing() const = 0;

    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) = 0;

    virtual int32_t SetMute(bool mute) = 0;
    virtual int32_t PlayoutDelay(uint16_t* delayMs) const = 0;
    virtual int GetPlayoutUnderrunCount() = 0;

    virtual absl::optional<AudioDeviceModule::Stats> GetStats() const
    {
        return absl::nullopt;
    }

    virtual void RegisterObserver(Observer* obs) = 0;
    virtual void UnregisterObserver(Observer* obs) = 0;

    virtual int32_t GetSampleRate() const = 0;
    virtual int32_t GetChannelCount() const = 0;
    virtual bool UseLowLatency() const = 0;
};

} // namespace webrtc

#endif // WEBRTC_AUDIO_OUTPUT_H
