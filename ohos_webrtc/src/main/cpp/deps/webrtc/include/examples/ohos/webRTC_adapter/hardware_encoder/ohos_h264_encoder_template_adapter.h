/*
# Copyright (c) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/

#ifndef OHOS_H264_ENCODER_TEMPLATE_ADAPTER
#define OHOS_H264_ENCODER_TEMPLATE_ADAPTER

#include <vector>
#include "api/video_codecs/sdp_video_format.h"
#include "modules/video_coding/include/video_coding.h"

namespace webrtc {
namespace ohos {

struct OhosH264EncoderTemplateAdapter {
    OhosH264EncoderTemplateAdapter() = default;
    ~OhosH264EncoderTemplateAdapter() = default;
    static std::vector<SdpVideoFormat> SupportedFormats();
    static std::unique_ptr<VideoEncoder>CreateEncoder(const SdpVideoFormat &format);
    static bool IsScalabilityModeSupported(ScalabilityMode scalability_mode);
};

}
}
#endif // OHOS_H264_ENCODER_TEMPLATE_ADAPTER
