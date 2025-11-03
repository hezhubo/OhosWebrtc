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

#include "yuv_converter.h"

#include "api/video/i420_buffer.h"
#include "rtc_base/logging.h"
#include "libyuv.h"

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <cstdint>

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
uniform vec2 xUnit;
// Color conversion coefficients, including constant term
uniform vec4 coefficients;

void main() {
    gl_FragColor.r = coefficients.a + dot(coefficients.rgb, texture2D(tex, vTexCoord - 1.5 * xUnit).rgb);
    gl_FragColor.g = coefficients.a + dot(coefficients.rgb, texture2D(tex, vTexCoord - 0.5 * xUnit).rgb);
    gl_FragColor.b = coefficients.a + dot(coefficients.rgb, texture2D(tex, vTexCoord + 0.5 * xUnit).rgb);
    gl_FragColor.a = coefficients.a + dot(coefficients.rgb, texture2D(tex, vTexCoord + 1.5 * xUnit).rgb);
}
)";

static constexpr char RGB_FRAGMENT_SHADER[] = R"(
precision mediump float;
varying vec2 vTexCoord;
uniform sampler2D tex;
uniform vec2 xUnit;
// Color conversion coefficients, including constant term
uniform vec4 coefficients;

void main() {
    gl_FragColor.r = coefficients.a + dot(coefficients.rgb, texture2D(tex, vTexCoord - 1.5 * xUnit).rgb);
    gl_FragColor.g = coefficients.a + dot(coefficients.rgb, texture2D(tex, vTexCoord - 0.5 * xUnit).rgb);
    gl_FragColor.b = coefficients.a + dot(coefficients.rgb, texture2D(tex, vTexCoord + 0.5 * xUnit).rgb);
    gl_FragColor.a = coefficients.a + dot(coefficients.rgb, texture2D(tex, vTexCoord + 1.5 * xUnit).rgb);
}
)";

static constexpr char YUV_FRAGMENT_SHADER[] = R"(
precision mediump float;
varying vec2 vTexCoord;
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
uniform vec2 xUnit;
// Color conversion coefficients, including constant term
uniform vec4 coefficients;

vec4 sample(vec2 p) {
  float y = texture2D(tex_y, p).r * 1.16438;
  float u = texture2D(tex_u, p).r;
  float v = texture2D(tex_v, p).r;
  return vec4(y + 1.59603 * v - 0.874202, y - 0.391762 * u - 0.812968 * v + 0.531668, y + 2.01723 * u - 1.08563, 1);
}

void main() {
    gl_FragColor.r = coefficients.a + dot(coefficients.rgb, sample(vTexCoord - 1.5 * xUnit).rgb);
    gl_FragColor.g = coefficients.a + dot(coefficients.rgb, sample(vTexCoord - 0.5 * xUnit).rgb);
    gl_FragColor.b = coefficients.a + dot(coefficients.rgb, sample(vTexCoord + 0.5 * xUnit).rgb);
    gl_FragColor.a = coefficients.a + dot(coefficients.rgb, sample(vTexCoord + 1.5 * xUnit).rgb);
}
)";

static constexpr float FULL_RECTANGLE_BUFFER[] = {
    -1.0f, -1.0f, // Bottom left
    1.0f,  -1.0f, // Bottom right
    -1.0f, 1.0f,  // Top left
    1.0f,  1.0f,  // Top right
};

// Texture coordinates - (0, 0) is bottom-left and (1, 1) is top-right
static constexpr float FULL_RECTANGLE_TEXTURE_BUFFER[] = {
    0.0f, 0.0f, // Bottom left
    1.0f, 0.0f, // Bottom right
    0.0f, 1.0f, // Top left
    1.0f, 1.0f, // Top right
};

constexpr int32_t kVerticePositionComponents = 2;
constexpr int32_t kTexturePositionComponents = 2;

constexpr int32_t kVerticesNum =
    sizeof(FULL_RECTANGLE_BUFFER) / sizeof(FULL_RECTANGLE_BUFFER[0]) / kVerticePositionComponents;

constexpr int32_t kTextureUnit_Default = 0;
constexpr int32_t kTextureUnit_Y = 0;
constexpr int32_t kTextureUnit_U = 1;
constexpr int32_t kTextureUnit_V = 2;

constexpr int32_t kBufferAlignment = 64;
constexpr int32_t kCoefficientsNum = 4;

} // namespace

class GlConverterDrawer : public GlDrawer {
public:
    enum class ShaderType {
        UNKNOWN = -1,
        OES,
        RGB,
        YUV
    };

    GlConverterDrawer() = default;
    ~GlConverterDrawer() override = default;

    void DrawOes(
        int oesTextureId, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportX, int viewportY,
        int viewportWidth, int viewportHeight) override;
    void DrawRgb(
        int textureId, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportX, int viewportY,
        int viewportWidth, int viewportHeight) override;
    void DrawYuv(
        std::vector<uint32_t> yuvTextures, const GLMatrixData& texMatrix, int frameWidth, int frameHeight,
        int viewportX, int viewportY, int viewportWidth, int viewportHeight) override;

    void SetStepSize(float stepSize);
    void SetCoefficients(std::array<float, kCoefficientsNum> coefficients);

protected:
    void PrepareShader(
        ShaderType shaderType, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportWidth,
        int viewportHeight);
    std::unique_ptr<GlShader> CreateShader(ShaderType shaderType);

private:
    ShaderType currentShaderType_{ShaderType::UNKNOWN};
    std::unique_ptr<GlShader> currentShader_;
    int positionLocation_;
    int textureLocation_;
    int texTransformLocation_;
    int xUnitLocation_;
    int coefficientsLocation_;
    float stepSize_;
    std::array<float, kCoefficientsNum> coefficients_;
};

void GlConverterDrawer::DrawOes(
    int oesTextureId, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportX, int viewportY,
    int viewportWidth, int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    PrepareShader(ShaderType::OES, texMatrix, frameWidth, frameHeight, viewportWidth, viewportHeight);

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, oesTextureId);

    // Draw the texture
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, kVerticesNum);

    // Unbind the texture as a precaution
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
}

void GlConverterDrawer::DrawRgb(
    int textureId, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportX, int viewportY,
    int viewportWidth, int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    PrepareShader(ShaderType::RGB, texMatrix, frameWidth, frameHeight, viewportWidth, viewportHeight);

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Draw the texture
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, kVerticesNum);

    // Unbind the texture as a precaution
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GlConverterDrawer::DrawYuv(
    std::vector<uint32_t> yuvTextures, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportX,
    int viewportY, int viewportWidth, int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    PrepareShader(ShaderType::YUV, texMatrix, frameWidth, frameHeight, viewportWidth, viewportHeight);

    // Bind the textures
    for (uint32_t i = 0; i < yuvTextures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, yuvTextures[i]);
    }

    // Draw the textures
    RTC_DLOG(LS_VERBOSE) << "view port: " << viewportX << ", " << viewportY << ", " << viewportWidth << ", "
                         << viewportHeight;
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, kVerticesNum);

    // Unbind the textures as a precaution
    for (uint32_t i = 0; i < yuvTextures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void GlConverterDrawer::SetStepSize(float stepSize)
{
    stepSize_ = stepSize;
}

void GlConverterDrawer::SetCoefficients(std::array<float, kCoefficientsNum> coefficients)
{
    coefficients_ = coefficients;
}

void GlConverterDrawer::PrepareShader(
    ShaderType shaderType, const GLMatrixData& texMatrix, int frameWidth, int frameHeight, int viewportWidth,
    int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    (void)frameHeight;
    (void)viewportWidth;
    (void)viewportHeight;

    if (frameWidth == 0) {
        return;
    }

    if (shaderType != currentShaderType_) {
        currentShader_ = CreateShader(shaderType);
        RTC_DCHECK(currentShader_);
        currentShaderType_ = shaderType;

        currentShader_->Use();

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
        xUnitLocation_ = currentShader_->GetUniformLocation("xUnit");
        coefficientsLocation_ = currentShader_->GetUniformLocation("coefficients");
    } else {
        // Same shader type as before, reuse exising shader
        currentShader_->Use();
    }

    // Upload the vertex coordinates
    glEnableVertexAttribArray(positionLocation_);
    glVertexAttribPointer(positionLocation_, kVerticePositionComponents, GL_FLOAT, false, 0, FULL_RECTANGLE_BUFFER);

    // Upload the texture coordinates
    glEnableVertexAttribArray(textureLocation_);
    glVertexAttribPointer(
        textureLocation_, kTexturePositionComponents, GL_FLOAT, false, 0, FULL_RECTANGLE_TEXTURE_BUFFER);

    // Upload the texture transformation matrix
    glUniformMatrix4fv(texTransformLocation_, 1, false, texMatrix.data());

    glUniform4fv(coefficientsLocation_, 1, coefficients_.data());
    glUniform2f(xUnitLocation_, stepSize_ * texMatrix[0] / frameWidth, stepSize_ * texMatrix[1] / frameWidth);
}

std::unique_ptr<GlShader> GlConverterDrawer::CreateShader(ShaderType shaderType)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

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

class LocalI420Buffer : public I420BufferInterface {
public:
    static rtc::scoped_refptr<LocalI420Buffer> Wrap(
        std::unique_ptr<uint8_t, AlignedFreeDeleter> data, int width, int height, const uint8_t* dataY, int strideY,
        const uint8_t* dataU, int strideU, const uint8_t* dataV, int strideV);

    int width() const override;
    int height() const override;

    const uint8_t* DataY() const override;
    const uint8_t* DataU() const override;
    const uint8_t* DataV() const override;

    int StrideY() const override;
    int StrideU() const override;
    int StrideV() const override;

    rtc::scoped_refptr<VideoFrameBuffer> CropAndScale(
        int offset_x, int offset_y, int crop_width, int crop_height, int scaled_width, int scaled_height) override;

protected:
    LocalI420Buffer(
        std::unique_ptr<uint8_t, AlignedFreeDeleter> data, int width, int height, const uint8_t* dataY, int strideY,
        const uint8_t* dataU, int strideU, const uint8_t* dataV, int strideV);

    ~LocalI420Buffer() override = default;

private:
    const std::unique_ptr<uint8_t, AlignedFreeDeleter> data_;
    const int width_;
    const int height_;
    const uint8_t* dataY_;
    const uint8_t* dataU_;
    const uint8_t* dataV_;
    const int strideY_;
    const int strideU_;
    const int strideV_;
};

rtc::scoped_refptr<LocalI420Buffer> LocalI420Buffer::Wrap(
    std::unique_ptr<uint8_t, AlignedFreeDeleter> data, int width, int height, const uint8_t* dataY, int strideY,
    const uint8_t* dataU, int strideU, const uint8_t* dataV, int strideV)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (dataY == nullptr || dataU == nullptr || dataV == nullptr) {
        RTC_LOG(LS_ERROR) << "Data buffers cannot be null";
        return nullptr;
    }

    return rtc::make_ref_counted<LocalI420Buffer>(
        std::move(data), width, height, dataY, strideY, dataU, strideU, dataV, strideV);
}

LocalI420Buffer::LocalI420Buffer(
    std::unique_ptr<uint8_t, AlignedFreeDeleter> data, int width, int height, const uint8_t* dataY, int strideY,
    const uint8_t* dataU, int strideU, const uint8_t* dataV, int strideV)
    : data_(std::move(data)),
      width_(width),
      height_(height),
      dataY_(dataY),
      dataU_(dataU),
      dataV_(dataV),
      strideY_(strideY),
      strideU_(strideU),
      strideV_(strideV)
{
}

int LocalI420Buffer::width() const
{
    return width_;
}

int LocalI420Buffer::height() const
{
    return height_;
}

const uint8_t* LocalI420Buffer::DataY() const
{
    return dataY_;
}

const uint8_t* LocalI420Buffer::DataU() const
{
    return dataU_;
}

const uint8_t* LocalI420Buffer::DataV() const
{
    return dataV_;
}

int LocalI420Buffer::StrideY() const
{
    return strideY_;
}

int LocalI420Buffer::StrideU() const
{
    return strideU_;
}

int LocalI420Buffer::StrideV() const
{
    return strideV_;
}

rtc::scoped_refptr<VideoFrameBuffer> LocalI420Buffer::CropAndScale(
    int offset_x, int offset_y, int crop_width, int crop_height, int scaled_width, int scaled_height)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    const uint8_t* srcY = DataY() + offset_y * StrideY() + offset_x;
    const uint8_t* srcU = DataU() + offset_y / 2 * StrideU() + offset_x / 2;
    const uint8_t* srcV = DataV() + offset_y / 2 * StrideV() + offset_x / 2;

    auto newBuffer = I420Buffer::Create(scaled_width, scaled_height);

    if (crop_width == scaled_width && crop_height == scaled_height) {
        bool ret = libyuv::I420Copy(
            srcY, StrideY(), srcU, StrideU(), srcV, StrideV(), newBuffer->MutableDataY(), newBuffer->StrideY(),
            newBuffer->MutableDataU(), newBuffer->StrideU(), newBuffer->MutableDataV(), newBuffer->StrideV(),
            scaled_width, scaled_height);
        RTC_DCHECK_EQ(ret, 0) << "I420Copy failed";
    } else {
        bool ret = libyuv::I420Scale(
            srcY, StrideY(), srcU, StrideU(), srcV, StrideV(), crop_width, crop_height, newBuffer->MutableDataY(),
            newBuffer->StrideY(), newBuffer->MutableDataU(), newBuffer->StrideU(), newBuffer->MutableDataV(),
            newBuffer->StrideV(), scaled_width, scaled_height, libyuv::kFilterBox);
        RTC_DCHECK_EQ(ret, 0) << "I420Scale failed";
    }

    return newBuffer;
}

YuvConverter::YuvConverter()
    : drawer_(std::make_unique<GlConverterDrawer>()), frameDrawer_(std::make_unique<VideoFrameDrawer>())
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

YuvConverter::~YuvConverter()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    drawer_.reset();
    frameDrawer_.reset();
    glDeleteTextures(1, &textureId_);
    textureId_ = 0;
    glDeleteFramebuffers(1, &frameBufferId_);
    frameBufferId_ = 0;
    frameBufferWidth_ = 0;
    frameBufferHeight_ = 0;
}

rtc::scoped_refptr<I420BufferInterface> YuvConverter::Convert(rtc::scoped_refptr<TextureBuffer> textureBuffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!textureBuffer) {
        RTC_LOG(LS_ERROR) << "Texture buffer is null";
        return nullptr;
    }

    int frameWidth = textureBuffer->width();
    int frameHeight = textureBuffer->height();
    int stride = ((frameWidth + 7) / 8) * 8;
    int uvHeight = (frameHeight + 1) / 2;
    int totalHeight = frameHeight + uvHeight;
    // Viewport width is divided by four since we are squeezing in four color bytes in each RGBA pixel
    int viewportWidth = stride / 4;

    if (!PrepareFrameBuffer(viewportWidth, totalHeight)) {
        return nullptr;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId_);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        RTC_LOG(LS_ERROR) << "Failed to call glBindFramebuffer: " << error;
        return nullptr;
    }

    Matrix renderMatrix;
    renderMatrix.PreScale(1.0f, -1.0f, 0.5f, 0.5f);

    // Draw Y
    drawer_->SetStepSize(1.0f);
    drawer_->SetCoefficients({0.256788f, 0.504129f, 0.0979059f, 0.0627451f});
    frameDrawer_->DrawTexture(
        textureBuffer, *drawer_, renderMatrix, frameWidth, frameHeight, 0, 0, viewportWidth, frameHeight);

    // Draw U
    drawer_->SetStepSize(2.0f);
    drawer_->SetCoefficients({-0.148223f, -0.290993f, 0.439216f, 0.501961f});
    frameDrawer_->DrawTexture(
        textureBuffer, *drawer_, renderMatrix, frameWidth, frameHeight, 0, frameHeight, viewportWidth / 2, uvHeight);

    // Draw V
    drawer_->SetStepSize(2.0f);
    drawer_->SetCoefficients({0.439216f, -0.367788f, -0.0714274f, 0.501961f});
    frameDrawer_->DrawTexture(
        textureBuffer, *drawer_, renderMatrix, frameWidth, frameHeight, viewportWidth / 2, frameHeight,
        viewportWidth / 2, uvHeight);

    std::unique_ptr<uint8_t, AlignedFreeDeleter> i420Buffer(
        static_cast<uint8_t*>(AlignedMalloc(stride * totalHeight, kBufferAlignment)));
    glReadPixels(0, 0, frameBufferWidth_, frameBufferHeight_, GL_RGBA, GL_UNSIGNED_BYTE, i420Buffer.get());
    error = glGetError();
    if (error != GL_NO_ERROR) {
        RTC_LOG(LS_ERROR) << "Failed to call glReadPixels: " << error;
        return nullptr;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int yPos = 0;
    int uPos = yPos + stride * frameHeight;
    int vPos = uPos + stride / 2;

    const uint8_t* dataY = i420Buffer.get() + yPos;
    const uint8_t* dataU = i420Buffer.get() + uPos;
    const uint8_t* dataV = i420Buffer.get() + vPos;

    return LocalI420Buffer::Wrap(
        std::move(i420Buffer), frameWidth, frameHeight, dataY, stride, dataU, stride, dataV, stride);
}

bool YuvConverter::PrepareFrameBuffer(int width, int height)
{
    if (width <= 0 || height <= 0) {
        RTC_LOG(LS_ERROR) << "Invalid size: " << width << "x" << height;
        return false;
    }

    if (width == frameBufferWidth_ && height == frameBufferHeight_) {
        return true;
    }

    frameBufferWidth_ = width;
    frameBufferHeight_ = height;

    if (textureId_ == 0) {
        glGenTextures(1, &textureId_);
        glBindTexture(GL_TEXTURE_2D, textureId_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    if (frameBufferId_ == 0) {
        glGenFramebuffers(1, &frameBufferId_);
    }

    // Allocate texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        RTC_LOG(LS_ERROR) << "Failed to prepare frame buffer: " << error;
        return false;
    }

    // Attach the texture to the framebuffer as color attachment
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId_, 0);

    // Check that the framebuffer is in a good state
    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        RTC_LOG(LS_ERROR) << "Framebuffer not complete, status: " << status;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

} // namespace webrtc
