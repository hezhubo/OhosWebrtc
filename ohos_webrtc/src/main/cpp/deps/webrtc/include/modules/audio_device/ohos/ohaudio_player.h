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

#ifndef OHAUDIO_PLAYER_H
#define OHAUDIO_PLAYER_H

#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostreambuilder.h>

#include <memory>

#include "absl/types/optional.h"
#include "api/sequence_checker.h"
#include "api/task_queue/task_queue_base.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/include/audio_device_defines.h"
#include "rtc_base/thread_annotations.h"
#include "rtc_base/race_checker.h"
#include "modules/audio_device/ohos/ohaudio_player_wrapper.h"
#include "modules/audio_device/ohos/audio_device_module.h"

namespace webrtc {

class AudioDeviceBuffer;
class FineAudioBuffer;

class OHAudioPlayer final : public AudioOutput, public OHAudioPlayerObserverInterface {
public:
    explicit OHAudioPlayer(const AudioParameters& audio_parameters);
    ~OHAudioPlayer() override;

    int Init() override;
    int Terminate() override;

    int InitPlayout() override;
    bool PlayoutIsInitialized() const override;

    int StartPlayout() override;
    int StopPlayout() override;
    bool Playing() const override;

    void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) override;

    int SpeakerVolumeIsAvailable(bool &available) override;
    int SetSpeakerVolume(uint32_t volume) override { return -1; }
    int SpeakerVolume(uint32_t &volume) const override { return -1; }
    int MaxSpeakerVolume(uint32_t &maxVolume) const override { return -1; }
    int MinSpeakerVolume(uint32_t &minVolume) const override { return -1; }

protected:
    int32_t OnDataCallback(void* audio_data, int32_t num_frames) override;
    int32_t OnErrorCallback(OH_AudioStream_Result error) override;

private:
    int GetPlayoutUnderrunCount() override { return 0; }
    void HandleStreamDisconnected();
    SequenceChecker main_thread_checker_;
    rtc::RaceChecker race_checker_ohaudio_;
    TaskQueueBase* main_thread_;
    OHAudioPlayerWrapper ohaudio_;
    std::unique_ptr<FineAudioBuffer> fine_audio_buffer_;
    int32_t underrun_count_ = 0;
    bool first_data_callback_ = true;
    AudioDeviceBuffer* audio_device_buffer_ RTC_GUARDED_BY(main_thread_checker_) = nullptr;

    bool initialized_ RTC_GUARDED_BY(main_thread_checker_) = false;
    bool playing_ RTC_GUARDED_BY(main_thread_checker_) = false;

    double latency_millis_ RTC_GUARDED_BY(race_checker_ohaudio_) = 0;
    static const int default_channels = 2;
};
}  // namespace webrtc

#endif  // OHAUDIO_PLAYER_H
