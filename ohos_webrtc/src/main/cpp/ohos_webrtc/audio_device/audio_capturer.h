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

#ifndef WEBRTC_AUDIO_CAPTURER_H
#define WEBRTC_AUDIO_CAPTURER_H

#include "audio_input.h"

#include <ohaudio/native_audiocapturer.h>
#include <ohaudio/native_audiostreambuilder.h>

#include <set>
#include <memory>
#include <atomic>

#include "napi.h"

#include "api/sequence_checker.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/fine_audio_buffer.h"

namespace webrtc {

class AudioCapturer final : public AudioInputBase {
public:
    static std::unique_ptr<AudioCapturer> Create(AudioInputOptions options);

    // Do not use this constructor directly, use 'Create' above.
    explicit AudioCapturer(AudioInputOptions options);
    ~AudioCapturer() override;

    int32_t Init() override;
    int32_t Terminate() override;

    int32_t InitRecording() override;
    bool RecordingIsInitialized() const override;

    int32_t StartRecording() override;
    int32_t StopRecording() override;
    bool Recording() const override;

    std::string GetLabel() const override;

protected:
    static int32_t OnReadData1(OH_AudioCapturer* stream, void* userData, void* buffer, int32_t lenth);
    static int32_t OnStreamEvent1(OH_AudioCapturer* capturer, void* userData, OH_AudioStream_Event event);
    static int32_t OnInterruptEvent1(
        OH_AudioCapturer* stream, void* userData, OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint);
    static int32_t OnError1(OH_AudioCapturer* stream, void* userData, OH_AudioStream_Result error);

    int32_t OnReadData(OH_AudioCapturer* stream, void* buffer, int32_t lenth);
    int32_t OnStreamEvent(OH_AudioCapturer* stream, OH_AudioStream_Event event);
    int32_t OnInterruptEvent(OH_AudioCapturer* stream, OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint);
    int32_t OnError(OH_AudioCapturer* stream, OH_AudioStream_Result error);

    int32_t GetAudioSource() const;
    OH_AudioStream_State GetCurrentState() const;
    double EstimateLatencyMillis() const;
    uint32_t GetOverflowCount() const;
    bool CheckConfiguration(OH_AudioCapturer* capturer);

private:
    SequenceChecker threadChecker_;

    bool initialized_{false};
    bool recording_{false};

    int32_t framesPerBurst_{0};
    uint32_t overflowCount_{0};

    OH_AudioCapturer* capturer_{nullptr};
};

} // namespace webrtc

#endif // WEBRTC_AUDIO_CAPTURER_H
