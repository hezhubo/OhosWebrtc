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

#include "audio_renderer.h"
#include "audio_common.h"

#include "rtc_base/logging.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "system_wrappers/include/field_trial.h"

namespace webrtc {

static constexpr int halfDen = 2;
std::unique_ptr<AudioRenderer>
AudioRenderer::Create(AudioOutputOptions options)
{
    return std::make_unique<AudioRenderer>(std::move(options));
}

int32_t AudioRenderer::OnWriteData1(OH_AudioRenderer* renderer, void* userData, void* buffer, int32_t length)
{
    if (!userData) {
        return -1;
    }

    auto self = (AudioRenderer*)userData;
    return self->OnWriteData(renderer, buffer, length);
}

int32_t AudioRenderer::OnStreamEvent1(OH_AudioRenderer* renderer, void* userData, OH_AudioStream_Event event)
{
    if (!userData) {
        return -1;
    }

    auto self = (AudioRenderer*)userData;
    return self->OnStreamEvent(renderer, event);
}

int32_t AudioRenderer::OnInterruptEvent1(
    OH_AudioRenderer* renderer, void* userData, OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint)
{
    if (!userData) {
        return -1;
    }

    auto self = (AudioRenderer*)userData;
    return self->OnInterruptEvent(renderer, type, hint);
}

int32_t AudioRenderer::OnError1(OH_AudioRenderer* renderer, void* userData, OH_AudioStream_Result error)
{
    if (!userData) {
        return -1;
    }

    auto self = (AudioRenderer*)userData;
    return self->OnError(renderer, error);
}

void AudioRenderer::OnDeviceChangeCallback1(
    OH_AudioRenderer* renderer, void* userData, OH_AudioStream_DeviceChangeReason reason)
{
    if (!userData) {
        return;
    }

    auto self = (AudioRenderer*)userData;
    return self->OnDeviceChangeCallback(renderer, reason);
}

AudioRenderer::AudioRenderer(AudioOutputOptions options)
    : options_(std::move(options))
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    threadChecker_.Detach();
}

AudioRenderer::~AudioRenderer()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    Terminate();

    RTC_LOG(LS_INFO) << "Detected underflows: " << underflowCount_;
}

int32_t AudioRenderer::Init()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    return 0;
}

int32_t AudioRenderer::Terminate()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    StopPlayout();
    threadChecker_.Detach();
    OH_RESULT_CHECK(OH_AudioRenderer_Release(renderer_), -1);

    initialized_ = false;

    return 0;
}

int32_t AudioRenderer::InitPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (initialized_) {
        // Already initialized.
        return 0;
    }
    RTC_DCHECK(!playing_);

    OH_AudioStreamBuilder* builder = nullptr;
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_RENDERER),
        NotifyError(AudioErrorType::INIT, "System error"), -1);

    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetRendererInfo(builder, static_cast<OH_AudioStream_Usage>(GetUsage())),
        NotifyError(AudioErrorType::INIT, "failed to set renderer info"), -1);
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetSamplingRate(builder, GetSampleRate()),
        NotifyError(AudioErrorType::INIT, "failed to set sample rate"), -1);
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetChannelCount(builder, GetChannelCount()),
        NotifyError(AudioErrorType::INIT, "failed to set channel count"), -1);
    // Use s16le in webrtc
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetSampleFormat(builder, AUDIOSTREAM_SAMPLE_S16LE),
        NotifyError(AudioErrorType::INIT, "failed to set sample format"), -1);
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetLatencyMode(
            builder, UseLowLatency() ? AUDIOSTREAM_LATENCY_MODE_FAST : AUDIOSTREAM_LATENCY_MODE_NORMAL),
        NotifyError(AudioErrorType::INIT, "failed to set latency mode"), -1);

    OH_AudioRenderer_Callbacks callbacks;
    callbacks.OH_AudioRenderer_OnWriteData = OnWriteData1;
    callbacks.OH_AudioRenderer_OnStreamEvent = OnStreamEvent1;
    callbacks.OH_AudioRenderer_OnInterruptEvent = OnInterruptEvent1;
    callbacks.OH_AudioRenderer_OnError = OnError1;
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetRendererCallback(builder, callbacks, this),
        NotifyError(AudioErrorType::INIT, "failed to set renderer callback"), -1);

    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_SetRendererOutputDeviceChangeCallback(builder, OnDeviceChangeCallback1, this),
        NotifyError(AudioErrorType::INIT, "failed to set renderer callback"), -1);

    OH_AudioRenderer* stream = nullptr;
    OH_RESULT_CHECK(
        OH_AudioStreamBuilder_GenerateRenderer(builder, &stream),
        NotifyError(AudioErrorType::INIT, "failed to generate renderer"), -1);

    if (!CheckConfiguration(stream)) {
        return -1;
    }

    renderer_ = stream;
    initialized_ = true;

    RTC_DLOG(LS_VERBOSE) << "current state: " << StateToString(GetCurrentState());

    return 0;
}

bool AudioRenderer::PlayoutIsInitialized() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return initialized_;
}

int32_t AudioRenderer::StartPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (playing_) {
        // Already playing.
        return 0;
    }

    if (!initialized_) {
        RTC_DLOG(LS_WARNING) << "Playout can not start since InitPlayout must succeed first";
        return 0;
    }

    if (fineAudioBuffer_) {
        fineAudioBuffer_->ResetPlayout();
    }

    OH_AudioStream_State state = GetCurrentState();
    if (state != AUDIOSTREAM_STATE_PREPARED && state != AUDIOSTREAM_STATE_STOPPED) {
        RTC_LOG(LS_ERROR) << "Invalid state: " << StateToString(state);
        NotifyError(AudioErrorType::START_STATE_MISMATCH, std::string("Invalid state: ") + StateToString(state));
        return -1;
    }

    OH_RESULT_CHECK(
        OH_AudioRenderer_Start(renderer_), NotifyError(AudioErrorType::START_EXCEPTION, "System error"), -1);
    RTC_DLOG(LS_VERBOSE) << "Current state: " << StateToString(GetCurrentState());

    underflowCount_ = GetUnderflowCount();
    playing_ = true;

    NotifyStateChange(AudioStateType::START);

    return 0;
}

int32_t AudioRenderer::StopPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (!initialized_ || !playing_) {
        return 0;
    }

    OH_RESULT_CHECK(OH_AudioRenderer_Stop(renderer_), -1);
    playing_ = false;

    NotifyStateChange(AudioStateType::STOP);

    return 0;
}

bool AudioRenderer::Playing() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return playing_;
}

void AudioRenderer::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    RTC_CHECK(audioBuffer);
    audioBuffer->SetPlayoutSampleRate(GetSampleRate());
    audioBuffer->SetPlayoutChannels(GetChannelCount());

    fineAudioBuffer_ = std::make_unique<FineAudioBuffer>(audioBuffer);
}

int32_t AudioRenderer::PlayoutDelay(uint16_t* delayMs) const
{
    if (!delayMs) {
        return 0;
    }

    RTC_DLOG(LS_VERBOSE) << "delayMs=" << *delayMs;

    // Best guess we can do is to use half of the estimated total delay.
    *delayMs = kHighLatencyModeDelayEstimateInMilliseconds / halfDen;
    RTC_DCHECK_GT(*delayMs, 0);

    return 0;
}

int AudioRenderer::GetPlayoutUnderrunCount()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return 0;
}

void AudioRenderer::RegisterObserver(Observer* obs)
{
    if (!obs) {
        return;
    }

    std::lock_guard<std::mutex> lock(obsMutex_);
    observers_.insert(obs);
}

void AudioRenderer::UnregisterObserver(Observer* obs)
{
    if (!obs) {
        return;
    }

    std::lock_guard<std::mutex> lock(obsMutex_);
    observers_.erase(obs);
}

int32_t AudioRenderer::GetSampleRate() const
{
    return options_.sampleRate.value_or(kAudioSampleRate_Default);
}

int32_t AudioRenderer::GetChannelCount() const
{
    return options_.channelCount.value_or(kAudioChannelCount_Mono);
}

bool AudioRenderer::UseLowLatency() const
{
    return options_.useLowLatency.value_or(false);
}

int32_t AudioRenderer::SetMute(bool mute)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    mute_ = mute;
    return 0;
}

int32_t AudioRenderer::OnWriteData(OH_AudioRenderer* renderer, void* buffer, int32_t length)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " length=" << length;

    (void)renderer;
    RTC_DCHECK_RUNS_SERIALIZED(&dataRaceChecker_);

    const uint32_t underflowCount = GetUnderflowCount();
    if (underflowCount_ < underflowCount) {
        RTC_LOG(LS_ERROR) << "Underflow detected: " << underflowCount;
        underflowCount_ = underflowCount;
    }

    auto latencyMillis = EstimateLatencyMillis();
    RTC_DLOG(LS_VERBOSE) << "Estimate latencyMillis=" << latencyMillis;

    fineAudioBuffer_->GetPlayoutData(
        rtc::MakeArrayView(static_cast<int16_t*>(buffer), length / sizeof(int16_t)),
        static_cast<int>(latencyMillis + kHalfSec));

    if (mute_) {
        auto it = static_cast<uint8_t*>(buffer);
        std::fill(it, it + length, 0);
    }

    return 0;
}

int32_t AudioRenderer::OnStreamEvent(OH_AudioRenderer* renderer, OH_AudioStream_Event event)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " event=" << event;

    (void)renderer;

    return 0;
}

int32_t AudioRenderer::OnInterruptEvent(
    OH_AudioRenderer* renderer, OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint)
{
    RTC_LOG(LS_WARNING) << __FUNCTION__ << " type=" << type << ", hint=" << hint;

    (void)renderer;

    return 0;
}

int32_t AudioRenderer::OnError(OH_AudioRenderer* renderer, OH_AudioStream_Result error)
{
    RTC_LOG(LS_ERROR) << __FUNCTION__ << " error=" << error;

    (void)renderer;

    NotifyError(AudioErrorType::GENERAL, "System error");

    return 0;
}

void AudioRenderer::OnDeviceChangeCallback(OH_AudioRenderer* renderer, OH_AudioStream_DeviceChangeReason reason)
{
    (void)renderer;
    RTC_LOG(LS_INFO) << __FUNCTION__ << " reason=" << reason;
}

int32_t AudioRenderer::GetUsage() const
{
    return options_.usage.value_or(AUDIOSTREAM_USAGE_VOICE_COMMUNICATION);
}

OH_AudioStream_State AudioRenderer::GetCurrentState() const
{
    OH_AudioStream_State state = AUDIOSTREAM_STATE_INVALID;
    if (renderer_) {
        OH_AudioRenderer_GetCurrentState(renderer_, &state);
    }

    return state;
}

double AudioRenderer::EstimateLatencyMillis() const
{
    RTC_DCHECK(renderer_);

    double latencyMillis = 0.0;

    int64_t framePosition = 0;
    int64_t timestamp = 0;
    OH_AudioStream_Result result =
        OH_AudioRenderer_GetTimestamp(renderer_, CLOCK_MONOTONIC, &framePosition, &timestamp);
    if (result != AUDIOSTREAM_SUCCESS) {
        return latencyMillis;
    }
    RTC_DLOG(LS_VERBOSE) << "framePosition=" << framePosition;
    RTC_DLOG(LS_VERBOSE) << "timestamp=" << timestamp;

    int64_t framesWritten = 0;
    result = OH_AudioRenderer_GetFramesWritten(renderer_, &framesWritten);
    if (result != AUDIOSTREAM_SUCCESS) {
        return latencyMillis;
    }
    RTC_DLOG(LS_VERBOSE) << "framesWritten=" << framesWritten;

    int64_t frameIndexDelta = framesWritten - framePosition;
    latencyMillis = static_cast<double>(frameIndexDelta * rtc::kNumMillisecsPerSec) / GetSampleRate();

    return latencyMillis;
}

uint32_t AudioRenderer::GetUnderflowCount() const
{
    uint32_t underflowCount = 0;
    auto ret = OH_AudioRenderer_GetUnderflowCount(renderer_, &underflowCount);
    if (ret != AUDIOSTREAM_SUCCESS) {
        RTC_LOG(LS_ERROR) << "Failed to get underflow count: " << ret;
        return 0;
    }

    return underflowCount;
}

bool AudioRenderer::CheckConfiguration(OH_AudioRenderer* renderer)
{
    if (!renderer) {
        RTC_LOG(LS_ERROR) << "Invalid parameter";
        return false;
    }
    
    int32_t rate;
    OH_RESULT_CHECK(
        OH_AudioRenderer_GetSamplingRate(renderer, &rate),
        NotifyError(AudioErrorType::INIT, "failed to get sampling rate"), false);
    if (rate != GetSampleRate()) {
        RTC_LOG(LS_ERROR) << "Stream unable to use requested sample rate";
        NotifyError(AudioErrorType::INIT, "unmatched sampling rate");
        return false;
    }

    int32_t channelCount;
    OH_RESULT_CHECK(
        OH_AudioRenderer_GetChannelCount(renderer, &channelCount),
        NotifyError(AudioErrorType::INIT, "failed to get channel count"), false);
    if (channelCount != GetChannelCount()) {
        RTC_LOG(LS_ERROR) << "Stream unable to use requested channel count";
        NotifyError(AudioErrorType::INIT, "unmatched channel count");
        return false;
    }

    OH_AudioStream_SampleFormat sampleFormat;
    OH_RESULT_CHECK(
        OH_AudioRenderer_GetSampleFormat(renderer, &sampleFormat),
        NotifyError(AudioErrorType::INIT, "failed to get sample format"), false);
    if (sampleFormat != AUDIOSTREAM_SAMPLE_S16LE) {
        RTC_LOG(LS_ERROR) << "Stream unable to use requested format";
        NotifyError(AudioErrorType::INIT, "unmatched sample format");
        return false;
    }

    return true;
}

void AudioRenderer::NotifyError(AudioErrorType error, const std::string& message)
{
    std::lock_guard<std::mutex> lock(obsMutex_);
    for (auto& obs : observers_) {
        obs->OnAudioOutputError(this, error, message);
    }
}

void AudioRenderer::NotifyStateChange(AudioStateType state)
{
    std::lock_guard<std::mutex> lock(obsMutex_);
    for (auto& obs : observers_) {
        obs->OnAudioOutputStateChange(this, state);
    }
}

} // namespace webrtc
