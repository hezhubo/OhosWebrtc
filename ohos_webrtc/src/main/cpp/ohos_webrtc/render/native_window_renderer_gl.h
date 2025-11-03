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

#ifndef WEBRTC_RENDER_NATIVE_WINDOW_RENDERER_GL_H
#define WEBRTC_RENDER_NATIVE_WINDOW_RENDERER_GL_H

#include "native_window_renderer.h"
#include "egl_env.h"
#include "render/gl_shader.h"
#include "video_frame_drawer.h"
#include "../video/texture_buffer.h"
#include "../helper/native_window.h"

#include <GLES3/gl3.h>
#include <memory>

#include "rtc_base/thread.h"

namespace webrtc {
namespace adapter {

class NativeWindowRendererGl : public NativeWindowRenderer {
public:
    static std::unique_ptr<NativeWindowRendererGl>
    Create(ohos::NativeWindow window, std::shared_ptr<EglContext> sharedContext);
    static std::unique_ptr<NativeWindowRendererGl>
    Create(ohos::NativeWindow window, std::shared_ptr<EglContext> sharedContext, const std::string& threadName);

    ~NativeWindowRendererGl() override;

    void SetMirrorHorizontally(bool mirror) override;
    void SetMirrorVertically(bool mirror) override;
    void SetScalingMode(ScalingMode scaleMode) override;

protected:
    NativeWindowRendererGl(
        ohos::NativeWindow window, std::shared_ptr<EglContext> sharedContext, const std::string& threadName);

    void OnFrame(const VideoFrame& frame) override;
    void OnDiscardedFrame() override;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) override;

    void RenderFrame(const VideoFrame& frame);

private:
    int32_t width_{};
    int32_t height_{};
    int32_t format_{};
    int32_t transform_{};
    uint64_t usage_{};

    std::unique_ptr<rtc::Thread> thread_;

    std::unique_ptr<EglEnv> eglEnv_;

    Matrix drawMatrix_;
    std::unique_ptr<GlDrawer> textureDrawer_;
    std::unique_ptr<VideoFrameDrawer> videoFrameDrawer_;

    bool mirrorHorizontally_{false};
    bool mirrorVertically_{false};
    ScalingMode scaleMode_{ScalingMode::FILL};
};

} // namespace adapter
} // namespace webrtc

#endif // WEBRTC_RENDER_NATIVE_WINDOW_RENDERER_GL_H
