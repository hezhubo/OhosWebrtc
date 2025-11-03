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

#include "gl_drawer.h"

#include "rtc_base/logging.h"

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

namespace webrtc {

namespace {

static constexpr char DEFAULT_VERTEX_SHADER[] = R"(
attribute vec4 position;
attribute vec4 texCoord;

varying vec2 vTexCoord;

uniform mat4 transform;

void main()
{
    gl_Position = position;
    vTexCoord = (transform * texCoord).xy;
}
)";

static constexpr char OES_FRAGMENT_SHADER[] = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
varying vec2 vTexCoord;
uniform samplerExternalOES tex;

void main()
{
    gl_FragColor = texture2D(tex, vTexCoord).rgba;
}
)";

static constexpr char RGB_FRAGMENT_SHADER[] = R"(
precision mediump float;
varying vec2 vTexCoord;
uniform sampler2D tex;

void main()
{
    gl_FragColor = texture2D(tex, vTexCoord).rgba;
}
)";

static constexpr char YUV_FRAGMENT_SHADER[] = R"(
precision mediump float;

varying vec2 vTexCoord;

uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;

void main()
{
    float y = texture2D(tex_y, vTexCoord).r * 1.16438;
    float u = texture2D(tex_u, vTexCoord).r;
    float v = texture2D(tex_v, vTexCoord).r;
    gl_FragColor = vec4(y + 1.59603 * v - 0.874202, y - 0.391762 * u - 0.812968 * v + 0.531668, y + 2.01723 * u - 1.08563, 1.0);
}
)";

static constexpr float FULL_RECTANGLE_BUFFER[] = {
    -1.0f, -1.0f, // Bottom left.
    1.0f,  -1.0f, // Bottom right.
    -1.0f, 1.0f,  // Top left.
    1.0f,  1.0f,  // Top right.
};

// Texture coordinates - (0, 0) is bottom-left and (1, 1) is top-right.
static constexpr float FULL_RECTANGLE_TEXTURE_BUFFER[] = {
    0.0f, 0.0f, // Bottom left.
    1.0f, 0.0f, // Bottom right.
    0.0f, 1.0f, // Top left.
    1.0f, 1.0f, // Top right.
};

constexpr int32_t kVerticePositionComponents = 2;
constexpr int32_t kTexturePositionComponents = 2;

constexpr int32_t kVerticesNum =
    sizeof(FULL_RECTANGLE_BUFFER) / sizeof(FULL_RECTANGLE_BUFFER[0]) / kVerticePositionComponents;

constexpr int32_t kTextureUnit_Default = 0;
constexpr int32_t kTextureUnit_Y = 0;
constexpr int32_t kTextureUnit_U = 1;
constexpr int32_t kTextureUnit_V = 2;

constexpr int32_t kYuvTexturesNum = 3;

} // namespace

GlGenericDrawer::GlGenericDrawer() = default;

GlGenericDrawer::~GlGenericDrawer() = default;

void GlGenericDrawer::DrawOes(
    int oesTextureId, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportX, int viewportY,
    int viewportWidth, int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    PrepareShader(ShaderType::OES, texMatrix, frameWidth, frameHeight, viewportWidth, viewportHeight);

    // Bind the texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, oesTextureId);

    // Draw the texture.
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, kVerticesNum);

    // Unbind the texture as a precaution.
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
}

void GlGenericDrawer::DrawRgb(
    int textureId, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportX, int viewportY,
    int viewportWidth, int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    PrepareShader(ShaderType::RGB, texMatrix, frameWidth, frameHeight, viewportWidth, viewportHeight);

    // Bind the texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Draw the texture.
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, kVerticesNum);

    // Unbind the texture as a precaution.
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GlGenericDrawer::DrawYuv(
    std::vector<uint32_t> yuvTextures, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportX,
    int viewportY, int viewportWidth, int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    PrepareShader(ShaderType::YUV, texMatrix, frameWidth, frameHeight, viewportWidth, viewportHeight);

    // Bind the textures.
    for (int i = 0; i < kYuvTexturesNum; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, yuvTextures[i]);
    }

    // Draw the textures.
    RTC_DLOG(LS_VERBOSE) << "view port: " << viewportX << ", " << viewportY << ", " << viewportWidth << ", "
                         << viewportHeight;
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, kVerticesNum);

    // Unbind the textures as a precaution.
    for (int i = 0; i < kYuvTexturesNum; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void GlGenericDrawer::PrepareShader(
    ShaderType shaderType, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportWidth,
    int viewportHeight)
{
    if (shaderType != currentShaderType_) {
        // Allocate new shader.
        currentShader_ = CreateShader(shaderType);
        RTC_DCHECK(currentShader_);
        currentShaderType_ = shaderType;

        currentShader_->Use();

        // Set input texture units.
        if (shaderType == ShaderType::YUV) {
            currentShader_->SetInt("tex_y", kTextureUnit_Y);
            currentShader_->SetInt("tex_u", kTextureUnit_U);
            currentShader_->SetInt("tex_v", kTextureUnit_V);
        } else {
            currentShader_->SetInt("tex", kTextureUnit_Default);
        }

        positionLocation_ = currentShader_->GetAttribLocation("position");
        textureLocation_ = currentShader_->GetAttribLocation("texCoord");
        texTransformLocation_ = currentShader_->GetUniformLocation("transform");
    } else {
        // Same shader type as before, reuse exising shader.
        currentShader_->Use();
    }

    // Upload the vertex coordinates.
    glEnableVertexAttribArray(positionLocation_);
    glVertexAttribPointer(positionLocation_, kVerticePositionComponents, GL_FLOAT, false, 0, FULL_RECTANGLE_BUFFER);

    // Upload the texture coordinates.
    glEnableVertexAttribArray(textureLocation_);
    glVertexAttribPointer(
        textureLocation_, kTexturePositionComponents, GL_FLOAT, false, 0, FULL_RECTANGLE_TEXTURE_BUFFER);

    // Upload the texture transformation matrix.
    glUniformMatrix4fv(texTransformLocation_, 1, false, texMatrix.data());
}

std::unique_ptr<GlShader> GlGenericDrawer::CreateShader(ShaderType shaderType)
{
    auto shader = std::make_unique<GlShader>();
    switch (shaderType) {
        case ShaderType::OES:
            shader->Compile(DEFAULT_VERTEX_SHADER, OES_FRAGMENT_SHADER);
            break;
        case ShaderType::RGB:
            shader->Compile(DEFAULT_VERTEX_SHADER, RGB_FRAGMENT_SHADER);
            break;
        case ShaderType::YUV:
            shader->Compile(DEFAULT_VERTEX_SHADER, YUV_FRAGMENT_SHADER);
            break;
        default:
            RTC_LOG(LS_ERROR) << "Unsupported shader type: " << shaderType;
            return nullptr;
    }

    return shader;
}

} // namespace webrtc
