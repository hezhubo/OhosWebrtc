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

#include "ohos_audio_device_module.h"
#include "audio_common.h"
#include "audio_capturer.h"
#include "audio_renderer.h"
#include "mixing_audio_input.h"

#include <memory>
#include <utility>

#include "api/make_ref_counted.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

const char kClassName[] = "AudioDeviceModule";

const char kAttributeNameOnCapturerError[] = "oncapturererror";
const char kAttributeNameOnCapturerStateChange[] = "oncapturerstatechange";
const char kAttributeNameOnCapturerSamplesReady[] = "oncapturersamplesready";
const char kAttributeNameOnRendererError[] = "onrenderererror";
const char kAttributeNameOnRendererStateChange[] = "onrendererstatechange";

const char kMethodNameSetSpeakerMute[] = "setSpeakerMute";
const char kMethodNameSetMicrophoneMute[] = "setMicrophoneMute";
const char kMethodNameSetNoiseSuppressorEnabled[] = "setNoiseSuppressorEnabled";
const char kMethodNameIsBuiltInAcousticEchoCancelerSupported[] = "isBuiltInAcousticEchoCancelerSupported";
const char kMethodNameIsBuiltInNoiseSuppressorSupported[] = "isBuiltInNoiseSuppressorSupported";
const char kMethodNameToJson[] = "toJSON";

const char kEventNameCapturerError[] = "capturererror";
const char kEventNameCapturerStateChange[] = "capturerstatechange";
const char kEventNameCapturerSamplesReady[] = "capturersamplesready";
const char kEventNameRendererError[] = "renderererror";
const char kEventNameRendererStateChange[] = "rendererstatechange";

const char kAttributeNameAudioSource[] = "audioSource";
const char kAttributeNameAudioFormat[] = "audioFormat";
const char kAttributeNameInputSampleRate[] = "inputSampleRate";
const char kAttributeNameUseStereoInput[] = "useStereoInput";
const char kAttributeNameOutputSampleRate[] = "outputSampleRate";
const char kAttributeNameUseStereoOutput[] = "useStereoOutput";
const char kAttributeNameRendererUsage[] = "rendererUsage";
const char kAttributeNameUseLowLatency[] = "useLowLatency";
const char kAttributeNameUseHardwareAcousticEchoCanceler[] = "useHardwareAcousticEchoCanceler";
const char kAttributeNameUseHardwareNoiseSuppressor[] = "useHardwareNoiseSuppressor";

const char kEnumAudioErrorTypeInit[] = "init";
const char kEnumAudioErrorTypeStartException[] = "start-exception";
const char kEnumAudioErrorTypeStartStateMismatch[] = "start-state-mismatch";
const char kEnumAudioErrorTypeGeneral[] = "general";

const char kEnumAudioStateStart[] = "start";
const char kEnumAudioStateStop[] = "stop";

const char* AudioErrorTypeToString(AudioErrorType type)
{
    switch (type) {
        case AudioErrorType::INIT:
            return kEnumAudioErrorTypeInit;
        case AudioErrorType::START_EXCEPTION:
            return kEnumAudioErrorTypeStartException;
        case AudioErrorType::START_STATE_MISMATCH:
            return kEnumAudioErrorTypeStartStateMismatch;
        case AudioErrorType::GENERAL:
            return kEnumAudioErrorTypeGeneral;
        default:
            break;
    }

    return "unknown";
}

const char* AudioStateToString(AudioStateType state)
{
    switch (state) {
        case AudioStateType::START:
            return kEnumAudioStateStart;
        case AudioStateType::STOP:
            return kEnumAudioStateStop;
        default:
            break;
    }

    return "unknown";
}

OhosAudioDeviceModule::OhosAudioDeviceModule(
    AudioInputOptions inputOptions, AudioOutputOptions outputOptions, AudioDeviceModule::AudioLayer audioLayer)
    : audioLayer_(audioLayer),
      defaultInput_(AudioCapturer::Create(inputOptions)),
      output_(AudioRenderer::Create(outputOptions)),
      isStereoRecordSupported_(inputOptions.channelCount > 1),
      isStereoPlayoutSupported_(outputOptions.channelCount > 1),
      taskQueueFactory_(CreateDefaultTaskQueueFactory())
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(defaultInput_);
    RTC_DCHECK(output_);

    output_->RegisterObserver(this);
    threadChecker_.Detach();
}

OhosAudioDeviceModule::~OhosAudioDeviceModule()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    output_->UnregisterObserver(this);
    if (input_) {
        input_->UnregisterObserver(this);
    }
}

void OhosAudioDeviceModule::AddAudioInput(std::shared_ptr<AudioInput> input)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (!input) {
        return;
    }

    RTC_DCHECK(!Recording());
    if (Recording()) {
        RTC_LOG(LS_WARNING) << "Can not add input while recording";
        return;
    }

    std::lock_guard<std::mutex> lock(mut_);
    if (std::find(inputs_.begin(), inputs_.end(), input) != inputs_.end()) {
        RTC_LOG(LS_INFO) << "The audio input already added";
        return;
    }

    inputs_.push_back(input);
}

void OhosAudioDeviceModule::RemoveAudioInput(std::shared_ptr<AudioInput> input)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(input);
    RTC_DCHECK(!Recording());
    if (Recording()) {
        RTC_LOG(LS_WARNING) << "Can not add input while recording";
        return;
    }

    std::lock_guard<std::mutex> lock(mut_);
    inputs_.remove(input);
}

rtc::scoped_refptr<OhosLocalAudioSource> OhosAudioDeviceModule::CreateAudioSource(
    cricket::AudioOptions options, std::shared_ptr<AudioInput> audioInput)
{
    if (!audioInput) {
        // Use default
        audioInput = defaultInput_;
    }

    return OhosLocalAudioSource::Create(std::move(options), std::move(audioInput));
}

void OhosAudioDeviceModule::RegisterInputObserver(AudioInput::Observer* obs)
{
    RTC_DCHECK(obs);
    if (obs) {
        std::lock_guard<std::mutex> lock(inputObsMutex_);
        inputObservers_.insert(obs);
    }
}

void OhosAudioDeviceModule::UnregisterInputObserver(AudioInput::Observer* obs)
{
    RTC_DCHECK(obs);
    if (obs) {
        std::lock_guard<std::mutex> lock(inputObsMutex_);
        inputObservers_.erase(obs);
    }
}

void OhosAudioDeviceModule::RegisterOutputObserver(AudioOutput::Observer* obs)
{
    RTC_DCHECK(obs);
    if (obs) {
        std::lock_guard<std::mutex> lock(outputObsMutex_);
        outputObservers_.insert(obs);
    }
}

void OhosAudioDeviceModule::UnregisterOutputObserver(AudioOutput::Observer* obs)
{
    RTC_DCHECK(obs);
    if (obs) {
        std::lock_guard<std::mutex> lock(outputObsMutex_);
        outputObservers_.erase(obs);
    }
}

int32_t OhosAudioDeviceModule::ActiveAudioLayer(AudioDeviceModule::AudioLayer* audioLayer) const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *audioLayer = audioLayer_;
    return 0;
}

int32_t OhosAudioDeviceModule::RegisterAudioCallback(AudioTransport* audioCallback)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return audioDeviceBuffer_->RegisterAudioCallback(audioCallback);
}

int32_t OhosAudioDeviceModule::Init()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    RTC_DCHECK(threadChecker_.IsCurrent());

    if (initialized_) {
        return 0;
    }

    audioDeviceBuffer_ = std::make_unique<AudioDeviceBuffer>(taskQueueFactory_.get());
    output_->AttachAudioBuffer(audioDeviceBuffer_.get());

    InitStatus status;
    if (output_->Init() != 0) {
        status = InitStatus::PLAYOUT_ERROR;
    } else {
        initialized_ = true;
        status = InitStatus::OK;
    }

    if (status != InitStatus::OK) {
        RTC_LOG(LS_ERROR) << "Audio device initialization failed.";
        return -1;
    }

    return 0;
}

int32_t OhosAudioDeviceModule::Terminate()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (!initialized_) {
        return 0;
    }

    RTC_DCHECK(threadChecker_.IsCurrent());

    int32_t err = 0;
    if (input_) {
        err = input_->Terminate();
        input_->UnregisterObserver(this);
    }

    err |= output_->Terminate();
    if (err != 0) {
        RTC_LOG(LS_ERROR) << "error: " << err;
    }

    initialized_ = false;
    threadChecker_.Detach();
    audioDeviceBuffer_.reset(nullptr);

    return err;
}

bool OhosAudioDeviceModule::Initialized() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << ":" << initialized_;

    return initialized_;
}

int16_t OhosAudioDeviceModule::PlayoutDevices()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return 1;
}

int16_t OhosAudioDeviceModule::RecordingDevices()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return 1;
}

int32_t OhosAudioDeviceModule::PlayoutDeviceName(uint16_t index, char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{
    (void)index;
    (void)name;
    (void)guid;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::RecordingDeviceName(uint16_t index,
    char name[kAdmMaxDeviceNameSize], char guid[kAdmMaxGuidSize])
{
    (void)index;
    (void)name;
    (void)guid;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::SetPlayoutDevice(uint16_t index)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << "(" << index << ")";

    // no effect
    return 0;
}

int32_t OhosAudioDeviceModule::SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType device)
{
    (void)device;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::SetRecordingDevice(uint16_t index)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << "(" << index << ")";

    // no effect
    return 0;
}

int32_t OhosAudioDeviceModule::SetRecordingDevice(AudioDeviceModule::WindowsDeviceType device)
{
    (void)device;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::PlayoutIsAvailable(bool* available)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *available = true;
    return 0;
}

int32_t OhosAudioDeviceModule::InitPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (!initialized_) {
        return -1;
    }

    if (PlayoutIsInitialized()) {
        return 0;
    }

    int32_t result = output_->InitPlayout();
    RTC_DLOG(LS_INFO) << "output: " << result;

    return result;
}

bool OhosAudioDeviceModule::PlayoutIsInitialized() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return output_->PlayoutIsInitialized();
}

int32_t OhosAudioDeviceModule::RecordingIsAvailable(bool* available)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *available = true;
    return 0;
}

int32_t OhosAudioDeviceModule::InitRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (!initialized_) {
        return -1;
    }

    if (RecordingIsInitialized()) {
        return 0;
    }

    {
        // Determine real input
        std::lock_guard<std::mutex> lock(mut_);
        if (inputs_.empty()) {
            // Use default
            input_ = defaultInput_;
        } else if (inputs_.size() == 1) {
            input_ = inputs_.front();
        } else {
            // Calculate sampleRate and channelCount by inputs, ignore other options.
            AudioInputOptions inputOptions;
            inputOptions.sampleRate = MixingAudioInput::CalculateOutputSampleRate(inputs_);
            inputOptions.channelCount = MixingAudioInput::CalculateOutputChannelCount(inputs_);

            auto mixerInput = std::make_shared<MixingAudioInput>(inputOptions);
            for (auto& input : inputs_) {
                mixerInput->AddAudioInput(input);
            }
            input_ = mixerInput;
        }
        input_->Init();
        input_->RegisterObserver(this);
        input_->SetMute(microphoneMute_);
    }

    audioDeviceBuffer_->SetRecordingSampleRate(input_->GetSampleRate());
    audioDeviceBuffer_->SetRecordingChannels(input_->GetChannelCount());
    inputAudioBuffer_ = std::make_unique<FineAudioBuffer>(audioDeviceBuffer_.get());

    int32_t result = input_->InitRecording();
    RTC_DLOG(LS_INFO) << "output: " << result;

    return result;
}

bool OhosAudioDeviceModule::RecordingIsInitialized() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    if (!input_) {
        return false;
    }

    return input_->RecordingIsInitialized();
}

int32_t OhosAudioDeviceModule::StartPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (!initialized_) {
        return -1;
    }

    if (Playing()) {
        return 0;
    }

    int32_t result = output_->StartPlayout();
    RTC_DLOG(LS_INFO) << "output: " << result;

    if (result == 0) {
        // Only start playing the audio device buffer if starting the audio output succeeded.
        audioDeviceBuffer_->StartPlayout();
    }

    return result;
}

int32_t OhosAudioDeviceModule::StopPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (!initialized_) {
        return -1;
    }

    if (!Playing()) {
        return 0;
    }

    audioDeviceBuffer_->StopPlayout();
    int32_t result = output_->StopPlayout();
    RTC_DLOG(LS_INFO) << "output: " << result;

    return result;
}

bool OhosAudioDeviceModule::Playing() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return output_->Playing();
}

int32_t OhosAudioDeviceModule::StartRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (!initialized_) {
        return -1;
    }

    if (Recording()) {
        return 0;
    }

    inputAudioBuffer_->ResetRecord();

    int32_t result = input_->StartRecording();
    RTC_DLOG(LS_INFO) << "output: " << result;

    if (result == 0) {
        // Only start recording the audio device buffer if starting the audio input succeeded.
        audioDeviceBuffer_->StartRecording();
    }

    return result;
}

int32_t OhosAudioDeviceModule::StopRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    if (!initialized_) {
        return -1;
    }

    if (!Recording()) {
        return 0;
    }

    audioDeviceBuffer_->StopRecording();
    int32_t result = input_->StopRecording();
    RTC_DLOG(LS_INFO) << "output: " << result;

    return result;
}

bool OhosAudioDeviceModule::Recording() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    if (!input_) {
        return false;
    }
    return input_->Recording();
}

int32_t OhosAudioDeviceModule::InitSpeaker()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return initialized_ ? 0 : -1;
}

bool OhosAudioDeviceModule::SpeakerIsInitialized() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return initialized_;
}

int32_t OhosAudioDeviceModule::InitMicrophone()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return initialized_ ? 0 : -1;
}

bool OhosAudioDeviceModule::MicrophoneIsInitialized() const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    return initialized_;
}

int32_t OhosAudioDeviceModule::SpeakerVolumeIsAvailable(bool* available)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *available = false;
    return 0;
}

int32_t OhosAudioDeviceModule::SetSpeakerVolume(uint32_t volume)
{
    (void)volume;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::SpeakerVolume(uint32_t* outputVolume) const
{
    (void)outputVolume;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::MaxSpeakerVolume(uint32_t* outputMaxVolume) const
{
    (void)outputMaxVolume;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::MinSpeakerVolume(uint32_t* outputMinVolume) const
{
    (void)outputMinVolume;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::MicrophoneVolumeIsAvailable(bool* available)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *available = false;
    return -1;
}

int32_t OhosAudioDeviceModule::SetMicrophoneVolume(uint32_t volume)
{
    (void)volume;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::MicrophoneVolume(uint32_t* volume) const
{
    (void)volume;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::MaxMicrophoneVolume(uint32_t* maxVolume) const
{
    (void)maxVolume;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::MinMicrophoneVolume(uint32_t* minVolume) const
{
    (void)minVolume;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::SpeakerMuteIsAvailable(bool* available)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::SetSpeakerMute(bool enable)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    output_->SetMute(enable);
    return 0;
}

int32_t OhosAudioDeviceModule::SpeakerMute(bool* enabled) const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::MicrophoneMuteIsAvailable(bool* available)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::SetMicrophoneMute(bool enable)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    microphoneMute_ = enable;
    if (input_) {
        input_->SetMute(enable);
    }
    return 0;
}

int32_t OhosAudioDeviceModule::MicrophoneMute(bool* enabled) const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    RTC_CHECK_NOTREACHED();
}

int32_t OhosAudioDeviceModule::StereoPlayoutIsAvailable(bool* available) const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *available = isStereoPlayoutSupported_;
    RTC_DLOG(LS_INFO) << "available: " << *available;

    return 0;
}

int32_t OhosAudioDeviceModule::SetStereoPlayout(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";

    bool available = isStereoPlayoutSupported_;
    if (enable != available) {
        RTC_LOG(LS_WARNING) << "changing stereo playout not supported";
        return -1;
    }

    return 0;
}

int32_t OhosAudioDeviceModule::StereoPlayout(bool* enabled) const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *enabled = isStereoPlayoutSupported_;
    RTC_DLOG(LS_INFO) << "enabled: " << *enabled;

    return 0;
}

int32_t OhosAudioDeviceModule::StereoRecordingIsAvailable(bool* available) const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *available = isStereoRecordSupported_;
    RTC_DLOG(LS_INFO) << "available: " << *available;

    return 0;
}

int32_t OhosAudioDeviceModule::SetStereoRecording(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";

    bool available = isStereoRecordSupported_;
    if (enable != available) {
        RTC_LOG(LS_WARNING) << "changing stereo recording not supported";
        return -1;
    }

    return 0;
}

int32_t OhosAudioDeviceModule::StereoRecording(bool* enabled) const
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    *enabled = isStereoRecordSupported_;
    RTC_DLOG(LS_INFO) << "output: " << *enabled;

    return 0;
}

int32_t OhosAudioDeviceModule::PlayoutDelay(uint16_t* delayMs) const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return output_->PlayoutDelay(delayMs);
}

bool OhosAudioDeviceModule::BuiltInAECIsAvailable() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return false;
}

bool OhosAudioDeviceModule::BuiltInAGCIsAvailable() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return false;
}

bool OhosAudioDeviceModule::BuiltInNSIsAvailable() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return false;
}

int32_t OhosAudioDeviceModule::EnableBuiltInAEC(bool enable)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << "(" << enable << ")";
    RTC_LOG_F(LS_ERROR) << "Not supported on this platform";

    return -1;
}

int32_t OhosAudioDeviceModule::EnableBuiltInAGC(bool enable)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << "(" << enable << ")";
    RTC_LOG_F(LS_ERROR) << "Not supported on this platform";

    return -1;
}

int32_t OhosAudioDeviceModule::EnableBuiltInNS(bool enable)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << "(" << enable << ")";
    RTC_LOG_F(LS_ERROR) << "Not supported on this platform";

    return -1;
}

int32_t OhosAudioDeviceModule::GetPlayoutUnderrunCount() const
{
    if (!initialized_) {
        return -1;
    }

    return output_->GetPlayoutUnderrunCount();
}

absl::optional<AudioDeviceModule::Stats> OhosAudioDeviceModule::GetStats() const
{
    if (!initialized_) {
        return absl::nullopt;
    }

    return output_->GetStats();
}

void OhosAudioDeviceModule::OnAudioInputError(AudioInput* input, AudioErrorType type, const std::string& message)
{
    RTC_LOG(LS_ERROR) << "Audio input error: " << type << ", " << message;

    std::lock_guard<std::mutex> lock(inputObsMutex_);
    for (auto& obs : inputObservers_) {
        obs->OnAudioInputError(input_.get(), type, message);
    }
}

void OhosAudioDeviceModule::OnAudioInputStateChange(AudioInput* input, AudioStateType newState)
{
    RTC_LOG(LS_INFO) << "Audio input state change: " << newState;

    std::lock_guard<std::mutex> lock(inputObsMutex_);
    for (auto& obs : inputObservers_) {
        obs->OnAudioInputStateChange(input_.get(), newState);
    }
}

void OhosAudioDeviceModule::OnAudioInputDataReady(
    AudioInput* input, void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    inputAudioBuffer_->DeliverRecordedData(
        rtc::MakeArrayView(static_cast<const int16_t*>(buffer), length / sizeof(int16_t)),
        static_cast<int>(deleyUs / rtc::kNumMicrosecsPerMillisec));

    std::lock_guard<std::mutex> lock(inputObsMutex_);
    for (auto& obs : inputObservers_) {
        obs->OnAudioInputDataReady(input_.get(), buffer, length, timestampUs, deleyUs);
    }
}

void OhosAudioDeviceModule::OnAudioOutputError(AudioOutput* output, AudioErrorType type, const std::string& message)
{
    RTC_LOG(LS_ERROR) << "Audio output error: " << type << ", " << message;

    std::lock_guard<std::mutex> lock(inputObsMutex_);
    for (auto& obs : outputObservers_) {
        obs->OnAudioOutputError(output_.get(), type, message);
    }
}

void OhosAudioDeviceModule::OnAudioOutputStateChange(AudioOutput* output, AudioStateType newState)
{
    RTC_LOG(LS_INFO) << "Audio output state change: " << newState;

    std::lock_guard<std::mutex> lock(inputObsMutex_);
    for (auto& obs : outputObservers_) {
        obs->OnAudioOutputStateChange(output_.get(), newState);
    }
}

FunctionReference NapiAudioDeviceModule::constructor_;

void NapiAudioDeviceModule::Init(Napi::Env env, Napi::Object exports)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Function func = DefineClass(
        env, kClassName,
        {
            InstanceAccessor<&NapiAudioDeviceModule::GetEventHandler, &NapiAudioDeviceModule::SetEventHandler>(
                kAttributeNameOnCapturerError, napi_default, (void*)kEventNameCapturerError),
            InstanceAccessor<&NapiAudioDeviceModule::GetEventHandler, &NapiAudioDeviceModule::SetEventHandler>(
                kAttributeNameOnCapturerStateChange, napi_default, (void*)kEventNameCapturerStateChange),
            InstanceAccessor<&NapiAudioDeviceModule::GetEventHandler, &NapiAudioDeviceModule::SetEventHandler>(
                kAttributeNameOnCapturerSamplesReady, napi_default, (void*)kEventNameCapturerSamplesReady),
            InstanceAccessor<&NapiAudioDeviceModule::GetEventHandler, &NapiAudioDeviceModule::SetEventHandler>(
                kAttributeNameOnRendererError, napi_default, (void*)kEventNameRendererError),
            InstanceAccessor<&NapiAudioDeviceModule::GetEventHandler, &NapiAudioDeviceModule::SetEventHandler>(
                kAttributeNameOnRendererStateChange, napi_default, (void*)kEventNameRendererStateChange),
            InstanceMethod<&NapiAudioDeviceModule::SetSpeakerMute>(kMethodNameSetSpeakerMute),
            InstanceMethod<&NapiAudioDeviceModule::SetMicrophoneMute>(kMethodNameSetMicrophoneMute),
            InstanceMethod<&NapiAudioDeviceModule::SetNoiseSuppressorEnabled>(kMethodNameSetNoiseSuppressorEnabled),
            InstanceMethod<&NapiAudioDeviceModule::ToJson>(kMethodNameToJson),
            StaticMethod<&NapiAudioDeviceModule::isBuiltInAcousticEchoCancelerSupported>(
                kMethodNameIsBuiltInAcousticEchoCancelerSupported),
            StaticMethod<&NapiAudioDeviceModule::isBuiltInNoiseSuppressorSupported>(
                kMethodNameIsBuiltInNoiseSuppressorSupported),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

bool GetOption(const Object& obj, const char* key, std::optional<int32_t>* valueOut)
{
    if (obj.Has(key)) {
        *valueOut = obj.Get(key).As<Number>().Int32Value();
        return true;
    }
    return false;
}

bool GetOption(const Object& obj, const char* key, std::optional<bool>* valueOut)
{
    if (obj.Has(key)) {
        *valueOut = obj.Get(key).As<Boolean>().Value();
        return true;
    }
    return false;
}

NapiAudioDeviceModule::NapiAudioDeviceModule(const Napi::CallbackInfo& info) : ObjectWrap<NapiAudioDeviceModule>(info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    AudioInputOptions inputOptions;
    AudioOutputOptions outputOptions;

    if (info.Length() > 0) {
        auto options = ((Napi::Value)info[0]).As<Object>();

        GetOption(options, kAttributeNameAudioSource, &inputOptions.source);
        GetOption(options, kAttributeNameInputSampleRate, &inputOptions.sampleRate);
        GetOption(options, kAttributeNameUseLowLatency, &inputOptions.useLowLatency);

        GetOption(options, kAttributeNameOutputSampleRate, &outputOptions.sampleRate);
        GetOption(options, kAttributeNameRendererUsage, &outputOptions.usage);
        GetOption(options, kAttributeNameUseLowLatency, &outputOptions.useLowLatency);

        if (options.Has(kAttributeNameUseStereoInput)) {
            bool useStereoInput = options.Get(kAttributeNameUseStereoInput).As<Boolean>().Value();
            inputOptions.channelCount = useStereoInput ? kAudioChannelCount_Stereo : kAudioChannelCount_Mono;
        }

        if (options.Has(kAttributeNameUseStereoOutput)) {
            bool useStereoOutput = options.Get(kAttributeNameUseStereoOutput).As<Boolean>().Value();
            inputOptions.channelCount = useStereoOutput ? kAudioChannelCount_Stereo : kAudioChannelCount_Mono;
        }

        if (options.Has(kAttributeNameAudioFormat)) {
            // Ignore, always use AUDIOSTREAM_SAMPLE_S16LE
            RTC_DLOG(LS_INFO) << "audioFormat: " << options.Get(kAttributeNameAudioFormat).As<Number>().Int32Value();
        }

        if (options.Has(kAttributeNameUseHardwareAcousticEchoCanceler)) {
            // Ignore
            RTC_DLOG(LS_INFO) << "useHardwareAcousticEchoCanceler: "
                              << options.Get(kAttributeNameUseHardwareAcousticEchoCanceler).As<Boolean>().Value();
        }

        if (options.Has(kAttributeNameUseHardwareNoiseSuppressor)) {
            // Ignore
            RTC_DLOG(LS_INFO) << "useHardwareNoiseSuppressor: "
                              << options.Get(kAttributeNameUseHardwareNoiseSuppressor).As<Boolean>().Value();
        }
    }

    adm_ = rtc::make_ref_counted<OhosAudioDeviceModule>(
        std::move(inputOptions), std::move(outputOptions), AudioDeviceModule::kPlatformDefaultAudio);
    if (!adm_) {
        NAPI_THROW_VOID(Error::New(info.Env(), "Failed to create audio device module"));
    }

    adm_->RegisterInputObserver(this);
    adm_->RegisterOutputObserver(this);
}

NapiAudioDeviceModule::~NapiAudioDeviceModule()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    for (auto& handler : eventHandlers_) {
        handler.second.tsfn.Release();
    }
}

Napi::Value NapiAudioDeviceModule::GetEventHandler(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const auto type = (const char*)info.Data();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = eventHandlers_.find(type);
    if (it != eventHandlers_.end()) {
        return it->second.ref.Value();
    }

    return info.Env().Null();
}

void NapiAudioDeviceModule::SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!value.IsFunction()) {
        NAPI_THROW_VOID(TypeError::New(info.Env(), "First argument is not Function"));
    }

    const auto type = (const char*)info.Data();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = eventHandlers_.find(type);
        if (it != eventHandlers_.end()) {
            it->second.tsfn.Release();
            eventHandlers_.erase(it);
        }
    }

    Function cb = value.As<Function>();

    Reference<Napi::Value>* context = new Reference<Napi::Value>;
    *context = Persistent(info.This());

    EventHandler handler;
    handler.ref = Persistent(cb);
    handler.tsfn =
        ThreadSafeFunction::New(info.Env(), cb, type, 0, 1, context, [](Napi::Env env, Reference<Napi::Value>* ctx) {
            (void)env;
            ctx->Reset();
            delete ctx;
        });

    std::lock_guard<std::mutex> lock(mutex_);
    eventHandlers_[type] = std::move(handler);
}

Napi::Value NapiAudioDeviceModule::SetSpeakerMute(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    rtc::scoped_refptr<AudioDeviceModule>(adm_)->SetSpeakerMute(info[0].As<Boolean>().Value());
    return info.Env().Undefined();
}

Napi::Value NapiAudioDeviceModule::SetMicrophoneMute(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    rtc::scoped_refptr<AudioDeviceModule>(adm_)->SetMicrophoneMute(info[0].As<Boolean>().Value());
    return info.Env().Undefined();
}

Napi::Value NapiAudioDeviceModule::SetNoiseSuppressorEnabled(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // Noise suppressor is not supported.
    return Boolean::New(info.Env(), false);
}

Napi::Value NapiAudioDeviceModule::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto result = Object::New(info.Env());
#ifndef NDEBUG
    result.Set("__native_class__", String::New(info.Env(), "NapiAudioDeviceModule"));
#endif

    return result;
}

Napi::Value NapiAudioDeviceModule::isBuiltInAcousticEchoCancelerSupported(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return Boolean::New(info.Env(), false);
}

Napi::Value NapiAudioDeviceModule::isBuiltInNoiseSuppressorSupported(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return Boolean::New(info.Env(), false);
}

void NapiAudioDeviceModule::OnAudioInputError(AudioInput* input, AudioErrorType type, const std::string& message)
{
    RTC_LOG(LS_ERROR) << "Audio input error: " << type << ", " << message;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = eventHandlers_.find(kEventNameCapturerError);
    if (it == eventHandlers_.end()) {
        return;
    }

    auto& tsfn = it->second.tsfn;
    Reference<Napi::Value>* context = tsfn.GetContext();
    napi_status status = tsfn.NonBlockingCall([context, type, message](Napi::Env env, Napi::Function jsCallback) {
        RTC_DLOG(LS_VERBOSE) << "error=" << type << ", " << message;
        auto jsError = Object::New(env);
        jsError.Set("type", String::New(env, AudioErrorTypeToString(type)));
        jsError.Set("message", String::New(env, message));

        auto jsEvent = Object::New(env);
        jsEvent.Set("error", jsError);
        jsCallback.Call(context ? context->Value() : env.Undefined(), {jsEvent});
    });
    if (status != napi_ok) {
        RTC_LOG(LS_ERROR) << " tsfn call error: " << status;
    }
}

void NapiAudioDeviceModule::OnAudioInputStateChange(AudioInput* input, AudioStateType newState)
{
    RTC_LOG(LS_INFO) << "Audio output state change: " << newState;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = eventHandlers_.find(kEventNameCapturerStateChange);
    if (it == eventHandlers_.end()) {
        return;
    }

    auto& tsfn = it->second.tsfn;
    Reference<Napi::Value>* context = tsfn.GetContext();
    napi_status status = tsfn.NonBlockingCall([context, newState](Napi::Env env, Napi::Function jsCallback) {
        auto jsEvent = Object::New(env);
        jsEvent.Set("state", String::New(env, AudioStateToString(newState)));
        jsCallback.Call(context ? context->Value() : env.Undefined(), {jsEvent});
    });
    if (status != napi_ok) {
        RTC_LOG(LS_ERROR) << " tsfn call error: " << status;
    }
}

void NapiAudioDeviceModule::OnAudioInputDataReady(
    AudioInput* input, void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = eventHandlers_.find(kEventNameCapturerSamplesReady);
    if (it == eventHandlers_.end()) {
        return;
    }

    auto data = new rtc::CopyOnWriteBuffer((uint8_t*)buffer, length);

    auto& tsfn = it->second.tsfn;
    Reference<Napi::Value>* context = tsfn.GetContext();
    napi_status status =
        tsfn.NonBlockingCall([context, sampleRate = input->GetSampleRate(), channelCount = input->GetChannelCount(),
                              data](Napi::Env env, Napi::Function jsCallback) {
            auto arrayBuffer = ArrayBuffer::New(
                env, static_cast<void*>(data->MutableData()), data->size(),
                [](Napi::Env /*env*/, void* /*data*/, rtc::CopyOnWriteBuffer* hint) {
                    RTC_DLOG(LS_VERBOSE) << "release rtc::CopyOnWriteBuffer";
                    delete hint;
                },
                data);

            auto jsAudioSamples = Object::New(env);
            jsAudioSamples.Set("sampleRate", Number::New(env, sampleRate));
            jsAudioSamples.Set("audioFormat", Number::New(env, AUDIOSTREAM_SAMPLE_S16LE));
            jsAudioSamples.Set("channelCount", Number::New(env, channelCount));
            jsAudioSamples.Set("data", arrayBuffer);

            auto jsEvent = Object::New(env);
            jsEvent.Set("samples", jsAudioSamples);
            jsCallback.Call(context ? context->Value() : env.Undefined(), {jsEvent});
        });
    if (status != napi_ok) {
        RTC_LOG(LS_ERROR) << " tsfn call error: " << status;
    }
}

void NapiAudioDeviceModule::OnAudioOutputError(AudioOutput* output, AudioErrorType type, const std::string& message)
{
    RTC_LOG(LS_ERROR) << "Audio output error: " << type << ", " << message;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = eventHandlers_.find(kEventNameRendererError);
    if (it == eventHandlers_.end()) {
        return;
    }

    auto& tsfn = it->second.tsfn;
    Reference<Napi::Value>* context = tsfn.GetContext();
    napi_status status = tsfn.NonBlockingCall([context, type, message](Napi::Env env, Napi::Function jsCallback) {
        auto jsError = Object::New(env);
        jsError.Set("type", String::New(env, AudioErrorTypeToString(type)));
        jsError.Set("message", String::New(env, message));

        auto jsEvent = Object::New(env);
        jsEvent.Set("error", jsError);
        jsCallback.Call(context ? context->Value() : env.Undefined(), {jsEvent});
    });
    if (status != napi_ok) {
        RTC_LOG(LS_ERROR) << " tsfn call error: " << status;
    }
}

void NapiAudioDeviceModule::OnAudioOutputStateChange(AudioOutput* output, AudioStateType newState)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "Audio output state change: " << newState;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = eventHandlers_.find(kEventNameRendererStateChange);
    if (it == eventHandlers_.end()) {
        return;
    }

    auto& tsfn = it->second.tsfn;
    Reference<Napi::Value>* context = tsfn.GetContext();
    napi_status status = tsfn.NonBlockingCall([context, newState](Napi::Env env, Napi::Function jsCallback) {
        auto jsEvent = Object::New(env);
        jsEvent.Set("state", String::New(env, AudioStateToString(newState)));
        jsCallback.Call(context ? context->Value() : env.Undefined(), {jsEvent});
    });
    if (status != napi_ok) {
        RTC_LOG(LS_ERROR) << " tsfn call error: " << status;
    }
}

rtc::scoped_refptr<OhosAudioDeviceModule> CreateDefaultAudioDeviceModule()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    AudioInputOptions inputOptions;
    AudioOutputOptions outputOptions;
    return rtc::make_ref_counted<OhosAudioDeviceModule>(
        std::move(inputOptions), std::move(outputOptions), AudioDeviceModule::kPlatformDefaultAudio);
}

} // namespace webrtc
