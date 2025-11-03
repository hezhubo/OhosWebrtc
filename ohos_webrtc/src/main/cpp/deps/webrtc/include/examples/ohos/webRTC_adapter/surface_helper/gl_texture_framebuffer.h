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
#ifndef OH_WEB_RTC_GL_TEXTURE_FRAMEBUFFER_H
#define OH_WEB_RTC_GL_TEXTURE_FRAMEBUFFER_H

#include "cstdint"
#include <GLES3/gl3.h>

namespace webrtc {
namespace ohos {

// fbo 管理
class GLTextureFrameBuffer {
public:
    bool SetSize(int width, int height);
    GLuint GetFrameBufferID();
    int GetWidth();
    int GetHeight();
private:
    GLuint fbo_{0};
    GLuint textureID_{0};
    int width_{0};
    int height_{0};
};
}
}

#endif //OH_WEB_RTC_GL_TEXTURE_FRAMEBUFFER_H
