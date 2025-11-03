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

#include "hardware_video_decoder.h"
#include "../video/video_frame_receiver_gl.h"
#include "../utils/marcos.h"

#include <native_buffer/native_buffer.h>
#include <multimedia/player_framework/native_averrors.h>

#include "api/video/i420_buffer.h"
#include "api/video/nv12_buffer.h"
#include "modules/video_coding/include/video_error_codes.h"
#include "rtc_base/time_utils.h"
#include "libyuv/convert.h"

namespace webrtc {
namespace adapter {

// RTP timestamps are 90 kHz.
const int64_t kNumRtpTicksPerMillisec = 90000 / rtc::kNumMillisecsPerSec;

std::unique_ptr<HardwareVideoDecoder> HardwareVideoDecoder::Create(
    const std::string& codecName, int32_t colorFormat, const SdpVideoFormat& format,
    std::shared_ptr<EglContext> sharedContext)
{
    return std::unique_ptr<HardwareVideoDecoder>(
        new HardwareVideoDecoder(codecName, colorFormat, format, sharedContext));
}

HardwareVideoDecoder::HardwareVideoDecoder(
    const std::string& codecName, int32_t colorFormat, const SdpVideoFormat& format,
    std::shared_ptr<EglContext> sharedContext)
    : codecName_(codecName), format_(format), colorFormat_(colorFormat), sharedContext_(sharedContext)
{
}

HardwareVideoDecoder::~HardwareVideoDecoder() = default;

bool HardwareVideoDecoder::Configure(const Settings& settings)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    decoderSettings_ = settings;
    return InitDecode();
}

int32_t HardwareVideoDecoder::Release()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (initialized_) {
        int32_t ret = OH_VideoDecoder_Stop(decoder_.Raw());
        if (ret != AV_ERR_OK) {
            RTC_LOG(LS_ERROR) << "Failed to stop" << ret;
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        if (videoFrameReceiver_) {
            videoFrameReceiver_.reset();
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

int32_t HardwareVideoDecoder::RegisterDecodeCompleteCallback(DecodedImageCallback* callback)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    RTC_DCHECK_RUNS_SERIALIZED(&callbackRaceChecker_);
    callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

// `missing_frames`, `fragmentation` and `render_time_ms` are ignored.
int32_t HardwareVideoDecoder::Decode(const EncodedImage& input_image, bool /*missing_frames*/, int64_t render_time_ms)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!initialized_) {
        // Most likely initializing the codec failed.
        RTC_LOG(LS_ERROR) << "Not initialized";
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (input_image.data() == nullptr || input_image.size() == 0) {
        RTC_LOG(LS_ERROR) << "input image is empty";
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    RTC_DLOG(LS_VERBOSE) << "input image, size=" << input_image.size()
                         << " capture time=" << input_image.capture_time_ms_ << " rotation=" << input_image.rotation()
                         << " rtp timestamp=" << input_image.RtpTimestamp();

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

    uint8_t* addr = OH_AVBuffer_GetAddr(codecBuffer.buf);
    if (!addr) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer addr";
        QueueInputBuffer(codecBuffer);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // copy data
    memcpy(addr + attr.offset, input_image.data(), input_image.size());

    attr.pts = input_image.RtpTimestamp() / kNumRtpTicksPerMillisec * rtc::kNumMicrosecsPerMillisec;
    attr.size = input_image.size();
    if (input_image.FrameType() == VideoFrameType::kVideoFrameKey) {
        RTC_DLOG(LS_VERBOSE) << "Key frame occurred";
        attr.flags |= AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    }

    ret = OH_AVBuffer_SetBufferAttr(codecBuffer.buf, &attr);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer attr: " << ret;
        QueueInputBuffer(codecBuffer);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    {
        FrameExtraInfo info;
        info.timestampUs = input_image.RtpTimestamp() / kNumRtpTicksPerMillisec * rtc::kNumMicrosecsPerMillisec;
        info.timestampRtp = input_image.RtpTimestamp();
        info.timestampNtp = input_image.NtpTimeMs();

        UNUSED std::lock_guard<std::mutex> lock(extraInfosMutex_);
        extraInfos_.push_back(info);
    }

    RTC_DLOG(LS_VERBOSE) << "push input buffer, pts=" << attr.pts;
    ret = OH_VideoDecoder_PushInputBuffer(decoder_.Raw(), codecBuffer.index);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to push input buffer: " << ret;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

VideoDecoder::DecoderInfo HardwareVideoDecoder::GetDecoderInfo() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    VideoDecoder::DecoderInfo decoder_info;
    decoder_info.implementation_name = codecName_;
    decoder_info.is_hardware_accelerated = true;
    return decoder_info;
}

const char* HardwareVideoDecoder::ImplementationName() const
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    return codecName_.c_str();
}

void HardwareVideoDecoder::OnFrameAvailable(
    rtc::scoped_refptr<VideoFrameBuffer> buffer, int64_t timestampUs, VideoRotation rotation)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    RTC_DLOG(LS_VERBOSE) << "rotation = " << rotation;

    FrameExtraInfo extraInfo;
    {
        UNUSED std::lock_guard<std::mutex> lock(extraInfosMutex_);

        do {
            if (extraInfos_.empty()) {
                RTC_LOG(LS_WARNING) << "unexpected frame: " << timestampUs;
                return;
            }

            extraInfo = extraInfos_.front();
            extraInfos_.pop_front();
            // If the decoder might drop frames so iterate through the queue until we
            // find a matching timestamp.
        } while (extraInfo.timestampUs != timestampUs);
    }

    auto frame = VideoFrame::Builder()
                     .set_id(65535)
                     .set_video_frame_buffer(buffer)
                     .set_rotation(kVideoRotation_0)
                     .set_timestamp_us(timestampUs)
                     .set_timestamp_rtp(extraInfo.timestampRtp)
                     .set_ntp_time_ms(extraInfo.timestampNtp)
                     .build();

    RTC_DCHECK_RUNS_SERIALIZED(&callbackRaceChecker_);
    callback_->Decoded(frame, std::nullopt, std::nullopt);
}

void HardwareVideoDecoder::OnCodecError1(OH_AVCodec* codec, int32_t errorCode, void* userData)
{
    if (!userData) {
        return;
    }

    auto self = (HardwareVideoDecoder*)userData;
    self->OnCodecError(codec, errorCode);
}

void HardwareVideoDecoder::OnStreamChanged1(OH_AVCodec* codec, OH_AVFormat* format, void* userData)
{
    if (!userData) {
        return;
    }

    auto self = (HardwareVideoDecoder*)userData;
    self->OnStreamChanged(codec, format);
}

void HardwareVideoDecoder::OnNeedInputBuffer1(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer, void* userData)
{
    if (!userData) {
        return;
    }

    auto self = (HardwareVideoDecoder*)userData;
    self->OnNeedInputBuffer(codec, index, buffer);
}

void HardwareVideoDecoder::OnNewOutputBuffer1(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer, void* userData)
{
    if (!userData) {
        return;
    }

    auto self = (HardwareVideoDecoder*)userData;
    self->OnNewOutputBuffer(codec, index, buffer);
}

void HardwareVideoDecoder::OnCodecError(OH_AVCodec* codec, int32_t errorCode)
{
    RTC_LOG(LS_ERROR) << __FUNCTION__;
    (void)codec;
    (void)errorCode;
}

void HardwareVideoDecoder::OnStreamChanged(OH_AVCodec* codec, OH_AVFormat* format)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    (void)codec;
    (void)format;
}

void HardwareVideoDecoder::OnNeedInputBuffer(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " buffer index: " << index;
    (void)codec;

    QueueInputBuffer(ohos::CodecBuffer(index, buffer));
}

void HardwareVideoDecoder::OnNewOutputBuffer(OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " buffer index: " << index;
    (void)codec;

    if (videoFrameReceiver_) {
        DeliverTextureFrame(ohos::CodecBuffer(index, buffer));
    } else {
        DeliverByteFrame(ohos::CodecBuffer(index, buffer));
    }
}

bool HardwareVideoDecoder::InitDecode()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    decoder_ = ohos::VideoDecoder::CreateByName(codecName_.c_str());

    OH_AVCodecCallback callback;
    callback.onError = &HardwareVideoDecoder::OnCodecError1;
    callback.onStreamChanged = &HardwareVideoDecoder::OnStreamChanged1;
    callback.onNeedInputBuffer = &HardwareVideoDecoder::OnNeedInputBuffer1;
    callback.onNewOutputBuffer = &HardwareVideoDecoder::OnNewOutputBuffer1;
    int32_t ret = OH_VideoDecoder_RegisterCallback(decoder_.Raw(), callback, this);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to register callback: " << ret;
        return ret;
    }

    RenderResolution resolution = decoderSettings_.max_render_resolution();

    auto format = ohos::AVFormat::Create();
    OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_WIDTH, resolution.Width());
    OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_HEIGHT, resolution.Height());
    OH_AVFormat_SetIntValue(format.Raw(), OH_MD_KEY_PIXEL_FORMAT, colorFormat_);

    ret = OH_VideoDecoder_Configure(decoder_.Raw(), format.Raw());
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to configure: " << ret;
        return false;
    }

    if (sharedContext_) {
        videoFrameReceiver_ = VideoFrameReceiverGl::Create("decoder-texture-thread", sharedContext_);
        videoFrameReceiver_->SetVideoFrameSize(resolution.Width(), resolution.Height());
        videoFrameReceiver_->SetCallback(this);
        nativeWindow_ = ohos::NativeWindow::CreateFromSurfaceId(videoFrameReceiver_->GetSurfaceId());
        ret = OH_VideoDecoder_SetSurface(decoder_.Raw(), nativeWindow_.Raw());
        if (ret != AV_ERR_OK) {
            RTC_LOG(LS_ERROR) << "Failed to set surface: " << ret;
            return false;
        }
    }

    ret = OH_VideoDecoder_Prepare(decoder_.Raw());
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to prepare: " << ret;
        return false;
    }

    ret = OH_VideoDecoder_Start(decoder_.Raw());
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to start: " << ret;
        return false;
    }

    initialized_ = true;

    return true;
}

void HardwareVideoDecoder::QueueInputBuffer(const ohos::CodecBuffer& buffer)
{
    UNUSED std::unique_lock<std::mutex> lock(inputMutex_);
    inputBufferQueue_.push(buffer);
    inputCond_.notify_all();
}

bool HardwareVideoDecoder::DequeueInputBuffer(ohos::CodecBuffer& buffer)
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

void HardwareVideoDecoder::DeliverTextureFrame(const ohos::CodecBuffer& buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    OH_AVCodecBufferAttr attr;
    int32_t ret = OH_AVBuffer_GetBufferAttr(buffer.buf, &attr);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer attr: " << ret;
        ret = OH_VideoDecoder_FreeOutputBuffer(decoder_.Raw(), buffer.index);
        if (ret != AV_ERR_OK) {
            RTC_LOG(LS_ERROR) << "Failed to free output buffer";
        }
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "buffer attr: offset=" << attr.offset << ", size=" << attr.size << ", flags=" << attr.flags
                         << ", pts=" << attr.pts;

    RTC_DLOG(LS_VERBOSE) << "render output buffer, index=" << buffer.index;
    ret = OH_VideoDecoder_RenderOutputBuffer(decoder_.Raw(), buffer.index);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to render output buffer" << ret;
    }
}

void HardwareVideoDecoder::DeliverByteFrame(const ohos::CodecBuffer& buffer)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    OH_AVCodecBufferAttr attr;
    int32_t ret = OH_AVBuffer_GetBufferAttr(buffer.buf, &attr);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer attr: " << ret;
        ret = OH_VideoDecoder_FreeOutputBuffer(decoder_.Raw(), buffer.index);
        if (ret != AV_ERR_OK) {
            RTC_LOG(LS_ERROR) << "Failed to free output buffer";
        }
        return;
    }
    RTC_DLOG(LS_VERBOSE) << "buffer attr: offset=" << attr.offset << ", size=" << attr.size << ", flags=" << attr.flags
                         << ", pts=" << attr.pts;

    OH_NativeBuffer* nativeBuffer = OH_AVBuffer_GetNativeBuffer(buffer.buf);
    if (nativeBuffer == nullptr) {
        RTC_LOG(LS_ERROR) << "Failed to get native buffer";
        return;
    }

    OH_NativeBuffer_Config config;
    OH_NativeBuffer_GetConfig(nativeBuffer, &config);
    RTC_DLOG(LS_VERBOSE) << "buffer config: format=" << config.format << ", width=" << config.width
                         << ", height=" << config.height << ", stride=" << config.stride;

    const int64_t timestampUs = attr.pts;

    FrameExtraInfo extraInfo;
    {
        UNUSED std::lock_guard<std::mutex> lock(extraInfosMutex_);

        do {
            if (extraInfos_.empty()) {
                RTC_LOG(LS_WARNING) << "unexpected frame: " << timestampUs;
                ret = OH_VideoDecoder_FreeOutputBuffer(decoder_.Raw(), buffer.index);
                if (ret != AV_ERR_OK) {
                    RTC_LOG(LS_ERROR) << "Failed to free output buffer";
                }
                return;
            }

            extraInfo = extraInfos_.front();
            extraInfos_.pop_front();
            // If the decoder might drop frames so iterate through the queue until we
            // find a matching timestamp.
        } while (extraInfo.timestampUs != timestampUs);
    }

    uint8_t* addr = OH_AVBuffer_GetAddr(buffer.buf);
    if (!addr) {
        RTC_LOG(LS_ERROR) << "Failed to get buffer addr";
        ret = OH_VideoDecoder_FreeOutputBuffer(decoder_.Raw(), buffer.index);
        if (ret != AV_ERR_OK) {
            RTC_LOG(LS_ERROR) << "Failed to free output buffer";
        }
        return;
    }

    RenderResolution resolution = decoderSettings_.max_render_resolution();
    int width = resolution.Width();
    int height = resolution.Height();

    auto i420Buffer = I420Buffer::Create(width, height);
    switch (colorFormat_) {
        case AV_PIXEL_FORMAT_YUVI420: {
            // copy data
            memcpy(i420Buffer->MutableDataY(), addr, width * height);                              // y planar
            memcpy(i420Buffer->MutableDataU(), addr + width * height, width * height / 4);         // u planar
            memcpy(i420Buffer->MutableDataV(), addr + width * height * 5 / 4, width * height / 4); // v planar
        } break;
        case AV_PIXEL_FORMAT_NV12: {
            int32_t ret = libyuv::NV12ToI420(
                (uint8_t*)addr, width, (uint8_t*)addr + width * height, width, i420Buffer->MutableDataY(),
                i420Buffer->StrideY(), i420Buffer->MutableDataU(), i420Buffer->StrideU(), i420Buffer->MutableDataV(),
                i420Buffer->StrideV(), width, height);
            RTC_DLOG(LS_VERBOSE) << "NV12ToI420 ret = " << ret;
        } break;
        case AV_PIXEL_FORMAT_NV21: {
            int32_t ret = libyuv::NV21ToI420(
                (uint8_t*)addr, width, (uint8_t*)addr + width * height, width, i420Buffer->MutableDataY(),
                i420Buffer->StrideY(), i420Buffer->MutableDataU(), i420Buffer->StrideU(), i420Buffer->MutableDataV(),
                i420Buffer->StrideV(), width, height);
            RTC_DLOG(LS_VERBOSE) << "NV21ToI420 ret = " << ret;
        } break;
        case AV_PIXEL_FORMAT_RGBA: {
            int32_t ret = libyuv::ABGRToI420(
                (uint8_t*)addr, width * 4, i420Buffer->MutableDataY(), i420Buffer->StrideY(),
                i420Buffer->MutableDataU(), i420Buffer->StrideU(), i420Buffer->MutableDataV(), i420Buffer->StrideV(),
                width, height);
            RTC_DLOG(LS_VERBOSE) << "ABGRToI420 ret = " << ret;
        } break;
        default: {
            RTC_LOG(LS_ERROR) << "Unsupported color format: " << colorFormat_;
            ret = OH_VideoDecoder_FreeOutputBuffer(decoder_.Raw(), buffer.index);
            if (ret != AV_ERR_OK) {
                RTC_LOG(LS_ERROR) << "Failed to free output buffer";
            }
            return;
        }
    }

    ret = OH_VideoDecoder_FreeOutputBuffer(decoder_.Raw(), buffer.index);
    if (ret != AV_ERR_OK) {
        RTC_LOG(LS_ERROR) << "Failed to free output buffer";
    }

    auto frame = VideoFrame::Builder()
                     .set_id(65535)
                     .set_video_frame_buffer(i420Buffer)
                     .set_rotation(kVideoRotation_0)
                     .set_timestamp_us(attr.pts)
                     .set_timestamp_rtp(extraInfo.timestampRtp)
                     .set_ntp_time_ms(extraInfo.timestampNtp)
                     .build();

    RTC_DCHECK_RUNS_SERIALIZED(&callbackRaceChecker_);
    callback_->Decoded(frame, std::nullopt, std::nullopt);
}

} // namespace adapter
} // namespace webrtc
