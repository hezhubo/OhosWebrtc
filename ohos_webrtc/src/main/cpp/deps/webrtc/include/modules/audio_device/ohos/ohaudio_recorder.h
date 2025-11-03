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

#ifndef SDK_OHOS_SRC_AUDIO_DEVICE_OHAUDIO_RECORDER_H
#define SDK_OHOS_SRC_AUDIO_DEVICE_OHAUDIO_RECORDER_H

#include <ohaudio/native_audiocapturer.h>
#include <ohaudio/native_audiostreambuilder.h>

#include <memory>

#include "api/sequence_checker.h"
#include "api/task_queue/task_queue_base.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/include/audio_device_defines.h"
#include "modules/audio_device/ohos/ohaudio_recorder_wrapper.h"
#include "modules/audio_device/ohos/audio_device_module.h"

namespace webrtc {

class FineAudioBuffer;
class AudioDeviceBuffer;

class OHAudioRecorder : public AudioInput, public OHAudioRecorderObserverInterface {
public:
    explicit OHAudioRecorder(const AudioParameters& audio_parameters);
    ~OHAudioRecorder() override;

    int Init() override;
    int Terminate() override;

    int InitRecording() override;
    bool RecordingIsInitialized() const override;

    int StartRecording() override;
    int StopRecording() override;
    bool Recording() const override;

    void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) override;

    bool IsAcousticEchoCancelerSupported() const override;
    bool IsNoiseSuppressorSupported() const override;
    int EnableBuiltInAEC(bool enable) override;
    int EnableBuiltInNS(bool enable) override;

protected:
    int32_t OnDataCallback(void* audio_data, int32_t num_frames) override;
    int32_t OnErrorCallback(OH_AudioStream_Result error) override;

private:
    void HandleStreamDisconnected();
    SequenceChecker thread_checker_;
    SequenceChecker thread_checker_ohaudio_;
    TaskQueueBase* main_thread_;
    OHAudioRecorderWrapper ohaudio_;
    AudioDeviceBuffer* audio_device_buffer_ = nullptr;

    bool initialized_ = false;
    bool recording_ = false;
    std::unique_ptr<FineAudioBuffer> fine_audio_buffer_;
    int32_t overflow_count_ = 0;
    double latency_millis_ = 0;
    bool first_data_callback_ = true;

    static const int default_channels = 2;
};
}  // namespace webrtc

#endif  // SDK_OHOS_SRC_AUDIO_DEVICE_OHAUDIO_RECORDER_H
