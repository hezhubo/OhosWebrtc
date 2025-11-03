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

#include "audio_device_enumerator.h"

#include <ohaudio/native_audio_device_base.h>
#include <ohaudio/native_audio_routing_manager.h>

#include "rtc_base/logging.h"

namespace webrtc {

AudioDeviceRole NativeAudioDeviceRoleToAudioDeviceRole(OH_AudioDevice_Role role)
{
    switch (role) {
        case AUDIO_DEVICE_ROLE_INPUT:
            return AudioDeviceRole::Input;
        case AUDIO_DEVICE_ROLE_OUTPUT:
            return AudioDeviceRole::Output;
        default:
            return AudioDeviceRole::Unknown;
    }
}

std::string AudioDeviceTypeToString(OH_AudioDevice_Type type)
{
    switch (type) {
        case AUDIO_DEVICE_TYPE_INVALID:
            return "Invalid";
        case AUDIO_DEVICE_TYPE_EARPIECE:
            return "Earpiece";
        case AUDIO_DEVICE_TYPE_SPEAKER:
            return "Speaker";
        case AUDIO_DEVICE_TYPE_WIRED_HEADSET:
            return "Headset";
        case AUDIO_DEVICE_TYPE_WIRED_HEADPHONES:
            return "Wired headphones";
        case AUDIO_DEVICE_TYPE_BLUETOOTH_SCO:
            return "Bluetooth SCO";
        case AUDIO_DEVICE_TYPE_BLUETOOTH_A2DP:
            return "Bluetooth A2DP";
        case AUDIO_DEVICE_TYPE_MIC:
            return "Microphone";
        case AUDIO_DEVICE_TYPE_USB_HEADSET:
            return "USB headset";
        case AUDIO_DEVICE_TYPE_DISPLAY_PORT:
            return "Display port";
        case AUDIO_DEVICE_TYPE_REMOTE_CAST:
            return "Remote cast";
        case AUDIO_DEVICE_TYPE_DEFAULT:
            return "Default";
        default:
            break;
    }
    return "Unspecified";
}

std::vector<AudioDeviceInfo> AudioDeviceEnumerator::GetDevices()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    std::vector<AudioDeviceInfo> result;

    OH_AudioRoutingManager* audioRoutingManager = nullptr;
    OH_AudioCommon_Result ret = OH_AudioManager_GetAudioRoutingManager(&audioRoutingManager);
    if (ret != AUDIOCOMMON_RESULT_SUCCESS) {
        RTC_LOG(LS_ERROR) << "Failed to get audio routing manager: " << ret;
        return result;
    }

    OH_AudioDeviceDescriptorArray* audioDeviceDescriptorArray = nullptr;
    ret = OH_AudioRoutingManager_GetDevices(audioRoutingManager, AUDIO_DEVICE_FLAG_ALL, &audioDeviceDescriptorArray);
    if (ret != AUDIOCOMMON_RESULT_SUCCESS) {
        RTC_LOG(LS_ERROR) << "Failed to get audio devices: " << ret;
        return result;
    }

    RTC_DLOG(LS_VERBOSE) << "audio devices: " << audioDeviceDescriptorArray->size;

    for (uint32_t index = 0; index < audioDeviceDescriptorArray->size; index++) {
        AudioDeviceInfo device;
        device.groupId = "default";

        OH_AudioDeviceDescriptor* descriptor = audioDeviceDescriptorArray->descriptors[index];

        char* address = nullptr;
        ret = OH_AudioDeviceDescriptor_GetDeviceAddress(descriptor, &address);
        if (ret == AUDIOCOMMON_RESULT_SUCCESS) {
            RTC_DLOG(LS_VERBOSE) << "audio device mac address: " << address;
        }

        uint32_t deviceId = 0;
        ret = OH_AudioDeviceDescriptor_GetDeviceId(descriptor, &deviceId);
        if (ret == AUDIOCOMMON_RESULT_SUCCESS) {
            RTC_DLOG(LS_VERBOSE) << "audio device id: " << deviceId;
            device.deviceId = std::to_string(deviceId);
        }

        char* name = nullptr;
        ret = OH_AudioDeviceDescriptor_GetDeviceName(descriptor, &name);
        if (ret == AUDIOCOMMON_RESULT_SUCCESS) {
            RTC_DLOG(LS_VERBOSE) << "audio device name: " << name;
            device.label = name;
        }

        char* displayName = nullptr;
        ret = OH_AudioDeviceDescriptor_GetDeviceDisplayName(descriptor, &displayName);
        if (ret == AUDIOCOMMON_RESULT_SUCCESS) {
            RTC_DLOG(LS_VERBOSE) << "audio device display name: " << displayName;
        }

        OH_AudioDevice_Role role = AUDIO_DEVICE_ROLE_INPUT;
        ret = OH_AudioDeviceDescriptor_GetDeviceRole(descriptor, &role);
        if (ret == AUDIOCOMMON_RESULT_SUCCESS) {
            RTC_DLOG(LS_VERBOSE) << "audio device role: " << role;
            device.role = NativeAudioDeviceRoleToAudioDeviceRole(role);
        }

        OH_AudioDevice_Type type = AUDIO_DEVICE_TYPE_INVALID;
        ret = OH_AudioDeviceDescriptor_GetDeviceType(descriptor, &type);
        if (ret == AUDIOCOMMON_RESULT_SUCCESS) {
            RTC_DLOG(LS_VERBOSE) << "audio device type: " << type;
        }

        if (device.label.length() == 0) {
            device.label = AudioDeviceTypeToString(type) + " (" + device.deviceId + ")";
        }

        result.push_back(device);
    }

    OH_AudioRoutingManager_ReleaseDevices(audioRoutingManager, audioDeviceDescriptorArray);

    return result;
}

} // namespace webrtc
