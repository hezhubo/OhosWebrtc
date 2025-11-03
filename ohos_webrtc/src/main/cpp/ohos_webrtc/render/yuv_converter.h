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

#ifndef WEBRTC_RENDER_YUV_CONVERTER_H
#define WEBRTC_RENDER_YUV_CONVERTER_H

#include "video_frame_drawer.h"

#include "api/video/video_frame_buffer.h"

#include <memory>

namespace webrtc {

class GlConverterDrawer;

// see //sdk/android/api/org/webrtc/YuvConverter.java
class YuvConverter {
public:
    YuvConverter();
    ~YuvConverter();

    rtc::scoped_refptr<I420BufferInterface> Convert(rtc::scoped_refptr<TextureBuffer> textureBuffer);

protected:
    bool PrepareFrameBuffer(int width, int height);

private:
    int frameBufferWidth_{0};
    int frameBufferHeight_{0};
    unsigned int frameBufferId_{0};
    unsigned int textureId_{0};
    std::unique_ptr<GlConverterDrawer> drawer_;
    std::unique_ptr<VideoFrameDrawer> frameDrawer_;
};

} // namespace webrtc

#endif // WEBRTC_RENDER_YUV_CONVERTER_H
