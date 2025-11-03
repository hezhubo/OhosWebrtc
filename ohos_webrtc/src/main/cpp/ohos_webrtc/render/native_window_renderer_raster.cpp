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

#include "native_window_renderer_raster.h"

#include "api/video/i420_buffer.h"
#include "rtc_base/logging.h"
#include "libyuv.h"

namespace webrtc {
namespace adapter {

std::unique_ptr<NativeWindowRendererRaster> NativeWindowRendererRaster::Create(ohos::NativeWindow window)
{
    if (window.IsEmpty()) {
        return nullptr;
    }

    return std::make_unique<NativeWindowRendererRaster>(std::move(window));
}

NativeWindowRendererRaster::NativeWindowRendererRaster(ohos::NativeWindow window)
    : NativeWindowRenderer(std::move(window)), thread_(rtc::Thread::Create())
{
    // [height] is before [width]
    int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(window_.Raw(), GET_BUFFER_GEOMETRY, &height_, &width_);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer geometry: " << ret;
    }
    RTC_DLOG(LS_VERBOSE) << "Window geometry: " << width_ << "x" << height_;

    ret = OH_NativeWindow_NativeWindowHandleOpt(window_.Raw(), GET_USAGE, &usage_);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to get usage: " << ret;
    }
    RTC_DLOG(LS_VERBOSE) << "Window usage: " << usage_;

    ret = OH_NativeWindow_NativeWindowHandleOpt(window_.Raw(), GET_FORMAT, &format_);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to get format: " << ret;
    }
    RTC_DLOG(LS_VERBOSE) << "Window format: " << format_;

    int32_t stride = 0;
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_.Raw(), GET_STRIDE, &stride);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to get stride: " << ret;
    }
    RTC_DLOG(LS_VERBOSE) << "Window stride: " << stride;

    ret = OH_NativeWindow_NativeWindowHandleOpt(window_.Raw(), GET_TRANSFORM, &transform_);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to get transform: " << ret;
    }
    RTC_DLOG(LS_VERBOSE) << "Window transform: " << transform_;

    // set
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_.Raw(), SET_USAGE, usage_ | NATIVEBUFFER_USAGE_CPU_WRITE);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to set usage: " << ret;
    }

    ret = OH_NativeWindow_NativeWindowSetScalingModeV2(window_.Raw(), OH_SCALING_MODE_SCALE_FIT_V2);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to set scale mode: " << ret;
    }

    thread_->SetName("window-renderer-thread", this);
    thread_->Start();
}

NativeWindowRendererRaster::~NativeWindowRendererRaster()
{
    thread_->Stop();
}

void NativeWindowRendererRaster::OnFrame(const VideoFrame& frame)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " this=" << this;
    RTC_DLOG(LS_VERBOSE) << "render frame, id=" << frame.id() << " size=" << frame.width() << "x" << frame.height()
                         << ", timestamp=" << frame.timestamp_us();

    if (!frame.video_frame_buffer()) {
        RTC_LOG(LS_ERROR) << "Buffer is null";
        return;
    }

    thread_->PostTask([this, frame] { RenderByteBuffer(frame.video_frame_buffer()->ToI420()); });
}

void NativeWindowRendererRaster::OnDiscardedFrame()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

void NativeWindowRendererRaster::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
}

void NativeWindowRendererRaster::RenderByteBuffer(rtc::scoped_refptr<I420BufferInterface> buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " enter, this=" << this;

    if (!buffer) {
        RTC_LOG(LS_ERROR) << "Buffer is nullptr";
        return;
    }

    if (format_ != NATIVEBUFFER_PIXEL_FMT_RGBA_8888) {
        RTC_DLOG(LS_VERBOSE) << "Set format";
        int32_t ret =
            OH_NativeWindow_NativeWindowHandleOpt(window_.Raw(), SET_FORMAT, NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
        if (ret != 0) {
            RTC_LOG(LS_ERROR) << "Failed to set format: " << ret;
            return;
        } else {
            format_ = NATIVEBUFFER_PIXEL_FMT_RGBA_8888;
        }
    }

    int width = buffer->width();
    int height = buffer->height();

    if (width_ != width || height_ != height) {
        RTC_DLOG(LS_VERBOSE) << "Set buffer geometry: " << width << "x" << height;
        int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(window_.Raw(), SET_BUFFER_GEOMETRY, width, height);
        if (ret != 0) {
            RTC_LOG(LS_ERROR) << "Failed to set buffer geometry: " << ret;
            return;
        } else {
            width_ = width;
            height_ = height;
        }
    }

    auto windowBuffer = window_.RequestBuffer(true);
    auto dstBuffer = ohos::NativeBuffer::From(windowBuffer.Raw());
    auto dstConfig = dstBuffer.GetConfig();
    RTC_DLOG(LS_VERBOSE) << "dst buffer config: " << dstConfig.width << ", " << dstConfig.height << ", "
                         << dstConfig.format << ", " << dstConfig.stride << ", " << dstConfig.usage;

    auto dstAddr = dstBuffer.Map();
    if (!dstAddr) {
        RTC_LOG(LS_ERROR) << "Failed to map dstBuffer";
        window_.AbortBuffer(windowBuffer.Raw());
        return;
    }

    if (dstConfig.format != NATIVEBUFFER_PIXEL_FMT_RGBA_8888) {
        RTC_LOG(LS_ERROR) << "Window buffer format is not rgba";
        window_.AbortBuffer(windowBuffer.Raw());
        return;
    }

    int ret = libyuv::I420ToABGR(
        buffer->DataY(), buffer->StrideY(), buffer->DataU(), buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
        (uint8_t*)dstAddr, dstConfig.stride, width, height);
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to convert i420 to rgba: " << ret;
        window_.AbortBuffer(windowBuffer.Raw());
        return;
    }

    window_.FlushBuffer(windowBuffer.Raw());
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " exit";
}

} // namespace adapter
} // namespace webrtc
