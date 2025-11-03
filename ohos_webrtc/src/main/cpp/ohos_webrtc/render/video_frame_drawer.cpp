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
#include "video_frame_drawer.h"

#include "rtc_base/logging.h"

#include <GLES3/gl3.h>

namespace {
constexpr int32_t kYuvTextureArrayLength = 3;
}

namespace webrtc {

void VideoFrameDrawer::DrawFrame(const VideoFrame& frame, GlDrawer& drawer, const Matrix& additionalRenderMatrix)
{
    DrawFrame(frame, drawer, additionalRenderMatrix, 0, 0, frame.width(), frame.height());
}

void VideoFrameDrawer::DrawFrame(
    const VideoFrame& frame, GlDrawer& drawer, const Matrix& additionalRenderMatrix, int viewportX, int viewportY,
    int viewportWidth, int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    renderMatrix_.Reset();
    if (!frame.is_texture()) {
        renderMatrix_.PreScale(1.0f, -1.0f, 0.5f, 0.5f); // I420-frames are upside down
    }
    renderMatrix_.PreRotate(frame.rotation(), 0.5f, 0.5f);
    renderMatrix_.PreConcat(additionalRenderMatrix);
    RTC_DLOG(LS_VERBOSE) << "Render matrix: " << renderMatrix_;

    if (frame.is_texture()) {
        auto buffer = static_cast<TextureBuffer*>(frame.video_frame_buffer().get());
        DrawTexture(
            rtc::scoped_refptr<TextureBuffer>(buffer), drawer, renderMatrix_, frame.width(), frame.width(), viewportX,
            viewportY, viewportWidth, viewportHeight);
    } else {
        if (yuvTextures_.size() == 0) {
            yuvTextures_.resize(kYuvTextureArrayLength);

            glGenTextures(yuvTextures_.size(), yuvTextures_.data());
            for (uint32_t i = 0; i < yuvTextures_.size(); i++) {
                glBindTexture(GL_TEXTURE_2D, yuvTextures_[i]);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            }
        }

        auto buffer = frame.video_frame_buffer()->ToI420();

        const uint8_t* planes[] = {buffer->DataY(), buffer->DataU(), buffer->DataV()};
        int planeWidths[] = {buffer->StrideY(), buffer->StrideU(), buffer->StrideV()};
        int planeHeights[] = {buffer->height(), buffer->ChromaHeight(), buffer->ChromaHeight()};

        for (uint32_t i = 0; i < yuvTextures_.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, yuvTextures_[i]);

            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_LUMINANCE, planeWidths[i], planeHeights[i], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                planes[i]);
        }

        auto glFinalMatrix = RenderCommon::ConvertMatrixToGLMatrixData(renderMatrix_);
        drawer.DrawYuv(
            yuvTextures_, glFinalMatrix, frame.width(), frame.height(), viewportX, viewportY, viewportWidth,
            viewportHeight);
    }
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

void VideoFrameDrawer::DrawTexture(
    rtc::scoped_refptr<TextureBuffer> buffer, GlDrawer& drawer, const Matrix& renderMatrix, int frameWidth,
    int frameHeight, int viewportX, int viewportY, int viewportWidth, int viewportHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!buffer) {
        RTC_LOG(LS_ERROR) << "Buffer is null";
        return;
    }

    auto textureData = buffer->GetTexture();
    if (!textureData) {
        RTC_LOG(LS_ERROR) << "Buffer is released";
        return;
    }

    textureData->Lock();

    Matrix finalMatrix = buffer->GetTransformMatrix();
    finalMatrix.PreConcat(renderMatrix);
    RTC_DLOG(LS_VERBOSE) << "Final matrix: " << finalMatrix;
    auto glFinalMatrix = RenderCommon::ConvertMatrixToGLMatrixData(finalMatrix);
    RTC_DLOG(LS_VERBOSE) << "Gl final matrix: " << RenderCommon::DumpGLMatrixDataToString(glFinalMatrix);

    switch (textureData->GetType()) {
        case TextureData::Type::OES:
            drawer.DrawOes(
                textureData->GetId(), glFinalMatrix, frameWidth, frameHeight, viewportX, viewportY, viewportWidth,
                viewportHeight);
            break;
        case TextureData::Type::RGB:
            drawer.DrawRgb(
                textureData->GetId(), glFinalMatrix, frameWidth, frameHeight, viewportX, viewportY, viewportWidth,
                viewportHeight);
            break;
        default:
            RTC_LOG(LS_ERROR) << "Unknown texture type: " << textureData->GetType();
            break;
    }

    textureData->Unlock();
}

} // namespace webrtc
