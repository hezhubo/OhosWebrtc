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

#include "audio_capturer.h"
#include "audio_common.h"

#include "rtc_base/copy_on_write_buffer.h"
#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

std::unique_ptr<AudioCapturer> AudioCapturer::Create(AudioInputOptions options)
{
    return std::make_unique<AudioCapturer>(std::move(options));
}

int32_t AudioCapturer::OnReadData1(OH_AudioCapturer* stream, void* userData, void* buffer, int32_t length)
{
    AudioCapturer* self = reinterpret_cast<AudioCapturer*>(userData);
    return self->OnReadData(stream, buffer, length);
}

int32_t AudioCapturer::OnStreamEvent1(OH_AudioCapturer* stream, void* userData, OH_AudioStream_Event event)
{
    AudioCapturer* self = reinterpret_cast<AudioCapturer*>(userData);
    return self->OnStreamEvent(stream, event);
}

int32_t AudioCapturer::OnInterruptEvent1(
    OH_AudioCapturer* stream, void* userData, OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint)
{
    AudioCapturer* self = reinterpret_cast<AudioCapturer*>(userData);
    return self->OnInterruptEvent(stream, type, hint);
}

int32_t AudioCapturer::OnError1(OH_AudioCapturer* stream, void* userData, OH_AudioStream_Result error)
{
    AudioCapturer* self = reinterpret_cast<AudioCapturer*>(userData);
    return self->OnError(stream, error);
}

AudioCapturer::AudioCapturer(AudioInputOptions options) : AudioInputBase(std::move(options))
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // Detach from this thread since construction is allowed to happen on a different thread.
    threadChecker_.Detach();
}

AudioCapturer::~AudioCapturer()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    Terminate();

    RTC_LOG(LS_INFO) << "detected owerflows: " << overflowCount_;
}

int32_t AudioCapturer::Init()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    return 0;
}

int32_t AudioCapturer::Terminate()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    StopRecording();
    threadChecker_.Detach();
    OH_RESULT_CHECK(OH_AudioCapturer_Release(capturer_), -1);

    initialized_ = false;

    return 0;
}

int32_t AudioCapturer::InitRecording()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (initialized_) {
        // Already initialized.
        return 0;
    }
    RTC_DCHECK(!recording_);

    OH_AudioStreamBuilder* builder = nullptr;
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_CAPTURER),
        NotifyError(AudioErrorType::INIT, "system error"), -1);

    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetCapturerInfo(builder, static_cast<OH_AudioStream_SourceType>(GetAudioSource())),
        NotifyError(AudioErrorType::INIT, "failed to set capturer info"), -1);
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetSamplingRate(builder, GetSampleRate()),
        NotifyError(AudioErrorType::INIT, "failed to set sample rate"), -1);
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetChannelCount(builder, GetChannelCount()),
        NotifyError(AudioErrorType::INIT, "failed to set channel count"), -1);
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetSampleFormat(builder, static_cast<OH_AudioStream_SampleFormat>(GetSampleFormat())),
        NotifyError(AudioErrorType::INIT, "failed to set sample format"), -1);
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetLatencyMode(
            builder, UseLowLatency() ? AUDIOSTREAM_LATENCY_MODE_FAST : AUDIOSTREAM_LATENCY_MODE_NORMAL),
        NotifyError(AudioErrorType::INIT, "failed to set latency mode"), -1);

    OH_AudioCapturer_Callbacks callbacks;
    callbacks.OH_AudioCapturer_OnReadData = OnReadData1;
    callbacks.OH_AudioCapturer_OnStreamEvent = OnStreamEvent1;
    callbacks.OH_AudioCapturer_OnInterruptEvent = OnInterruptEvent1;
    callbacks.OH_AudioCapturer_OnError = OnError1;
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetCapturerCallback(builder, callbacks, this),
        NotifyError(AudioErrorType::INIT, "failed to set capture callback"), -1);

    OH_AudioCapturer* stream = nullptr;
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_GenerateCapturer(builder, &stream), NotifyError(AudioErrorType::INIT, "system error"),
        -1);

    if (!CheckConfiguration(stream)) {
        return -1;
    }

    capturer_ = stream;
    initialized_ = true;

    RTC_DLOG(LS_VERBOSE) << "current state: " << StateToString(GetCurrentState());

    return 0;
}

bool AudioCapturer::RecordingIsInitialized() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " initialized_ = " << initialized_;
    return initialized_;
}

int32_t AudioCapturer::StartRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (recording_) {
        // Already recording.
        return 0;
    }

    if (!initialized_) {
        RTC_DLOG(LS_WARNING) << "Recording can not start since InitRecording must succeed first";
        return 0;
    }

    OH_AudioStream_State state = GetCurrentState();
    if (state != AUDIOSTREAM_STATE_PREPARED && state != AUDIOSTREAM_STATE_STOPPED) {
        RTC_LOG(LS_ERROR) << "Invalid state: " << StateToString(state);
        NotifyError(AudioErrorType::START_STATE_MISMATCH, std::string("invalid state: ") + StateToString(state));
        return -1;
    }

    OH_RESULT_CHECK(
        OH_AudioCapturer_Start(capturer_), NotifyError(AudioErrorType::START_EXCEPTION, "system error"), -1);
    RTC_DLOG(LS_INFO) << "current state: " << StateToString(GetCurrentState());

    overflowCount_ = GetOverflowCount();
    recording_ = true;

    NotifyStateChange(AudioStateType::START);

    return 0;
}

int32_t AudioCapturer::StopRecording()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (!initialized_ || !recording_) {
        return 0;
    }

    OH_RESULT_CHECK(OH_AudioCapturer_Stop(capturer_), -1);
    recording_ = false;

    NotifyStateChange(AudioStateType::STOP);

    return 0;
}

bool AudioCapturer::Recording() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << " recording_ = " << recording_;
    return recording_;
}

std::string AudioCapturer::GetLabel() const
{
    return "Default";
}

int32_t AudioCapturer::OnReadData(OH_AudioCapturer* stream, void* buffer, int32_t length)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " bufferLen=" << length;

    (void)stream;

    const uint32_t overflowCount = GetOverflowCount();
    if (overflowCount_ < overflowCount) {
        RTC_LOG(LS_ERROR) << "Overflow detected: " << overflowCount;
        overflowCount_ = overflowCount;
    }

    auto latencyMillis = EstimateLatencyMillis();
    RTC_DLOG(LS_VERBOSE) << "Estimate latencyMillis=" << latencyMillis;

    if (mute_) {
        auto it = static_cast<uint8_t*>(buffer);
        std::fill(it, it + length, 0);
    }

    NotifyDataReady(buffer, length, rtc::TimeMicros(), latencyMillis);

    return -1;
}

int32_t AudioCapturer::OnStreamEvent(OH_AudioCapturer* stream, OH_AudioStream_Event event)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " event=" << event;

    (void)stream;

    return 0;
}

int32_t
AudioCapturer::OnInterruptEvent(OH_AudioCapturer* stream, OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint)
{
    RTC_LOG(LS_WARNING) << __FUNCTION__ << " type=" << type << ", hint=" << hint;

    (void)stream;

    return 0;
}

int32_t AudioCapturer::OnError(OH_AudioCapturer* stream, OH_AudioStream_Result error)
{
    RTC_LOG(LS_ERROR) << __FUNCTION__ << " error=" << error;

    (void)stream;

    NotifyError(AudioErrorType::GENERAL, "system error");

    return 0;
}

int32_t AudioCapturer::GetAudioSource() const
{
    return options_.source.value_or(AUDIOSTREAM_SOURCE_TYPE_VOICE_COMMUNICATION);
}

OH_AudioStream_State AudioCapturer::GetCurrentState() const
{
    OH_AudioStream_State state = AUDIOSTREAM_STATE_INVALID;
    if (capturer_) {
        OH_AudioCapturer_GetCurrentState(capturer_, &state);
    }

    return state;
}

double AudioCapturer::EstimateLatencyMillis() const
{
    RTC_DCHECK(capturer_);

    return static_cast<double>(framesPerBurst_) / GetSampleRate() * rtc::kNumMillisecsPerSec;
}

uint32_t AudioCapturer::GetOverflowCount() const
{
    uint32_t overflowCount = 0;
    auto ret = OH_AudioCapturer_GetOverflowCount(capturer_, &overflowCount);
    if (ret != AUDIOSTREAM_SUCCESS) {
        RTC_LOG(LS_ERROR) << "Failed to get overflow count: " << ret;
        return 0;
    }

    return overflowCount;
}

bool AudioCapturer::CheckConfiguration(OH_AudioCapturer* capturer)
{
    if (!capturer) {
        RTC_LOG(LS_ERROR) << "Invalid parameter";
        return false;
    }

    int32_t rate;
    OH_RESULT_CHECK(
        OH_AudioCapturer_GetSamplingRate(capturer, &rate),
        NotifyError(AudioErrorType::INIT, "failed to get sample rate"), false);
    if (rate != GetSampleRate()) {
        RTC_LOG(LS_ERROR) << "Stream unable to use requested sample rate";
        NotifyError(AudioErrorType::INIT, "unmatched sample rate");
        return false;
    }

    int32_t channelCount;
    OH_RESULT_CHECK(
        OH_AudioCapturer_GetChannelCount(capturer, &channelCount),
        NotifyError(AudioErrorType::INIT, "failed to get channel count"), false);
    if (channelCount != GetChannelCount()) {
        RTC_LOG(LS_ERROR) << "Stream unable to use requested channel count";
        NotifyError(AudioErrorType::INIT, "unmatched channel count");
        return false;
    }

    OH_AudioStream_SampleFormat sampleFormat;
    OH_RESULT_CHECK(
        OH_AudioCapturer_GetSampleFormat(capturer, &sampleFormat),
        NotifyError(AudioErrorType::INIT, "failed to get sample format"), false);
    if (sampleFormat != AUDIOSTREAM_SAMPLE_S16LE) {
        RTC_LOG(LS_ERROR) << "Stream unable to use requested format";
        NotifyError(AudioErrorType::INIT, "unmatched sample format");
        return false;
    }

    return true;
}

} // namespace webrtc
