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

#ifndef WEBRTC_AUDIO_INPUT_H
#define WEBRTC_AUDIO_INPUT_H

#include <list>
#include <mutex>
#include <string>

#include "api/sequence_checker.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "rtc_base/copy_on_write_buffer.h"

#include "audio_common.h"

namespace webrtc {

struct AudioInputOptions {
    std::optional<int32_t> sampleRate;
    std::optional<int32_t> channelCount;
    std::optional<int32_t> source;
    std::optional<int32_t> format;
    std::optional<bool> useLowLatency;
};

class AudioInput {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void OnAudioInputError(AudioInput* input, AudioErrorType type, const std::string& message) = 0;
        virtual void OnAudioInputStateChange(AudioInput* input, AudioStateType newState) = 0;
        // virtual void OnAudioInputSamplesReady(
        //     int32_t sampleRate, int32_t format, int32_t channelCount, rtc::CopyOnWriteBuffer* data) = 0;
        virtual void OnAudioInputDataReady(
            AudioInput* input, void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs) = 0;
    };

    virtual ~AudioInput() {}

    virtual int32_t Init() = 0;
    virtual int32_t Terminate() = 0;

    virtual int32_t InitRecording() = 0;
    virtual bool RecordingIsInitialized() const = 0;

    virtual int32_t StartRecording() = 0;
    virtual int32_t StopRecording() = 0;
    virtual bool Recording() const = 0;

    virtual int32_t SetMute(bool mute) = 0;

    virtual void RegisterObserver(Observer* obs) = 0;
    virtual void UnregisterObserver(Observer* obs) = 0;

    virtual int32_t GetSampleRate() const = 0;
    virtual int32_t GetChannelCount() const = 0;
    virtual int32_t GetSampleFormat() const = 0;
    virtual bool UseLowLatency() const = 0;

    virtual std::string GetLabel() const
    {
        return "";
    }
};

class AudioInputBase : public AudioInput {
public:
    explicit AudioInputBase(AudioInputOptions options) : options_(std::move(options)) {}
    ~AudioInputBase() override = default;

    void RegisterObserver(Observer* obs) override
    {
        if (!obs) {
            return;
        }

        std::lock_guard<std::mutex> lock(obsMutex_);
        observers_.push_back(obs);
    }

    void UnregisterObserver(Observer* obs) override
    {
        if (!obs) {
            return;
        }

        std::lock_guard<std::mutex> lock(obsMutex_);
        observers_.remove(obs);
    }

    int32_t GetSampleRate() const override
    {
        return options_.sampleRate.value_or(kAudioSampleRate_Default);
    }

    int32_t GetChannelCount() const override
    {
        return options_.channelCount.value_or(kAudioChannelCount_Mono);
    }
    
    int32_t GetSampleFormat() const override
    {
        return options_.format.value_or(AUDIOSTREAM_SAMPLE_S16LE);
    }

    bool UseLowLatency() const override
    {
        return options_.useLowLatency.value_or(false);
    }

    int32_t SetMute(bool mute) override
    {
        mute_ = mute;
        return 0;
    }

protected:
    void NotifyError(AudioErrorType error, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(obsMutex_);
        for (auto& obs : observers_) {
            obs->OnAudioInputError(this, error, message);
        }
    }

    void NotifyStateChange(AudioStateType state)
    {
        std::lock_guard<std::mutex> lock(obsMutex_);
        for (auto& obs : observers_) {
            obs->OnAudioInputStateChange(this, state);
        }
    }

    void NotifyDataReady(void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs)
    {
        std::lock_guard<std::mutex> lock(obsMutex_);
        for (auto& obs : observers_) {
            obs->OnAudioInputDataReady(this, buffer, length, timestampUs, deleyUs);
        }
    }

    const AudioInputOptions options_;

    // Sets all recorded samples to zero if `mute_` is true.
    std::atomic<bool> mute_{false};

    std::mutex obsMutex_;
    std::list<Observer*> observers_;
};

} // namespace webrtc

#endif // WEBRTC_AUDIO_INPUT_H
