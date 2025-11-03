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

#include "video_frame_receiver_native.h"

#include "api/video/i420_buffer.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/logging.h"
#include "libyuv.h"

namespace webrtc {

constexpr int32_t kBufferCount_Default = 8;

std::map<OH_ImageReceiverNative*, VideoFrameReceiverNative*> VideoFrameReceiverNative::receiverMap_;

std::unique_ptr<VideoFrameReceiverNative> VideoFrameReceiverNative::Create(const std::string& threadName)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return std::make_unique<VideoFrameReceiverNative>(threadName);
}

VideoFrameReceiverNative::VideoFrameReceiverNative(const std::string& threadName) : thread_(rtc::Thread::Create())
{
    thread_->SetName(threadName, this);
    thread_->Start();
}

VideoFrameReceiverNative::~VideoFrameReceiverNative()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    ReleaseImageReceiver();
    thread_->Stop();
}

uint64_t VideoFrameReceiverNative::GetSurfaceId() const
{
    uint64_t surfaceId = 0;

    if (imageReceiver_) {
        Image_ErrorCode ret = OH_ImageReceiverNative_GetReceivingSurfaceId(imageReceiver_, &surfaceId);
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to get surface id of image receiver";
        }
    }

    return surfaceId;
}

void VideoFrameReceiverNative::SetVideoFrameSize(int32_t width, int32_t height)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (width <= 0 || height <= 0) {
        RTC_LOG(LS_ERROR) << "invalid size";
        return;
    }

    if (width_ == width && height_ == height) {
        RTC_LOG(LS_VERBOSE) << "Same size";
        return;
    }

    width_ = width;
    height_ = height;

    ReleaseImageReceiver();
    CreateImageReceiver();
}

void VideoFrameReceiverNative::CreateImageReceiver()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    Image_ErrorCode ret = IMAGE_SUCCESS;
    OH_ImageReceiverOptions* options = nullptr;
    OH_ImageReceiverNative* imageReceiver = nullptr;

    do {
        ret = OH_ImageReceiverOptions_Create(&options);
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to create image receiver options";
            break;
        }

        ret = OH_ImageReceiverOptions_SetSize(options, {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_)});
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to set size of image receiver options";
            break;
        }

        ret = OH_ImageReceiverOptions_SetCapacity(options, kBufferCount_Default);
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to set capacity of image receiver options";
            break;
        }

        ret = OH_ImageReceiverNative_Create(options, &imageReceiver);
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to create image receiver";
            break;
        }

        receiverMap_[imageReceiver] = this;
        imageReceiver_ = imageReceiver;
        ret = OH_ImageReceiverNative_On(imageReceiver_, VideoFrameReceiverNative::OnImageReceiverCallback1);
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to set callback of image receiver";
        }
    } while (0);

    if (options) {
        ret = OH_ImageReceiverOptions_Release(options);
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to release image receiver options";
        }
    }
}

void VideoFrameReceiverNative::ReleaseImageReceiver()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (imageReceiver_) {
        Image_ErrorCode ret = OH_ImageReceiverNative_Off(imageReceiver_);
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to unset callback of image receiver";
        }

        ret = OH_ImageReceiverNative_Release(imageReceiver_);
        if (ret != IMAGE_SUCCESS) {
            RTC_LOG(LS_ERROR) << "Failed to release image receiver";
        }

        receiverMap_.erase(imageReceiver_);
        imageReceiver_ = nullptr;
    }
}

void VideoFrameReceiverNative::OnImageReceiverCallback1(OH_ImageReceiverNative* receiver)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto it = receiverMap_.find(receiver);
    if (it == receiverMap_.end()) {
        return;
    }

    it->second->OnImageReceiverCallback();
}

void VideoFrameReceiverNative::OnImageReceiverCallback()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!thread_->IsCurrent()) {
        thread_->PostTask([this] { OnImageReceiverCallback(); });
        return;
    }

    OH_ImageNative* image;
    Image_ErrorCode ret = OH_ImageReceiverNative_ReadNextImage(imageReceiver_, &image);
    if (ret != IMAGE_SUCCESS) {
        RTC_LOG(LS_ERROR) << "Failed to read latest image: " << ret;
        return;
    }

    Image_Size imageSize;
    ret = OH_ImageNative_GetImageSize(image, &imageSize);
    if (ret != IMAGE_SUCCESS) {
        OH_ImageNative_Release(image);
        RTC_LOG(LS_ERROR) << "Failed to get image size: " << ret;
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "Image size: " << imageSize.width << " x " << imageSize.height;

    // ComponentType没有明确的定义，似乎可参照OH_NativeBuffer_Format。此处不检查ComponentType，默认与相机预览的格式一致（RGBA）。
    uint32_t* types;
    size_t typeSize;
    ret = OH_ImageNative_GetComponentTypes(image, nullptr, &typeSize);
    if (ret != IMAGE_SUCCESS || typeSize <= 0) {
        OH_ImageNative_Release(image);
        RTC_LOG(LS_ERROR) << "Failed to get size of component types: " << ret;
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "Component types size: " << typeSize;

    types = new uint32_t[typeSize];
    ret = OH_ImageNative_GetComponentTypes(image, &types, &typeSize);
    if (ret != IMAGE_SUCCESS) {
        OH_ImageNative_Release(image);
        delete[] types;
        RTC_LOG(LS_ERROR) << "Failed to get component types: " << ret;
        return;
    }
    for (uint32_t i = 0; i < typeSize; i++) {
        RTC_DLOG(LS_VERBOSE) << "Component type: " << types[i];
    }

    int32_t rowStride;
    ret = OH_ImageNative_GetRowStride(image, types[0], &rowStride);
    if (ret != IMAGE_SUCCESS) {
        OH_ImageNative_Release(image);
        delete[] types;
        RTC_LOG(LS_ERROR) << "Failed to get row stride: " << ret;
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "Row stride: " << rowStride;

    int32_t pixelStride;
    ret = OH_ImageNative_GetPixelStride(image, types[0], &pixelStride);
    if (ret != IMAGE_SUCCESS) {
        OH_ImageNative_Release(image);
        delete[] types;
        RTC_LOG(LS_ERROR) << "Failed to get pixel stride: " << ret;
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "Pixel stride: " << pixelStride;

    size_t bufferSize;
    ret = OH_ImageNative_GetBufferSize(image, types[0], &bufferSize);
    if (ret != IMAGE_SUCCESS) {
        OH_ImageNative_Release(image);
        delete[] types;
        RTC_LOG(LS_ERROR) << "Failed to get buffer size: " << ret;
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "Buffer size: " << bufferSize;

    OH_NativeBuffer* buffer;
    ret = OH_ImageNative_GetByteBuffer(image, types[0], &buffer);
    if (ret != IMAGE_SUCCESS) {
        OH_ImageNative_Release(image);
        delete[] types;
        RTC_LOG(LS_ERROR) << "Failed to get byte buffer: " << ret;
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "Buffer: " << buffer;

    delete[] types;

    OH_NativeBuffer_Config bufferConfig{};
    OH_NativeBuffer_GetConfig(buffer, &bufferConfig);
    RTC_DLOG(LS_VERBOSE) << "Buffer config: format=" << bufferConfig.format << " usage=" << bufferConfig.usage;

    void* addr = nullptr;
    OH_NativeBuffer_Map(buffer, &addr);
    RTC_DLOG(LS_VERBOSE) << "Buffer map addr: " << addr;

    rtc::scoped_refptr<I420Buffer> i420Buffer = I420Buffer::Create(bufferConfig.width, bufferConfig.height);

    switch (bufferConfig.format) {
        case NATIVEBUFFER_PIXEL_FMT_RGBA_8888: {
            libyuv::ABGRToI420(
                (uint8_t*)addr, bufferConfig.stride, i420Buffer->MutableDataY(), i420Buffer->StrideY(),
                i420Buffer->MutableDataU(), i420Buffer->StrideU(), i420Buffer->MutableDataV(), i420Buffer->StrideV(),
                bufferConfig.width, bufferConfig.height);
            RTC_DLOG(LS_VERBOSE) << "ABGRToI420 ret = " << ret;
        } break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP: {
            int32_t ret = libyuv::NV12ToI420(
                (uint8_t*)addr, bufferConfig.width, (uint8_t*)addr + bufferConfig.width * bufferConfig.height,
                bufferConfig.width, i420Buffer->MutableDataY(), i420Buffer->StrideY(), i420Buffer->MutableDataU(),
                i420Buffer->StrideU(), i420Buffer->MutableDataV(), i420Buffer->StrideV(), bufferConfig.width,
                bufferConfig.height);
            RTC_DLOG(LS_VERBOSE) << "NV12ToI420 ret = " << ret;
        } break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP: {
            int32_t ret = libyuv::NV21ToI420(
                (uint8_t*)addr, bufferConfig.width, (uint8_t*)addr + bufferConfig.width * bufferConfig.height,
                bufferConfig.width, i420Buffer->MutableDataY(), i420Buffer->StrideY(), i420Buffer->MutableDataU(),
                i420Buffer->StrideU(), i420Buffer->MutableDataV(), i420Buffer->StrideV(), bufferConfig.width,
                bufferConfig.height);
            RTC_DLOG(LS_VERBOSE) << "NV21ToI420 ret = " << ret;
        } break;
        default: {
            RTC_LOG(LS_ERROR) << "Unsupported pixel format: " << bufferConfig.format;
            ret = OH_ImageNative_Release(image);
            if (ret != IMAGE_SUCCESS) {
                RTC_LOG(LS_ERROR) << "Failed to release image: " << ret;
            }
        }
            return;
    }

    if (callback_) {
        callback_->OnFrameAvailable(i420Buffer, rtc::TimeMicros(), kVideoRotation_0);
    }

    ret = OH_ImageNative_Release(image);
    if (ret != IMAGE_SUCCESS) {
        RTC_LOG(LS_ERROR) << "Failed to release image: " << ret;
        return;
    }
}

} // namespace webrtc
