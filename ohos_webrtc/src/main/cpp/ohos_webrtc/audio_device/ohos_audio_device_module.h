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

#ifndef WEBRTC_OHOS_AUDIO_DEVICE_MODULE_H
#define WEBRTC_OHOS_AUDIO_DEVICE_MODULE_H

#include <map>
#include <list>
#include <string>
#include <memory>

#include "api/audio_options.h"
#include "api/sequence_checker.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/fine_audio_buffer.h"

#include "napi.h"

#include "audio_input.h"
#include "audio_output.h"
#include "ohos_local_audio_source.h"

namespace webrtc {

class OhosAudioDeviceModule : public AudioDeviceModule, public AudioInput::Observer, public AudioOutput::Observer {
public:
    OhosAudioDeviceModule(
        AudioInputOptions inputOptions, AudioOutputOptions outputOptions,
        AudioDeviceModule::AudioLayer audioLayer = AudioDeviceModule::kPlatformDefaultAudio);

    OhosAudioDeviceModule(const OhosAudioDeviceModule&) = delete;
    OhosAudioDeviceModule& operator=(const OhosAudioDeviceModule&) = delete;

    void AddAudioInput(std::shared_ptr<AudioInput> input);
    void RemoveAudioInput(std::shared_ptr<AudioInput> input);

    // Create default or system audio source
    rtc::scoped_refptr<OhosLocalAudioSource>
    CreateAudioSource(cricket::AudioOptions options, std::shared_ptr<AudioInput> audioInput = nullptr);

    void RegisterInputObserver(AudioInput::Observer* obs);
    void UnregisterInputObserver(AudioInput::Observer* obs);

    void RegisterOutputObserver(AudioOutput::Observer* obs);
    void UnregisterOutputObserver(AudioOutput::Observer* obs);

protected:
    enum class InitStatus {
        OK = 0,
        PLAYOUT_ERROR = 1,
        RECORDING_ERROR = 2,
        OTHER_ERROR = 3,
        NUM_STATUSES = 4
    };

    ~OhosAudioDeviceModule() override;

    // webrtc::AudioDeviceModule implementation.
    int32_t ActiveAudioLayer(AudioDeviceModule::AudioLayer* audioLayer) const override;
    int32_t RegisterAudioCallback(AudioTransport* audioCallback) override;
    int32_t Init() override;
    int32_t Terminate() override;
    bool Initialized() const override;
    int16_t PlayoutDevices() override;
    int16_t RecordingDevices() override;
    int32_t PlayoutDeviceName(uint16_t index, char name[kAdmMaxDeviceNameSize], char guid[kAdmMaxGuidSize]) override;
    int32_t RecordingDeviceName(uint16_t index, char name[kAdmMaxDeviceNameSize], char guid[kAdmMaxGuidSize]) override;
    int32_t SetPlayoutDevice(uint16_t index) override;
    int32_t SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType device) override;
    int32_t SetRecordingDevice(uint16_t index) override;
    int32_t SetRecordingDevice(AudioDeviceModule::WindowsDeviceType device) override;
    int32_t PlayoutIsAvailable(bool* available) override;
    int32_t InitPlayout() override;
    bool PlayoutIsInitialized() const override;
    int32_t RecordingIsAvailable(bool* available) override;
    int32_t InitRecording() override;
    bool RecordingIsInitialized() const override;
    int32_t StartPlayout() override;
    int32_t StopPlayout() override;
    bool Playing() const override;
    int32_t StartRecording() override;
    int32_t StopRecording() override;
    bool Recording() const override;
    int32_t InitSpeaker() override;
    bool SpeakerIsInitialized() const override;
    int32_t InitMicrophone() override;
    bool MicrophoneIsInitialized() const override;
    int32_t SpeakerVolumeIsAvailable(bool* available) override;
    int32_t SetSpeakerVolume(uint32_t volume) override;
    int32_t SpeakerVolume(uint32_t* outputVolume) const override;
    int32_t MaxSpeakerVolume(uint32_t* outputMaxVolume) const override;
    int32_t MinSpeakerVolume(uint32_t* outputMinVolume) const override;
    int32_t MicrophoneVolumeIsAvailable(bool* available) override;
    int32_t SetMicrophoneVolume(uint32_t volume) override;
    int32_t MicrophoneVolume(uint32_t* volume) const override;
    int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override;
    int32_t MinMicrophoneVolume(uint32_t* minVolume) const override;
    int32_t SpeakerMuteIsAvailable(bool* available) override;
    int32_t SetSpeakerMute(bool enable) override;
    int32_t SpeakerMute(bool* enabled) const override;
    int32_t MicrophoneMuteIsAvailable(bool* available) override;
    int32_t SetMicrophoneMute(bool enable) override;
    int32_t MicrophoneMute(bool* enabled) const override;
    int32_t StereoPlayoutIsAvailable(bool* available) const override;
    int32_t SetStereoPlayout(bool enable) override;
    int32_t StereoPlayout(bool* enabled) const override;
    int32_t StereoRecordingIsAvailable(bool* available) const override;
    int32_t SetStereoRecording(bool enable) override;
    int32_t StereoRecording(bool* enabled) const override;
    int32_t PlayoutDelay(uint16_t* delayMs) const override;
    bool BuiltInAECIsAvailable() const override;
    bool BuiltInAGCIsAvailable() const override;
    bool BuiltInNSIsAvailable() const override;
    int32_t EnableBuiltInAEC(bool enable) override;
    int32_t EnableBuiltInAGC(bool enable) override;
    int32_t EnableBuiltInNS(bool enable) override;
    int32_t GetPlayoutUnderrunCount() const override;
    absl::optional<Stats> GetStats() const override;

    // AudioInput::Observer implementation.
    void OnAudioInputError(AudioInput* input, AudioErrorType type, const std::string& message) override;
    void OnAudioInputStateChange(AudioInput* input, AudioStateType newState) override;
    void OnAudioInputDataReady(
        AudioInput* input, void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs) override;

    // AudioOutput::Observer implementation.
    void OnAudioOutputError(AudioOutput* output, AudioErrorType type, const std::string& message) override;
    void OnAudioOutputStateChange(AudioOutput* output, AudioStateType newState) override;

private:
    SequenceChecker threadChecker_;

    const AudioDeviceModule::AudioLayer audioLayer_;
    const std::shared_ptr<AudioInput> defaultInput_;
    const std::unique_ptr<AudioOutput> output_;
    const bool isStereoRecordSupported_;
    const bool isStereoPlayoutSupported_;
    const std::unique_ptr<TaskQueueFactory> taskQueueFactory_;

    std::unique_ptr<AudioDeviceBuffer> audioDeviceBuffer_;
    std::unique_ptr<FineAudioBuffer> inputAudioBuffer_;

    bool initialized_{false};

    std::mutex inputObsMutex_;
    std::mutex outputObsMutex_;
    std::set<AudioInput::Observer*> inputObservers_;
    std::set<AudioOutput::Observer*> outputObservers_;

    std::shared_ptr<AudioInput> input_;
    mutable std::mutex mut_;
    std::list<std::shared_ptr<AudioInput>> inputs_;

    // Sets all recorded samples to zero if `microphoneMute_` is true, i.e., ensures that
    // the microphone is muted.
    std::atomic<bool> microphoneMute_{false};
};

class NapiAudioDeviceModule : public Napi::ObjectWrap<NapiAudioDeviceModule>,
                              public AudioInput::Observer,
                              public AudioOutput::Observer {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    ~NapiAudioDeviceModule() override;

    rtc::scoped_refptr<OhosAudioDeviceModule> Get() const
    {
        return adm_;
    }

protected:
    friend class ObjectWrap;

    explicit NapiAudioDeviceModule(const Napi::CallbackInfo& info);

    Napi::Value GetEventHandler(const Napi::CallbackInfo& info);
    void SetEventHandler(const Napi::CallbackInfo& info, const Napi::Value& value);

    Napi::Value SetSpeakerMute(const Napi::CallbackInfo& info);
    Napi::Value SetMicrophoneMute(const Napi::CallbackInfo& info);
    Napi::Value SetNoiseSuppressorEnabled(const Napi::CallbackInfo& info);
    Napi::Value ToJson(const Napi::CallbackInfo& info);

    static Napi::Value isBuiltInAcousticEchoCancelerSupported(const Napi::CallbackInfo& info);
    static Napi::Value isBuiltInNoiseSuppressorSupported(const Napi::CallbackInfo& info);

protected:
    // Implements AudioInput::Observer
    void OnAudioInputError(AudioInput* input, AudioErrorType type, const std::string& message) override;
    void OnAudioInputStateChange(AudioInput* input, AudioStateType newState) override;
    void OnAudioInputDataReady(
        AudioInput* input, void* buffer, int32_t length, int64_t timestampUs, int64_t deleyUs) override;

    // Implements AudioOutput::Observer
    void OnAudioOutputError(AudioOutput* output, AudioErrorType type, const std::string& message) override;
    void OnAudioOutputStateChange(AudioOutput* output, AudioStateType newState) override;

private:
    static Napi::FunctionReference constructor_;

    rtc::scoped_refptr<OhosAudioDeviceModule> adm_;

    struct EventHandler {
        Napi::FunctionReference ref;
        Napi::ThreadSafeFunction tsfn;
    };

    mutable std::mutex mutex_;
    std::map<std::string, EventHandler> eventHandlers_;
};

rtc::scoped_refptr<OhosAudioDeviceModule> CreateDefaultAudioDeviceModule();

} // namespace webrtc

#endif // WEBRTC_OHOS_AUDIO_DEVICE_MODULE_H
