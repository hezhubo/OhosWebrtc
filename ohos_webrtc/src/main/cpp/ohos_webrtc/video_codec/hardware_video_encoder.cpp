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

#include "hardware_video_encoder.h"
#include "../render/native_window_renderer_gl.h"
#include "../video_codec/video_codec_mime_type.h"
#include "../helper/native_buffer.h"
#include "../utils/marcos.h"

#include <cstdint>
#include <vector>

#include <multimedia/player_framework/native_averrors.h>
#include <multimedia/player_framework/native_avcapability.h>

#include "api/video/video_codec_type.h"
#include "render/egl_config_attributes.h"
#include <api/video_codecs/h264_profile_level_id.h>
#include <modules/video_coding/include/video_codec_interface.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <rtc_base/time_utils.h>
#include <rtc_base/logging.h>
#include <libyuv.h>

namespace webrtc {
namespace adapter {

constexpr int32_t kRequestedResolutionAlignment = 16;

// QP scaling thresholds.
static const int kH264QpThresholdLow = 24;
static const int kH264QpThresholdHigh = 37;

std::unique_ptr<HardwareVideoEncoder> HardwareVideoEncoder::Create(
    const std::string& codecName, int32_t pixelFormat, const SdpVideoFormat& format,
    std::shared_ptr<EglContext> sharedContext)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return std::unique_ptr<HardwareVideoEncoder>(
        new HardwareVideoEncoder(codecName, pixelFormat, format, sharedContext));
}

HardwareVideoEncoder::HardwareVideoEncoder(
    const std::string& codecName, int32_t pixelFormat, const SdpVideoFormat& format,
    std::shared_ptr<EglContext> sharedContext)
    : codecName_(codecName),
      pixelFormat_(pixelFormat),
      format_(format),
      sharedContext_(sharedContext),
      textureDrawer_(std::make_unique<GlGenericDrawer>()),
      videoFrameDrawer_(std::make_unique<VideoFrameDrawer>())
{
    UpdateEncoderInfo();
}

HardwareVideoEncoder::~HardwareVideoEncoder()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    Release();
}

int HardwareVideoEncoder::InitEncode(const VideoCodec* codec_settings, const Settings& settings)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    codecSettings_ = *codec_settings;

    curBitrate_ = codecSettings_.startBitrate * 1000; // kilobits/sec to bits/sec
    targetBitrate_ = curBitrate_;
    curFramerate_ = codecSettings_.maxFramerate;
    targetFramerate_ = curFramerate_;

    RTC_DLOG(LS_INFO) << "codec settings: codecType=" << codecSettings_.codecType;
    RTC_DLOG(LS_INFO) << "codec settings: width=" << codecSettings_.width;
    RTC_DLOG(LS_INFO) << "codec settings: height=" << codecSettings_.height;
    RTC_DLOG(LS_INFO) << "codec settings: startBitrate=" << codecSettings_.startBitrate;
    RTC_DLOG(LS_INFO) << "codec settings: minBitrate=" << codecSettings_.minBitrate;
    RTC_DLOG(LS_INFO) << "codec settings: maxBitrate=" << codecSettings_.maxBitrate;
    RTC_DLOG(LS_INFO) << "codec settings: maxFramerate=" << codecSettings_.maxFramerate;
    RTC_DLOG(LS_INFO) << "codec settings: expect_encode_from_texture=" << codecSettings_.expect_encode_from_texture;
    if (codecSettings_.codecType == kVideoCodecH264) {
        RTC_DLOG(LS_INFO) << "codec settings: H264.keyFrameInterval=" << codecSettings_.H264()->keyFrameInterval;
    }

    auto type = VideoCodecMimeType::valueOf(format_.name);
    OH_AVCapability* capability = OH_AVCodec_GetCapabilityByCategory(type.mimeType(), true, HARDWARE);
    OH_AVCapability_GetEncoderBitrateRange(capability, &supportedBitrateRange_);
    RTC_DLOG(LS_VERBOSE) << "supportedBitrateRange=[" << supportedBitrateRange_.minVal << "~"
                         << supportedBitrateRange_.maxVal << "]";
    OH_AVRange qualityRange;
    OH_AVCapability_GetEncoderQualityRange(capability, &qualityRange);
    RTC_DLOG(LS_VERBOSE) << "qualityRange=[" << qualityRange.minVal << "~" << qualityRange.maxVal << "]";

    encoder_ = ohos::VideoEncoder::CreateByName(codecName_.c_str());

    OH_AVCodecCallback callback;
    callback.onError = &HardwareVideoEncoder::OnCodecError1;
    callback.onStreamChanged = &HardwareVideoEncoder::OnStreamChanged1;
    callback.onNeedInputBuffer = &HardwareVideoEncoder::OnNeedInputBuffer1;
    callback.onNewOutputBuffer = &HardwareVideoEncoder::OnNewOutputBuffer1;
    int32_t ret = OH_VideoEncoder_RegisterCallback(encoder_.Raw(), callback, this);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to register callback: " << ret;
        return ret;
    }

    // 写入format
    auto format = ohos::AVFormat::Create();
    OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_WIDTH, codec_settings->width);
    OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_HEIGHT, codec_settings->height);
    OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_PIXEL_FORMAT, pixelFormat_);
    OH_AVFormat_SetDoubleValue(format.Raw(), OH_MD_KEY_FRAME_RATE, targetFramerate_);
    OH_AVFormat_SetLongValue(format.Raw(), OH_MD_KEY_BITRATE, targetBitrate_);
    OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, OH_VideoEncodeBitrateMode::CBR);

    if (codecSettings_.codecType == kVideoCodecH264) {
        OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_I_FRAME_INTERVAL, codecSettings_.H264()->keyFrameInterval);
        auto profileLevelId = ParseSdpForH264ProfileLevelId(format_.parameters);
        if (profileLevelId && (profileLevelId->profile == H264Profile::kProfileConstrainedHigh &&
                               profileLevelId->level == H264Level::kLevel3_1))
        {
            OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_PROFILE, AVC_PROFILE_HIGH);
            // OH_MD_KEY_LEVEL ?
        }
    }

    ret = OH_VideoEncoder_Configure(encoder_.Raw(), format.Raw());
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to configure: " << ret;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    if (sharedContext_) {
        OHNativeWindow* nativeWindow = nullptr;
        ret = OH_VideoEncoder_GetSurface(encoder_.Raw(), &nativeWindow);
        if (ret != AV_ERR_OK) {
            RTC_LOG(LS_ERROR) << "Failed to get surface: " << ret;
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        ret = OH_NativeWindow_NativeWindowHandleOpt(
            nativeWindow, SET_BUFFER_GEOMETRY, codecSettings_.width, codecSettings_.height);
        if (ret != 0) {
            RTC_LOG(LS_ERROR) << "Failed to set buffer geometry: " << ret;
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        nativeWindow_ = ohos::NativeWindow::TakeOwnership(nativeWindow);
        eglEnv_ = EglEnv::Create(sharedContext_, EglConfigAttributes::RGBA);
        eglEnv_->CreateWindowSurface(nativeWindow_);
        eglEnv_->MakeCurrent();
    }

    ret = OH_VideoEncoder_Prepare(encoder_.Raw());
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to prepare: " << ret;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    ret = OH_VideoEncoder_Start(encoder_.Raw());
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to start: " << ret;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    initialized_ = true;
    RTC_DLOG(LS_VERBOSE) << "Initialized";

    UpdateEncoderInfo();

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t HardwareVideoEncoder::RegisterEncodeCompleteCallback(EncodedImageCallback* callback)
{
    callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t HardwareVideoEncoder::Encode(const VideoFrame& frame, const std::vector<VideoFrameType>* frame_types)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " frame size=" << frame.width() << "x" << frame.height()
                         << ", pts=" << frame.timestamp_us() << ", rotation=" << frame.rotation();

    if (!initialized_) {
        RTC_LOG(LS_ERROR) << "Not initialized";
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    bool requestedKeyFrame = false;
    if (frame_types) {
        for (auto frameType : *frame_types) {
            if (frameType == VideoFrameType::kVideoFrameKey) {
                requestedKeyFrame = true;
            }
        }
    }

    if (requestedKeyFrame) {
        RTC_DLOG(LS_VERBOSE) << "Request key frame";
        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, true);
        int32_t ret = OH_VideoEncoder_SetParameter(encoder_.Raw(), format);
        if (ret != AV_ERR_OK) {
            RTC_LOG(LS_ERROR) << "Failed to set parameter OH_MD_KEY_REQUEST_I_FRAME: " << ret;
        }
    }

    FrameExtraInfo info;
    info.timestampUs = frame.timestamp_us();
    info.timestampRtp = frame.timestamp();
    info.rotation = frame.rotation();
    {
        UNUSED std::lock_guard<std::mutex> lock(extraInfosMutex_);
        extraInfos_.push_back(info);
    }

    if (sharedContext_ && eglEnv_) {
        return EncodeTextureBuffer(frame);
    }

    return EncodeByteBuffer(frame);
}

void HardwareVideoEncoder::SetRates(const RateControlParameters& parameters)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    RTC_DLOG(LS_VERBOSE) << "bitrate=" << parameters.bitrate.get_sum_bps()
                         << ", target_bitrate=" << parameters.target_bitrate.get_sum_bps();
    RTC_DLOG(LS_VERBOSE) << "framerate_fps=" << parameters.framerate_fps;

    targetBitrate_ = std::clamp(
        parameters.bitrate.get_sum_bps(), static_cast<uint32_t>(supportedBitrateRange_.minVal),
        static_cast<uint32_t>(supportedBitrateRange_.maxVal));

    if (parameters.framerate_fps <= 0) {
        targetFramerate_ = codecSettings_.maxFramerate;
    } else {
        targetFramerate_ = parameters.framerate_fps;
    }

    if (targetBitrate_ != curBitrate_) {
        RTC_LOG(LS_INFO) << "Update bitrate: " << targetBitrate_;
        auto format = ohos::AVFormat::Create();
        OH_AVFormat_SetLongValue(format.Raw(), OH_MD_KEY_BITRATE, targetBitrate_);
        int32_t ret = OH_VideoEncoder_SetParameter(encoder_.Raw(), format.Raw());
        if (ret == AV_ERR_OK) {
            curBitrate_ = targetBitrate_;
        } else {
            RTC_LOG(LS_ERROR) << "Failed to update bitrate" << ret;
        }
    }

    if (targetFramerate_ != curFramerate_) {
        RTC_LOG(LS_INFO) << "Update framerate: " << targetFramerate_;
        auto format = ohos::AVFormat::Create();
        OH_AVFormat_SetDoubleValue(format.Raw(), OH_MD_KEY_FRAME_RATE, targetFramerate_);
        int32_t ret = OH_VideoEncoder_SetParameter(encoder_.Raw(), format.Raw());
        if (ret == AV_ERR_OK) {
            curFramerate_ = targetFramerate_;
        } else {
            RTC_LOG(LS_ERROR) << "Failed to update framerate" << ret;
        }
    }
}

int32_t HardwareVideoEncoder::Release()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (initialized_) {
        int32_t ret = OH_VideoEncoder_Stop(encoder_.Raw());
        if (ret != AV_ERR_OK) {
            RTC_LOG(LS_ERROR) << "Failed to stop" << ret;
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        UNUSED std::unique_lock<std::mutex> lock(inputMutex_);
        std::queue<ohos::CodecBuffer> temp;
        std::swap(temp, inputBufferQueue_);
    }

    {
        UNUSED std::lock_guard<std::mutex> lock(extraInfosMutex_);
        extraInfos_.clear();
    }

    initialized_ = false;

    return WEBRTC_VIDEO_CODEC_OK;
}

VideoEncoder::EncoderInfo HardwareVideoEncoder::GetEncoderInfo() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return encoderInfo_;
}

void HardwareVideoEncoder::OnCodecError1(OH_AVCodec* codec, int32_t errorCode, void* userData)
{
    if (!userData) {
        return;
    }

    auto self = (HardwareVideoEncoder*)userData;
    self->OnCodecError(codec, errorCode);
}

void HardwareVideoEncoder::OnStreamChanged1(OH_AVCodec* codec, OH_AVFormat* format, void* userData)
{
    if (!userData) {
        return;
    }

    auto self = (HardwareVideoEncoder*)userData;
    self->OnStreamChanged(codec, format);
}

void HardwareVideoEncoder::OnNeedInputBuffer1(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer, void* userData)
{
    if (!userData) {
        return;
    }

    auto self = (HardwareVideoEncoder*)userData;
    self->OnNeedInputBuffer(codec, index, buffer);
}

void HardwareVideoEncoder::OnNewOutputBuffer1(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer, void* userData)
{
    if (!userData) {
        return;
    }

    auto self = (HardwareVideoEncoder*)userData;
    self->OnNewOutputBuffer(codec, index, buffer);
}

void HardwareVideoEncoder::OnCodecError(OH_AVCodec* codec, int32_t errorCode)
{
    (void)codec;
    (void)errorCode;
    RTC_LOG(LS_ERROR) << __FUNCTION__;
}

void HardwareVideoEncoder::OnStreamChanged(OH_AVCodec* codec, OH_AVFormat* format)
{
    (void)codec;
    (void)format;
    RTC_LOG(LS_INFO) << __FUNCTION__;
}

void HardwareVideoEncoder::OnNeedInputBuffer(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " buffer index: " << index;
    (void)codec;

    QueueInputBuffer(ohos::CodecBuffer(index, buffer));
}

void HardwareVideoEncoder::OnNewOutputBuffer(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " buffer index: " << index;

    OH_AVCodecBufferAttr attr;
    int32_t ret = OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer attr: " << ret;
        OH_VideoEncoder_FreeOutputBuffer(codec, index);
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "Output buffer attr: flags=" << attr.flags << ", pts=" << attr.pts << ", size=" << attr.size
                         << ", offset=" << attr.offset;

    uint8_t* addr = OH_AVBuffer_GetAddr(buffer);
    if (!addr) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer addr";
        OH_VideoEncoder_FreeOutputBuffer(codec, index);
        return;
    }

    if ((attr.flags & AVCODEC_BUFFER_FLAGS_CODEC_DATA) != 0) {
        configData_ = EncodedImageBuffer::Create(addr, attr.size);
        OH_VideoEncoder_FreeOutputBuffer(codec, index);
        return;
    }

    if ((attr.flags & AVCODEC_BUFFER_FLAGS_INCOMPLETE_FRAME) != 0) {
        RTC_DLOG(LS_VERBOSE) << "Incomplete frame";
    }

    bool isKeyFrame = (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) != 0;
    if (isKeyFrame) {
        RTC_DLOG(LS_VERBOSE) << "Sync frame generated";
    }

    if (!initialized_) {
        RTC_LOG(LS_ERROR) << "Not initialized";
        OH_VideoEncoder_FreeOutputBuffer(codec, index);
        return;
    }

    rtc::scoped_refptr<EncodedImageBuffer> encodedData;
    if (isKeyFrame && configData_) {
        encodedData = EncodedImageBuffer::Create(attr.size + configData_->size());
        std::memcpy(encodedData->data(), configData_->data(), configData_->size());
        std::memcpy(encodedData->data() + configData_->size(), addr + attr.offset, attr.size);
    } else {
        encodedData = EncodedImageBuffer::Create(addr + attr.offset, attr.size);
    }

    ret = OH_VideoEncoder_FreeOutputBuffer(codec, index);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to free output buffer";
    }

    const int64_t timestampUs = attr.pts;
    FrameExtraInfo extraInfo;
    {
        UNUSED std::lock_guard<std::mutex> lock(extraInfosMutex_);

        do {
            if (extraInfos_.empty()) {
                RTC_LOG(LS_WARNING) << "Unexpected frame with timestamp: " << timestampUs;
                return;
            }

            extraInfo = extraInfos_.front();
            extraInfos_.pop_front();
        } while (extraInfo.timestampUs != timestampUs);
    }

    EncodedImage encodedImage;
    encodedImage._encodedWidth = codecSettings_.width;
    encodedImage._encodedHeight = codecSettings_.height;
    encodedImage.capture_time_ms_ = timestampUs / rtc::kNumMicrosecsPerMillisec;
    encodedImage.rotation_ = extraInfo.rotation;
    encodedImage.SetRtpTimestamp(extraInfo.timestampRtp);
    encodedImage.SetEncodedData(encodedData);
    encodedImage.set_size(encodedData->size());

    CodecSpecificInfo info;
    info.codecType = codecSettings_.codecType;

    callback_->OnEncodedImage(encodedImage, &info);
}

void HardwareVideoEncoder::UpdateEncoderInfo()
{
    encoderInfo_.implementation_name = codecName_;
    encoderInfo_.supports_native_handle = true;
    encoderInfo_.is_hardware_accelerated = true;
    encoderInfo_.supports_simulcast = true;
    encoderInfo_.scaling_settings = GetScalingSettings();

    encoderInfo_.requested_resolution_alignment = kRequestedResolutionAlignment;
    encoderInfo_.apply_alignment_to_all_simulcast_layers = false;
}

VideoEncoder::ScalingSettings HardwareVideoEncoder::GetScalingSettings()
{
    if (codecSettings_.codecType == VideoCodecType::kVideoCodecH264) {
        return VideoEncoder::ScalingSettings(kH264QpThresholdLow, kH264QpThresholdHigh);
    }

    return VideoEncoder::ScalingSettings::kOff;
}

bool HardwareVideoEncoder::DequeueInputBuffer(ohos::CodecBuffer& buffer)
{
    std::unique_lock<std::mutex> lock(inputMutex_);
    inputCond_.wait_for(
        lock, std::chrono::milliseconds(10), [this]() { return !inputBufferQueue_.empty() || !initialized_; });
    if (!initialized_) {
        return false;
    }
    if (this->inputBufferQueue_.empty()) {
        return false;
    }
    buffer = inputBufferQueue_.front();
    inputBufferQueue_.pop();

    return true;
}

void HardwareVideoEncoder::QueueInputBuffer(const ohos::CodecBuffer& buffer)
{
    UNUSED std::unique_lock<std::mutex> lock(inputMutex_);
    inputBufferQueue_.push(buffer);
    inputCond_.notify_all();
}

int32_t HardwareVideoEncoder::EncodeTextureBuffer(const VideoFrame& frame)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " enter";

    int ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_.Raw(), SET_UI_TIMESTAMP, frame.timestamp_us());
    if (ret != 0) {
        RTC_LOG(LS_ERROR) << "Failed to set ui timestamp: " << ret;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    VideoFrame derotatedFrame = VideoFrame::Builder()
                                    .set_video_frame_buffer(frame.video_frame_buffer())
                                    .set_rotation(kVideoRotation_0)
                                    .set_timestamp_us(frame.timestamp_us())
                                    .build();
    videoFrameDrawer_->DrawFrame(derotatedFrame, *textureDrawer_, Matrix());

    if (!eglEnv_->SwapBuffers(frame.timestamp_us() * rtc::kNumNanosecsPerMicrosec)) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " exit";
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t HardwareVideoEncoder::EncodeByteBuffer(const VideoFrame& frame)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (frame.is_texture()) {
        RTC_LOG(LS_ERROR) << "Texture buffer is not supported in buffer mode yet";
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    ohos::CodecBuffer codecBuffer;
    if (!DequeueInputBuffer(codecBuffer)) {
        RTC_LOG(LS_ERROR) << "Failed to get cached input buffer";
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    OH_AVCodecBufferAttr attr;
    int32_t ret = OH_AVBuffer_GetBufferAttr(codecBuffer.buf, &attr);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer attr: " << ret;
        QueueInputBuffer(codecBuffer);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    RTC_DLOG(LS_VERBOSE) << "Input buffer attr: flags=" << attr.flags << ", pts=" << attr.pts << ", size=" << attr.size
                         << ", offset=" << attr.offset;

    uint8_t* addr = OH_AVBuffer_GetAddr(codecBuffer.buf);
    if (!addr) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer addr";
        QueueInputBuffer(codecBuffer);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    auto srcBuffer = frame.video_frame_buffer()->ToI420();
    int width = frame.width();
    int height = frame.height();

    switch (pixelFormat_) {
        case AV_PIXEL_FORMAT_YUVI420: {
            // copy
            memcpy(addr, srcBuffer->DataY(), width * height);                              // y planar
            memcpy(addr + width * height, srcBuffer->DataU(), width * height / 4);         // u planar
            memcpy(addr + width * height * 5 / 4, srcBuffer->DataV(), width * height / 4); // v planar
        } break;
        case AV_PIXEL_FORMAT_NV12: {
            ret = libyuv::I420ToNV12(
                srcBuffer->DataY(), srcBuffer->StrideY(), srcBuffer->DataU(), srcBuffer->StrideU(), srcBuffer->DataV(),
                srcBuffer->StrideV(), addr, width, addr + width * height, width, width, height);
            RTC_DLOG(LS_VERBOSE) << "I420ToNV12 ret = " << ret;
        } break;
        case AV_PIXEL_FORMAT_NV21: {
            ret = libyuv::I420ToNV21(
                srcBuffer->DataY(), srcBuffer->StrideY(), srcBuffer->DataU(), srcBuffer->StrideU(), srcBuffer->DataV(),
                srcBuffer->StrideV(), addr, width, addr + width * height, width, width, height);
            RTC_DLOG(LS_VERBOSE) << "I420ToNV12 ret = " << ret;
        } break;
        case AV_PIXEL_FORMAT_RGBA: {
            ret = libyuv::I420ToABGR(
                srcBuffer->DataY(), srcBuffer->StrideY(), srcBuffer->DataU(), srcBuffer->StrideU(), srcBuffer->DataV(),
                srcBuffer->StrideV(), addr, width * 4, width, height);
            RTC_DLOG(LS_VERBOSE) << "I420ToABGR ret = " << ret;
        } break;
        default: {
            RTC_LOG(LS_ERROR) << "Unsupported pixel format: " << pixelFormat_;
            QueueInputBuffer(codecBuffer);
        }
            return WEBRTC_VIDEO_CODEC_ERROR;
    }

    attr.pts = frame.timestamp_us();
    attr.size = frame.size();
    ret = OH_AVBuffer_SetBufferAttr(codecBuffer.buf, &attr);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer attr: " << ret;
        QueueInputBuffer(codecBuffer);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    ret = OH_VideoEncoder_PushInputBuffer(encoder_.Raw(), codecBuffer.index);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to get push input buffer: " << ret;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

} // namespace adapter
} // namespace webrtc
