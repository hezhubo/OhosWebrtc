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

#ifndef WEBRTC_MIXING_AUDIO_INPUT_H
#define WEBRTC_MIXING_AUDIO_INPUT_H

#include <mutex>
#include <memory>
#include <string>
#include <vector>

#include "api/audio/audio_mixer.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/fine_audio_buffer.h"
#include "rtc_base/thread.h"

#include "audio_common.h"
#include "audio_input.h"

namespace webrtc {

class AudioMixerSourceAdapter;

class MixingAudioInput final : public AudioInputBase, public AudioInput::Observer {
public:
    static int CalculateOutputSampleRate(const std::list<std::shared_ptr<AudioInput>>& inputs);
    static int CalculateOutputChannelCount(const std::list<std::shared_ptr<AudioInput>>& inputs);

    static std::unique_ptr<MixingAudioInput> Create(AudioInputOptions options);

    explicit MixingAudioInput(AudioInputOptions options);
    ~MixingAudioInput() override;

    int32_t Init() override;
    int32_t Terminate() override;

    int32_t InitRecording() override;
    bool RecordingIsInitialized() const override;

    int32_t StartRecording() override;
    int32_t StopRecording() override;
    bool Recording() const override;

    bool AddAudioInput(std::shared_ptr<AudioInput> input);
    bool RemoveAudioInput(std::shared_ptr<AudioInput> input);

protected:
    void DoMix();

    void OnAudioInputError(AudioInput* input, AudioErrorType type, const std::string& message) override;
    void OnAudioInputStateChange(AudioInput* input, AudioStateType newState) override;
    void OnAudioInputDataReady(
        AudioInput* input, void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs) override;

private:
    SequenceChecker threadChecker_;

    rtc::scoped_refptr<AudioMixer> mixer_;

    std::mutex sourcesMutex_;
    std::vector<std::shared_ptr<AudioMixerSourceAdapter>> sources_;

    std::unique_ptr<rtc::Thread> thread_;

    bool initialized_{false};
    std::atomic<bool> recording_{false};
};

} // namespace webrtc

#endif // WEBRTC_MIXING_AUDIO_INPUT_H
