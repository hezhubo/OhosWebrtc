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
#ifndef __YUVCONVERTER__
#define __YUVCONVERTER__

#include <string>
#include "ohos_gl_drawer.h"
#include "api/video/i420_buffer.h"
#include "gl_texture_framebuffer.h"

namespace webrtc {
namespace ohos {

class YuvConverter {
public:
    YuvConverter();
    ~YuvConverter();
    rtc::scoped_refptr<webrtc::I420Buffer> Convert(OHOSVideoBuffer *videoBuffer);
    class ShaderCallback : public OhosGLDrawer::ShaderCallbacks {
    public:
        void SetPlanY();
        void SetPlanU();
        void SetPlanV();
        void OnNewShader(std::shared_ptr<ShaderProgram> shader) override;
        void OnPrepareShader(std::shared_ptr<ShaderProgram> shader, float *matrix,
            int frameWidth, int frameHeight, int viewportWidth, int viewportHeight) override;
    private:
        float yCoeffs[4]{0.256788f, 0.504129f, 0.0979059f, 0.0627451f};
        float uCoeffs[4]{-0.148223f, -0.290993f, 0.439216f, 0.501961f};
        float vCoeffs[4]{0.439216f, -0.367788f, -0.0714274f, 0.501961f};
        float *coeffs{nullptr};
        float stepSize{0};
    };
private:
    void Init();
    static std::string fragmentShader_;
    std::shared_ptr<OhosGLDrawer>drawer_;
    std::shared_ptr<ShaderCallback> converterShaderCallbacks_;
    GLTextureFrameBuffer glTextureFrameBuffer_;
};

}
}

#endif //__YUVCONVERTER__
