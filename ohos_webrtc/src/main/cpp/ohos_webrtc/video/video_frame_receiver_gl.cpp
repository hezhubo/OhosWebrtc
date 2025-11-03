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

#include "video_frame_receiver_gl.h"
#include "../render/egl_config_attributes.h"
#include "../render/yuv_converter.h"

#include "rtc_base/logging.h"
#include "rtc_base/time_utils.h"

#include <GLES2/gl2ext.h>

namespace webrtc {

std::unique_ptr<VideoFrameReceiverGl>
VideoFrameReceiverGl::Create(const std::string& threadName, std::shared_ptr<EglContext> sharedContext)
{
    return std::make_unique<VideoFrameReceiverGl>(threadName, sharedContext);
}

VideoFrameReceiverGl::VideoFrameReceiverGl(const std::string& threadName, std::shared_ptr<EglContext> sharedContext)
    : thread_(rtc::Thread::Create()), yuvConverter_(std::make_unique<YuvConverter>())
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << ": this=" << this;

    thread_->SetName(threadName, this);
    thread_->Start();
    thread_->BlockingCall([this, sharedContext] {
        eglEnv_ = EglEnv::Create(sharedContext, EglConfigAttributes::RGBA_PIXEL_BUFFER);
        eglEnv_->CreatePbufferSurface(1, 1);
        eglEnv_->MakeCurrent();
        CreateNativeImage();
    });
}

VideoFrameReceiverGl::~VideoFrameReceiverGl()
{
    thread_->PostTask([this] { ReleaseNativeImage(); });
    thread_->Stop();
}

uint64_t VideoFrameReceiverGl::GetSurfaceId() const
{
    return nativeImage_.GetSurfaceId();
}

void VideoFrameReceiverGl::SetVideoFrameSize(int width, int height)
{
    SetTextureSize(width, height);
}

bool VideoFrameReceiverGl::CreateNativeImage()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // 创建 OpenGL 纹理
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    // 创建 NativeImage 实例，关联 OpenGL 纹理
    nativeImage_ = ohos::NativeImage::Create(textureId, GL_TEXTURE_EXTERNAL_OES);
    if (nativeImage_.IsEmpty()) {
        return false;
    }

    OH_OnFrameAvailableListener listener;
    listener.context = this;
    listener.onFrameAvailable = &VideoFrameReceiverGl::OnNativeImageFrameAvailable1;
    nativeImage_.SetOnFrameAvailableListener(listener);

    textureData_ = std::make_shared<TextureData>(textureId, TextureData::Type::OES, thread_.get(), yuvConverter_.get());

    return true;
}

void VideoFrameReceiverGl::ReleaseNativeImage()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    nativeImage_.UnsetOnFrameAvailableListener();
    nativeImage_.Reset();

    std::shared_ptr<TextureData> temp;
    textureData_.swap(temp);
    auto textureId = temp->GetId();
    glDeleteTextures(1, &textureId);

    yuvConverter_.reset();
}

void VideoFrameReceiverGl::SetTextureSize(int32_t textureWidth, int32_t textureHeight)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (textureWidth <= 0 || textureHeight <= 0) {
        RTC_LOG(LS_ERROR) << "Texture size must be positive: " << textureWidth << "x" << textureHeight;
        return;
    }

    auto nativeWindow = nativeImage_.AcquireNativeWindow();
    int32_t ret =
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow.Raw(), SET_BUFFER_GEOMETRY, textureWidth, textureHeight);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to set buffer geometry: " << ret;
        return;
    }

    thread_->PostTask([this, textureWidth, textureHeight]() {
        this->width_ = textureWidth;
        this->height_ = textureHeight;
    });
}

void VideoFrameReceiverGl::OnNativeImageFrameAvailable1(void* data)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!data) {
        return;
    }

    auto self = reinterpret_cast<VideoFrameReceiverGl*>(data);
    self->OnNativeImageFrameAvailable();
}

void VideoFrameReceiverGl::OnNativeImageFrameAvailable()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!thread_->IsCurrent()) {
        thread_->PostTask([this] { OnNativeImageFrameAvailable(); });
        return;
    }

    textureData_->Lock();
    // 更新内容到OpenGL纹理。
    nativeImage_.UpdateSurfaceImage();
    textureData_->Unlock();

    if (ohos::NativeError::HasPendingException()) {
        auto error = ohos::NativeError::GetAndClearPendingException();
        RTC_LOG(LS_ERROR) << "Failed to update surface image: " << error.Code() << ", " << error.what();
        return;
    }

    // 获取最近调用OH_NativeImage_UpdateSurfaceImage的纹理图像的时间戳和变化矩阵。
    auto timestamp = nativeImage_.GetTimestamp();
    auto matrix = nativeImage_.GetTransformMatrixV2();
    RTC_DLOG(LS_VERBOSE) << "timestamp: " << timestamp;
    RTC_DLOG(LS_VERBOSE) << "matrix: " << RenderCommon::DumpGLMatrixDataToString(matrix);

    auto timestampUs = timestampConverter_.Convert(timestamp);
    RTC_DLOG(LS_VERBOSE) << "timestampUs=" << timestampUs;

    // create video frame
    auto buffer =
        TextureBuffer::Create(textureData_, width_, height_, RenderCommon::ConvertGLMatrixDataToMatrix(matrix));

    if (callback_) {
        callback_->OnFrameAvailable(buffer, timestampUs > 0 ? timestampUs : rtc::TimeMicros(), kVideoRotation_0);
    }
}

} // namespace webrtc
