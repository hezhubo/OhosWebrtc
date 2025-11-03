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

#include "texture_buffer.h"
#include "../render/yuv_converter.h"

#include "api/make_ref_counted.h"
#include "rtc_base/logging.h"

namespace webrtc {

rtc::scoped_refptr<TextureBuffer>
TextureBuffer::Create(std::weak_ptr<TextureData> texture, int width, int height, const Matrix& transformMatrix)
{
    return rtc::make_ref_counted<TextureBuffer>(texture, width, height, transformMatrix);
}

TextureBuffer::TextureBuffer(std::weak_ptr<TextureData> texture, int width, int height, const Matrix& transformMatrix)
    : texture_(texture), width_(width), height_(height), transformMatrix_(transformMatrix)
{
    RTC_DLOG(LS_VERBOSE) << "TextureBuffer ctor";
}

TextureBuffer::~TextureBuffer()
{
    RTC_DLOG(LS_VERBOSE) << "TextureBuffer dtor";
}

rtc::scoped_refptr<I420BufferInterface> TextureBuffer::ToI420()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    auto texture = texture_.lock();

    rtc::Thread* toI420Handler = texture->getToI420Handler();
    YuvConverter* yuvConverter = texture->getYuvConverter();
    if (!toI420Handler || !yuvConverter) {
        return nullptr;
    }

    auto textureBuffer = rtc::scoped_refptr<TextureBuffer>(this);
    return toI420Handler->BlockingCall([yuvConverter, textureBuffer] { return yuvConverter->Convert(textureBuffer); });
}

rtc::scoped_refptr<VideoFrameBuffer> TextureBuffer::CropAndScale(
    int offset_x, int offset_y, int crop_width, int crop_height, int scaled_width, int scaled_height)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    // crop and scale by transformMatrix_
    // for now, just return self without crop and scale.
    return rtc::scoped_refptr<VideoFrameBuffer>(this);
}

} // namespace webrtc
