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

#include "camera_device_info.h"

namespace webrtc {

std::string CameraPositionToString(Camera_Position position)
{
    switch (position) {
        case CAMERA_POSITION_BACK:
            return "Back";
        case CAMERA_POSITION_FRONT:
            return "Front";
        default:
            return "Unspecified";
    }
}

std::string CameraConnectionTypeToString(Camera_Connection connectionType)
{
    switch (connectionType) {
        case CAMERA_CONNECTION_BUILT_IN:
            return "Build_in";
        case CAMERA_CONNECTION_USB_PLUGIN:
            return "USB";
        case CAMERA_CONNECTION_REMOTE:
            return "Remote";
        default:
            return "Unspecified";
    }
}

video::PixelFormat NativeCameraFormatToPixelFormat(Camera_Format format)
{
    switch (format) {
        case CAMERA_FORMAT_RGBA_8888:
            return video::PixelFormat::RGBA;
        case CAMERA_FORMAT_YUV_420_SP:
            return video::PixelFormat::NV12; // NV21?
        default:
            return video::PixelFormat::Unsupported;
    }
}

} // namespace webrtc
