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
#ifndef OHOS_GL_DRAWER_H
#define OHOS_GL_DRAWER_H

#include <map>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>
#include <native_window/external_window.h>
#include "ohos_shader_program.h"
#include "commom/ohos_video_buffer.h"

namespace webrtc {
namespace ohos {

const char *GetGLErrorString();

const std::string g_defaultVertexShader = R"delimiter(
attribute vec3 position;
attribute vec2 texCoord;

varying vec2 vTexCoord;

uniform mat4 matTransform;

void main()
{
    gl_Position = matTransform * vec4(position, 1.0);
    vTexCoord = texCoord;
}
)delimiter";

class OhosGLDrawer {
public:
    class ShaderCallbacks {
    public:
        virtual void OnNewShader(std::shared_ptr<ShaderProgram> shader) = 0;
        virtual void OnPrepareShader(std::shared_ptr<ShaderProgram> shader, float matrix[16],
            int frameWidth, int frameHeight, int viewportWidth, int viewportHeight) = 0;
    };
    explicit OhosGLDrawer(std::string fragmentShader = std::string(),
                          std::shared_ptr<ShaderCallbacks> shaderCallbacks = nullptr);
    ~OhosGLDrawer();
    void CreateGLResources();                       //创建GL画布
    void CleanGLResources();                        //清理GL画布
    void DrawFrame(OHOSVideoBuffer::TextureBuffer buffer, float *matrix, int frameWidth, int frameHeight,
        int viewportX, int viewportY, int viewportWidth, int viewportHeight);   //绘制图像
private:
    bool PrePareShader(OHOSVideoBuffer::TextureBuffer::Type frameType, float *matrix, int frameWidth,
        int frameHeight, int viewportX, int viewportY, int viewportWidth, int viewportHeight);  //shaderProgram初始化
    void DrawOES(GLuint textureID, float *matrix, int frameWidth, int frameHeight, int viewportX, int viewportY,
        int viewportWidth, int viewportHeight);
    void DrawRgb(int textureId, float *matrix, int frameWidth, int frameHeight,
        int viewportX, int viewportY, int viewportWidth, int viewportHeight);
    void DrawYuv(int *yuvTextures, float *matrix, int frameWidth, int frameHeight,
        int viewportX, int viewportY, int viewportWidth, int viewportHeight);

    OHOSVideoBuffer::TextureBuffer::Type currentFrameType_ {OHOSVideoBuffer::TextureBuffer::Type::OES};
    std::shared_ptr<ShaderProgram> shaderProgram_;
    std::shared_ptr<ShaderCallbacks> shaderCallbacks_;
    std::string fragmentShader_;
    static std::map<OHOSVideoBuffer::TextureBuffer::Type, std::string> shaderMap_;
    GLuint vertexArrayObject_{0};
    GLuint vertexBufferObject_{0};
};

}
} // namespace webrtc::ohos
#endif //OHOS_GL_DRAWER_H
