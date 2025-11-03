/*
# Copyright (c) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/
#ifndef EGL_RENDER_CONTEXT_H
#define EGL_RENDER_CONTEXT_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>

#ifdef LOG_DOMAIN
#undef LOG_DOMAIN
#endif
#define LOG_DOMAIN 0x3200

namespace webrtc {
namespace ohos {

// 获取EGL错误信息
const char *GetEglErrorString();

class EglRenderContext {
public:
    EglRenderContext();
    ~EglRenderContext() noexcept;

    // disallow copy and move
    EglRenderContext(const EglRenderContext &) = delete;
    void operator=(const EglRenderContext &) = delete;
    EglRenderContext(const EglRenderContext &&) = delete;
    void operator=(const EglRenderContext &&) = delete;

    bool Init(EGLContext eglContext = EGL_NO_CONTEXT);
    bool IsEglContextReady() const { return eglContext_ != EGL_NO_CONTEXT; }

    EGLDisplay GetEGLDisplay() const { return eglDisplay_; }
    EGLContext GetEGLContext() const { return eglContext_; }

    EGLSurface CreateEglSurface(EGLNativeWindowType surface, const EGLint *attribList = nullptr);
    EGLSurface CreatePBufferEglSurface(const EGLint *attribList = nullptr);
    void DestroyEglSurface(EGLSurface surface);

    void MakeCurrent(EGLSurface surface = EGL_NO_SURFACE) const;
    void SwapBuffers(EGLSurface surface) const;

private:
    void SetupEglExtensions();

protected:
    EGLDisplay eglDisplay_{EGL_NO_DISPLAY};
    EGLContext eglContext_{EGL_NO_CONTEXT};
    EGLConfig config_{nullptr};

    bool hasEglSurfacelessContext_{false};
    bool hasEglBufferAge_{false};
    bool hasEglPartialUpdate_{false};
    PFNEGLSETDAMAGEREGIONKHRPROC eglSetDamageRegionFunc_{nullptr};
    PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC eglSwapBuffersWithDamageFunc_{nullptr};
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageFunc_{nullptr};
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC eglImageTargetTexture2DOESFunc_{nullptr};
};
} // namespace ohos
} // namespace webrtc
#endif // EGL_RENDER_CONTEXT_H
