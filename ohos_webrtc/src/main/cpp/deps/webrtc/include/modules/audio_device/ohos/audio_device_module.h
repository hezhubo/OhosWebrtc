/*
# Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_AUDIO_DEVICE_MODULE_H
#define OHOS_AUDIO_DEVICE_MODULE_H

#include <memory>

#include "absl/types/optional.h"
#include "modules/audio_device/include/audio_device.h"

namespace webrtc {
class AudioDeviceBuffer;
class AudioInput {
public:
    virtual ~AudioInput() {}

    virtual int32_t Init() = 0;
    virtual int32_t Terminate() = 0;

    virtual int32_t InitRecording() = 0;
    virtual bool RecordingIsInitialized() const = 0;

    virtual int32_t StartRecording() = 0;
    virtual int32_t StopRecording() = 0;
    virtual bool Recording() const = 0;

    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) = 0;

    virtual bool IsAcousticEchoCancelerSupported() const = 0;
    virtual bool IsNoiseSuppressorSupported() const = 0;

    virtual int32_t EnableBuiltInAEC(bool enable) = 0;
    virtual int32_t EnableBuiltInNS(bool enable) = 0;
};

class AudioOutput {
public:
    virtual ~AudioOutput() {}

    virtual int32_t Init() = 0;
    virtual int32_t Terminate() = 0;
    virtual int32_t InitPlayout() = 0;
    virtual bool PlayoutIsInitialized() const = 0;
    virtual int32_t StartPlayout() = 0;
    virtual int32_t StopPlayout() = 0;
    virtual bool Playing() const = 0;
    virtual int32_t SpeakerVolumeIsAvailable(bool &available) = 0;
    virtual int32_t SetSpeakerVolume(uint32_t volume) = 0;
    virtual int32_t SpeakerVolume(uint32_t &volume) const = 0;
    virtual int32_t MaxSpeakerVolume(uint32_t &maxVolume) const = 0;
    virtual int32_t MinSpeakerVolume(uint32_t &minVolume) const = 0;
    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) = 0;
    virtual int GetPlayoutUnderrunCount() = 0;
    virtual absl::optional<AudioDeviceModule::Stats> GetStats() const
    {
        return absl::nullopt;
    }
};
}  // namespace webrtc

#endif  // OHOS_AUDIO_DEVICE_MODULE_H
