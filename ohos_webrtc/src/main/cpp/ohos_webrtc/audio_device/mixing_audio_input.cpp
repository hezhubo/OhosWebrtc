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

#include "mixing_audio_input.h"
#include "audio_common.h"

#include "common_audio/ring_buffer.h"
#include "common_audio/resampler/include/push_resampler.h"
#include "modules/audio_mixer/audio_mixer_impl.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/copy_on_write_buffer.h"
#include "rtc_base/logging.h"

#include <cmath>
#include <queue>
#include <fstream>
#include <condition_variable>

namespace webrtc {

namespace {

// Only accepts 10 ms frames.
constexpr int32_t kFrameDurationInMs = 10;

constexpr int32_t kBufferDurationInMs = 200;

} // namespace

class AudioMixerSourceAdapter : public AudioMixer::Source, public AudioInput::Observer {
public:
    explicit AudioMixerSourceAdapter(std::shared_ptr<AudioInput> input, int ssrc = 0)
        : input_(std::move(input)), ssrc_(ssrc)
    {
        RTC_DLOG(LS_INFO) << __FUNCTION__;

        RTC_DCHECK(input_);

        buffer_ = WebRtc_CreateBuffer(
            input_->GetSampleRate() * input_->GetChannelCount() / rtc::kNumMillisecsPerSec * kBufferDurationInMs,
            sizeof(int16_t));
        RTC_CHECK(buffer_);

        input_->RegisterObserver(this);
    }

    ~AudioMixerSourceAdapter() override
    {
        input_->UnregisterObserver(this);

        std::lock_guard<std::mutex> lock(mutex_);
        WebRtc_FreeBuffer(buffer_);
    }

    const std::shared_ptr<AudioInput>& GetInput() const
    {
        return input_;
    }

protected:
    // AudioInput::Observer Implementation.
    AudioFrameInfo GetAudioFrameWithInfo(int targetSampleRate, AudioFrame* frame) override
    {
        frame->samples_per_channel_ = targetSampleRate / rtc::kNumMillisecsPerSec * kFrameDurationInMs;
        frame->sample_rate_hz_ = targetSampleRate;
        frame->num_channels_ = input_->GetChannelCount();

        // Read 10ms
        const size_t numToRead =
            input_->GetChannelCount() * input_->GetSampleRate() / rtc::kNumMillisecsPerSec * kFrameDurationInMs;

        std::unique_lock<std::mutex> lock(mutex_);
        dirtyCondVar_.wait(
            lock, [this, numToRead] { return (WebRtc_available_read(buffer_) >= numToRead) || !running_; });
        if (!running_) {
            // Stopped
            return AudioFrameInfo::kError;
        }

        if (targetSampleRate != input_->GetSampleRate()) {
            if (!resampler_) {
                resampler_ = std::make_unique<PushResampler<int16_t>>();
            }
            resampler_->InitializeIfNeeded(input_->GetSampleRate(), targetSampleRate, input_->GetChannelCount());
            if (tempData_.size() < numToRead) {
                tempData_.resize(numToRead);
            }

            void* data = nullptr;
            const size_t read = WebRtc_ReadBuffer(buffer_, &data, tempData_.data(), numToRead);
            RTC_CHECK_EQ(read, numToRead);
            resampler_->Resample(
                static_cast<int16_t*>(data), numToRead, frame->mutable_data(), frame->max_16bit_samples());
        } else {
            const size_t read = WebRtc_ReadBuffer(buffer_, nullptr, frame->mutable_data(), numToRead);
            RTC_CHECK_EQ(read, numToRead);
        }

        freeCondVar_.notify_all();

        return AudioFrameInfo::kNormal;
    }

    int Ssrc() const override
    {
        return ssrc_;
    }

    int PreferredSampleRate() const override
    {
        return input_->GetSampleRate();
    }

    // AudioInput::Observer Implementation
    void OnAudioInputError(AudioInput* input, AudioErrorType type, const std::string& message) override
    {
        // Error
        RTC_LOG(LS_ERROR) << "[" << ssrc_ << "]" << "Error: " << type << ", " << message;
    }

    void OnAudioInputStateChange(AudioInput* input, AudioStateType newState) override
    {
        switch (newState) {
            case AudioStateType::START: {
                RTC_LOG(LS_INFO) << "[" << ssrc_ << "]" << "Start";
                std::lock_guard<std::mutex> lock(mutex_);
                running_ = true;
                WebRtc_InitBuffer(buffer_);
                break;
            }
            case AudioStateType::STOP: {
                RTC_LOG(LS_INFO) << "[" << ssrc_ << "]" << "Stop";
                std::lock_guard<std::mutex> lock(mutex_);
                running_ = false;
                dirtyCondVar_.notify_all();
                freeCondVar_.notify_all();
                break;
            }
            default:
                break;
        }
    }

    void OnAudioInputDataReady(
        AudioInput* input, void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs) override
    {
        RTC_DLOG(LS_VERBOSE) << "[" << ssrc_ << "]" << __FUNCTION__;

        size_t numToWritten = length / sizeof(int16_t);

        std::unique_lock<std::mutex> lock(mutex_);
        auto ret = freeCondVar_.wait_for(lock, std::chrono::milliseconds(kFrameDurationInMs), [this, numToWritten] {
            return (WebRtc_available_write(buffer_) >= numToWritten) || !running_;
        });
        if (!running_) {
            // Stopped
            return;
        } else if (!ret) {
            // Timeout
            RTC_LOG(LS_WARNING) << "[" << ssrc_ << "]" << "Timeout";
            return;
        }

        const size_t written = WebRtc_WriteBuffer(buffer_, buffer, numToWritten);
        RTC_CHECK_EQ(written, numToWritten);
        dirtyCondVar_.notify_all();
    }

private:
    std::shared_ptr<AudioInput> input_;
    const int ssrc_;
    bool running_{false};
    RingBuffer* buffer_{};
    std::mutex mutex_;
    std::condition_variable dirtyCondVar_;
    std::condition_variable freeCondVar_;

    std::unique_ptr<PushResampler<int16_t>> resampler_;
    std::vector<int16_t> tempData_;
};

int MixingAudioInput::CalculateOutputSampleRate(const std::list<std::shared_ptr<AudioInput>>& inputs)
{
    if (inputs.empty()) {
        return kAudioSampleRate_48000;
    }

    int32_t maxSampleRate = 0;
    for (const auto& input : inputs) {
        maxSampleRate = std::max(maxSampleRate, input->GetSampleRate());
    }

    RTC_DCHECK_LE(AudioProcessing::NativeRate::kSampleRate8kHz, maxSampleRate);
    RTC_DCHECK_GE(AudioProcessing::NativeRate::kSampleRate48kHz, maxSampleRate);

    const auto* it = std::lower_bound(
        std::begin(AudioProcessing::kNativeSampleRatesHz), std::end(AudioProcessing::kNativeSampleRatesHz),
        maxSampleRate);
    RTC_DCHECK(it != std::end(AudioProcessing::kNativeSampleRatesHz));

    return *it;
}

int MixingAudioInput::CalculateOutputChannelCount(const std::list<std::shared_ptr<AudioInput>>& inputs)
{
    if (inputs.empty()) {
        return kAudioChannelCount_Stereo;
    }

    int32_t maxChannelCount = 0;
    for (const auto& input : inputs) {
        maxChannelCount = std::max(maxChannelCount, input->GetChannelCount());
    }

    RTC_DCHECK_LE(kAudioChannelCount_Mono, maxChannelCount);
    RTC_DCHECK_GE(kAudioChannelCount_Stereo, maxChannelCount);

    return maxChannelCount;
}

std::unique_ptr<MixingAudioInput> MixingAudioInput::Create(AudioInputOptions options)
{
    return std::make_unique<MixingAudioInput>(std::move(options));
}

MixingAudioInput::MixingAudioInput(AudioInputOptions options)
    : AudioInputBase(std::move(options)), mixer_(webrtc::AudioMixerImpl::Create()), thread_(rtc::Thread::Create())
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    // Detach from this thread since construction is allowed to happen on a different thread.
    threadChecker_.Detach();

    thread_->SetName("mixing-audio-input", this);
}

MixingAudioInput::~MixingAudioInput()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    Terminate();
}

int32_t MixingAudioInput::Init()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());

    {
        std::lock_guard<std::mutex> lock(sourcesMutex_);
        for (auto& source : sources_) {
            source->GetInput()->Init();
        }
    }

    return 0;
}

int32_t MixingAudioInput::Terminate()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());

    {
        std::lock_guard<std::mutex> lock(sourcesMutex_);
        for (auto& source : sources_) {
            source->GetInput()->Terminate();
        }
    }

    threadChecker_.Detach();

    return 0;
}

int32_t MixingAudioInput::InitRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (initialized_) {
        // Already initialized.
        return 0;
    }
    RTC_DCHECK(!recording_);

    {
        std::lock_guard<std::mutex> lock(sourcesMutex_);
        for (auto& source : sources_) {
            source->GetInput()->InitRecording();
        }
    }

    initialized_ = true;

    return 0;
}

bool MixingAudioInput::RecordingIsInitialized() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << " initialized_ = " << initialized_;
    return initialized_;
}

int32_t MixingAudioInput::StartRecording()
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

    {
        std::lock_guard<std::mutex> lock(sourcesMutex_);
        for (auto& source : sources_) {
            source->GetInput()->StartRecording();
        }
    }

    recording_ = true;

    thread_->Start();
    thread_->PostTask([this] { DoMix(); });

    NotifyStateChange(AudioStateType::START);

    return 0;
}

int32_t MixingAudioInput::StopRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (!initialized_ || !recording_) {
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(sourcesMutex_);
        for (auto& source : sources_) {
            source->GetInput()->StopRecording();
        }
    }

    initialized_ = false;
    recording_ = false;

    thread_->Stop();

    NotifyStateChange(AudioStateType::STOP);

    return 0;
}

bool MixingAudioInput::Recording() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << " recording_ = " << recording_;
    return recording_;
}

bool MixingAudioInput::AddAudioInput(std::shared_ptr<AudioInput> input)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << ": " << input.get();

    RTC_DCHECK(input);
    if (!input) {
        RTC_LOG(LS_WARNING) << "Invalid parameter";
        return false;
    }

    std::lock_guard<std::mutex> lock(sourcesMutex_);
    auto it = std::find_if(sources_.begin(), sources_.end(), [&input](const auto& e) {
        return e->GetInput() == input;
    });
    if (it != sources_.end()) {
        RTC_LOG(LS_WARNING) << "Input already added";
        return false;
    }

    auto adapter = std::make_shared<AudioMixerSourceAdapter>(input, sources_.size());
    mixer_->AddSource(adapter.get());
    sources_.push_back(adapter);

    return true;
}

bool MixingAudioInput::RemoveAudioInput(std::shared_ptr<AudioInput> input)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << ": " << input.get();

    RTC_DCHECK(input);

    std::lock_guard<std::mutex> lock(sourcesMutex_);
    auto it =
        std::find_if(sources_.begin(), sources_.end(), [&input](const auto& e) { return e->GetInput() == input; });
    if (it == sources_.end()) {
        RTC_LOG(LS_WARNING) << "Input not present";
        return false;
    }

    sources_.erase(it);
    mixer_->RemoveSource(it->get());

    return true;
}

void MixingAudioInput::DoMix()
{
    AudioFrame frame;

    while (recording_) {
        auto begin = std::chrono::steady_clock::now();

        mixer_->Mix(GetChannelCount(), &frame);
        RTC_CHECK_EQ(GetChannelCount(), frame.num_channels());
        RTC_CHECK_EQ(GetSampleRate(), frame.sample_rate_hz());
        RTC_CHECK_EQ(GetSampleRate() / rtc::kNumMillisecsPerSec * kFrameDurationInMs, frame.samples_per_channel());

        if (mute_) {
            frame.Mute();
        }

        // Timestamp and deley
        NotifyDataReady(
            (void*)frame.data(), frame.num_channels() * frame.samples_per_channel() * sizeof(int16_t), 0, 0);
    }
}

void MixingAudioInput::OnAudioInputError(AudioInput* input, AudioErrorType type, const std::string& message)
{
    NotifyError(type, message);
}

void MixingAudioInput::OnAudioInputStateChange(AudioInput* input, AudioStateType newState)
{
    NotifyStateChange(newState);
}

void MixingAudioInput::OnAudioInputDataReady(
    AudioInput* input, void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs)
{
    // Do nothing
}

} // namespace webrtc
