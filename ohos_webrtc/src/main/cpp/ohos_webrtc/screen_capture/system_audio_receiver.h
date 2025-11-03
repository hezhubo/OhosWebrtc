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

#ifndef WEBRTC_PLAYBACK_AUDIO_CAPTURER_H
#define WEBRTC_PLAYBACK_AUDIO_CAPTURER_H

#include <set>
#include <memory>
#include <atomic>

#include "napi.h"

#include "api/sequence_checker.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/fine_audio_buffer.h"

#include "../audio_device/audio_input.h"
#include "screen_capturer.h"

namespace webrtc {

class SystemAudioReceiver final : public AudioInputBase, public ScreenCapturer::AudioObserver {
public:
    static std::unique_ptr<SystemAudioReceiver> Create();
    static std::unique_ptr<SystemAudioReceiver> Create(AudioInputOptions options);

    // Do not use this constructor directly, use 'Create' above.
    explicit SystemAudioReceiver(AudioInputOptions options);
    ~SystemAudioReceiver() override;

    // Implements AudioInput
    int32_t Init() override;
    int32_t Terminate() override;

    int32_t InitRecording() override;
    bool RecordingIsInitialized() const override;

    int32_t StartRecording() override;
    int32_t StopRecording() override;
    bool Recording() const override;

    std::string GetLabel() const override;

    int32_t GetAudioSource() const;

    // Implements ScreenCapturer::AudioObserver
    void OnStarted(bool success) override;
    void OnStopped() override;
    void OnData(void* buffer, int32_t length, int64_t timestampUs) override;
    void OnError(int32_t errorCode, const std::string& message) override;

private:
    SequenceChecker threadChecker_;

    std::atomic<bool> initialized_{false};
    std::atomic<bool> recording_{false};

    // Sets all recorded samples to zero if `mute_` is true.
    std::atomic<bool> mute_{false};
};

} // namespace webrtc

#endif // WEBRTC_PLAYBACK_AUDIO_CAPTURER_H
