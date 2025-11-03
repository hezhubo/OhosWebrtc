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

#ifndef WEBRTC_RENDER_NATIVE_WINDOW_RENDERER_RASTER_H
#define WEBRTC_RENDER_NATIVE_WINDOW_RENDERER_RASTER_H

#include "native_window_renderer.h"
#include "../helper/native_buffer.h"

#include <memory>

#include "rtc_base/thread.h"

namespace webrtc {
namespace adapter {

class NativeWindowRendererRaster : public NativeWindowRenderer {
public:
    static std::unique_ptr<NativeWindowRendererRaster> Create(ohos::NativeWindow window);

    // Do not use this constructor directly, use 'Create' above.
    explicit NativeWindowRendererRaster(ohos::NativeWindow window);
    ~NativeWindowRendererRaster() override;

protected:
    void OnFrame(const VideoFrame& frame) override;
    void OnDiscardedFrame() override;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) override;

    void RenderByteBuffer(rtc::scoped_refptr<I420BufferInterface> buffer);

private:
    int32_t width_{};
    int32_t height_{};
    int32_t format_{};
    int32_t transform_{};
    uint64_t usage_{};

    std::unique_ptr<rtc::Thread> thread_;
};

} // namespace adapter
} // namespace webrtc

#endif // WEBRTC_RENDER_NATIVE_WINDOW_RENDERER_RASTER_H
