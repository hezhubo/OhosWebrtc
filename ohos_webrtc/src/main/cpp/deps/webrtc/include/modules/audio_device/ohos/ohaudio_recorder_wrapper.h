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

#ifndef SDK_OHOS_SRC_AUDIO_DEVICE_OHAUDIO_RECORDER_WRAPPER_H
#define SDK_OHOS_SRC_AUDIO_DEVICE_OHAUDIO_RECORDER_WRAPPER_H

#include <ohaudio/native_audiocapturer.h>
#include <ohaudio/native_audiostreambuilder.h>

#include "api/sequence_checker.h"
#include "modules/audio_device/include/audio_device_defines.h"

namespace webrtc {

class OHAudioRecorderObserverInterface {
public:
    virtual int32_t OnDataCallback(void* audio_data, int32_t num_frames) = 0;
    virtual int32_t OnErrorCallback(OH_AudioStream_Result error) = 0;

protected:
    virtual ~OHAudioRecorderObserverInterface() {}
};

class OHAudioRecorderWrapper {
public:
    OHAudioRecorderWrapper(const AudioParameters& audio_parameters,
                  OH_AudioStream_Type direction,
                  OHAudioRecorderObserverInterface* observer);
    ~OHAudioRecorderWrapper();

    bool Init();
    bool Start();
    bool Stop();

    double EstimateLatencyMillis() const;
    bool IncreaseOutputBufferSize();
    void ClearInputStream(void* audio_data, int32_t num_frames);

    OHAudioRecorderObserverInterface* observer() const;
    AudioParameters audio_parameters() const;
    int32_t samples_per_frame() const;
    int32_t device_id() const;
    int32_t xrun_count() const;
    OH_AudioStream_SampleFormat format() const;
    int32_t sample_rate() const;
    int32_t channel_count() const;
    OH_AudioStream_LatencyMode performance_mode() const;
    OH_AudioStream_State stream_state() const;
    int64_t frames_written() const;
    int64_t frames_read() const;
    OH_AudioStream_Type direction() const { return direction_; }
    OH_AudioCapturer* stream() const { return stream_; }
    int32_t frames_per_burst() const { return frames_per_burst_; }

private:
    void SetStreamConfiguration(OH_AudioStreamBuilder* builder);
    bool OpenStream(OH_AudioStreamBuilder* builder);
    void CloseStream();
    void LogStreamConfiguration();
    void LogStreamState();
    bool VerifyStreamConfiguration();
    bool OptimizeBuffers();

    SequenceChecker thread_checker_;
    SequenceChecker ohaudio_thread_checker_;
    const AudioParameters audio_parameters_;
    const OH_AudioStream_Type direction_;
    OHAudioRecorderObserverInterface* observer_ = nullptr;
    OH_AudioCapturer* stream_ = nullptr;
    int32_t frames_per_burst_ = 240;
};
}  // namespace webrtc

#endif  // SDK_OHOS_SRC_AUDIO_DEVICE_OHAUDIO_RECORDER_WRAPPER_H
