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

#include "system_audio_receiver.h"
#include "../audio_device/audio_common.h"

#include "rtc_base/copy_on_write_buffer.h"
#include "rtc_base/logging.h"
#include <bits/alltypes.h>
#include <cmath>

#include <fstream>

namespace webrtc {

std::unique_ptr<SystemAudioReceiver> SystemAudioReceiver::Create()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    AudioInputOptions options;
    options.sampleRate = kAudioSampleRate_48000;
    options.channelCount = kAudioChannelCount_Stereo;
    options.format = AUDIOSTREAM_SAMPLE_S16LE;
    options.source = OH_ALL_PLAYBACK;
    return SystemAudioReceiver::Create(std::move(options));
}

std::unique_ptr<SystemAudioReceiver> SystemAudioReceiver::Create(AudioInputOptions options)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return std::make_unique<SystemAudioReceiver>(std::move(options));
}

SystemAudioReceiver::SystemAudioReceiver(AudioInputOptions options) : AudioInputBase(std::move(options))
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    // Detach from this thread since construction is allowed to happen on a different thread.
    threadChecker_.Detach();
}

SystemAudioReceiver::~SystemAudioReceiver()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    Terminate();
}

int32_t SystemAudioReceiver::Init()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    return 0;
}

int32_t SystemAudioReceiver::Terminate()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    StopRecording();
    threadChecker_.Detach();

    return 0;
}

int32_t SystemAudioReceiver::InitRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (initialized_) {
        // Already initialized.
        return 0;
    }
    RTC_DCHECK(!recording_);

    initialized_ = true;

    return 0;
}

bool SystemAudioReceiver::RecordingIsInitialized() const
{
    RTC_DLOG_F(LS_INFO) << "initialized_ = " << initialized_;
    return initialized_;
}

int32_t SystemAudioReceiver::StartRecording()
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

    recording_ = true;

    NotifyStateChange(AudioStateType::START);

    return 0;
}

int32_t SystemAudioReceiver::StopRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());
    if (!initialized_ || !recording_) {
        return 0;
    }

    initialized_ = false;
    recording_ = false;

    NotifyStateChange(AudioStateType::STOP);

    return 0;
}

bool SystemAudioReceiver::Recording() const
{
    RTC_DLOG_F(LS_VERBOSE) << "recording_ = " << recording_;
    return recording_;
}

std::string SystemAudioReceiver::GetLabel() const
{
    return "System Audio";
}

int32_t SystemAudioReceiver::GetAudioSource() const
{
    return options_.source.value_or(OH_ALL_PLAYBACK);
}

void SystemAudioReceiver::OnStarted(bool success)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    NotifyStateChange(AudioStateType::START);
}

void SystemAudioReceiver::OnStopped()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    NotifyStateChange(AudioStateType::STOP);
}

void SystemAudioReceiver::OnData(void* buffer, int32_t length, int64_t timestampUs)
{
    RTC_DLOG_F(LS_VERBOSE) << "buffer=" << buffer << ", length=" << length;
    if (!recording_) {
        RTC_DLOG(LS_VERBOSE) << "Not recording";
        return;
    }
    
    if (mute_) {
        auto it = static_cast<uint8_t*>(buffer);
        std::fill(it, it + length, 0);
    }

    NotifyDataReady(buffer, length, timestampUs, 0);
}

void SystemAudioReceiver::OnError(int32_t errorCode, const std::string& message)
{
    NotifyError(AudioErrorType::GENERAL, message);
}

} // namespace webrtc
