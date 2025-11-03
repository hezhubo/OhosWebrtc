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

#include "egl_env.h"
#include "egl_config_attributes.h"

#include <EGL/eglext.h>

#include <rtc_base/logging.h>

namespace webrtc {

namespace {

constexpr char CHARACTER_WHITESPACE = ' ';
constexpr const char* CHARACTER_STRING_WHITESPACE = " ";

static const char* GetEglErrorString()
{
    EGLint error = eglGetError();
    switch (error) {
        case EGL_SUCCESS:
            return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
            return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
            return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
            return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
            return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:
            return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:
            return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:
            return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
            return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:
            return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:
            return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:
            return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:
            return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
            return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:
            return "EGL_CONTEXT_LOST";
        default:
            return "Unknow Error";
    }
}

static bool CheckEglExtension(const char* eglExtensions, const char* eglExtension)
{
    // Check egl extension
    size_t extLenth = strlen(eglExtension);
    const char* endPos = eglExtensions + strlen(eglExtensions);

    while (eglExtensions < endPos) {
        size_t len = 0;
        if (*eglExtensions == CHARACTER_WHITESPACE) {
            eglExtensions++;
            continue;
        }
        len = strcspn(eglExtensions, CHARACTER_STRING_WHITESPACE);
        if (len == extLenth && strncmp(eglExtension, eglExtensions, len) == 0) {
            return true;
        }
        eglExtensions += len;
    }

    return false;
}

} // namespace

class EglContextImpl : public EglContext {
public:
    explicit EglContextImpl(EGLContext context) : eglContext_(context) {}

    EGLContext GetRawContext() override
    {
        return eglContext_;
    }

private:
    EGLContext eglContext_{EGL_NO_CONTEXT};
};

EglEnv& EglEnv::GetDefault()
{
    static std::unique_ptr<EglEnv> _eglEnv = Create(EglConfigAttributes::DEFAULT);
    return *_eglEnv;
}

std::unique_ptr<EglEnv> EglEnv::Create()
{
    return EglEnv::Create(nullptr);
}

std::unique_ptr<EglEnv> EglEnv::Create(std::shared_ptr<EglContext> sharedContext)
{
    return EglEnv::Create(sharedContext, EglConfigAttributes::DEFAULT);
}

std::unique_ptr<EglEnv> EglEnv::Create(const std::vector<int32_t>& configAttributes)
{
    return EglEnv::Create(nullptr, configAttributes);
}

std::unique_ptr<EglEnv>
EglEnv::Create(std::shared_ptr<EglContext> sharedContext, const std::vector<int32_t>& configAttributes)
{
    auto eglEnv = std::make_unique<EglEnv>();
    if (!eglEnv->Init(sharedContext ? sharedContext->GetRawContext() : EGL_NO_CONTEXT, configAttributes)) {
        RTC_LOG(LS_ERROR) << "Failed to init egl context";
        return nullptr;
    }
    return eglEnv;
}

int32_t EglEnv::GetOpenGLESVersionFromConfig(const std::vector<int32_t>& configAttributes)
{
    for (std::size_t i = 0; i < configAttributes.size() - 1; ++i) {
        if (configAttributes[i] == EGL_RENDERABLE_TYPE) {
            switch (configAttributes[i + 1]) {
                case EGL_OPENGL_ES2_BIT:
                    return kOpenGLVersion_2;
                case EGL_OPENGL_ES3_BIT:
                    return kOpenGLVersion_3;
                default:
                    return kOpenGLVersion_1;
            }
        }
    }
    return kOpenGLVersion_1;
}

EglEnv::EglEnv() = default;

EglEnv::~EglEnv()
{
    Release();
}

std::shared_ptr<EglContext> EglEnv::GetContext() const
{
    return std::make_shared<EglContextImpl>(eglContext_);
}

bool EglEnv::CreatePbufferSurface(int width, int height)
{
    if (eglSurface_ != EGL_NO_SURFACE) {
        RTC_LOG(LS_ERROR) << "Already has an EGLSurface";
        return false;
    }

    EGLint attribs[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
    eglSurface_ = eglCreatePbufferSurface(eglDisplay_, eglConfig_, attribs);
    if (eglSurface_ == EGL_NO_SURFACE) {
        RTC_LOG(LS_ERROR) << "Failed to create pixel buffer surface with size " << width << "x" << height << ": "
                          << eglGetError();
        return false;
    }

    return true;
}

bool EglEnv::CreateWindowSurface(ohos::NativeWindow window)
{
    if (eglSurface_ != EGL_NO_SURFACE) {
        RTC_LOG(LS_ERROR) << "Already has an EGLSurface";
        return false;
    }

    eglSurface_ =
        eglCreateWindowSurface(eglDisplay_, eglConfig_, reinterpret_cast<EGLNativeWindowType>(window.Raw()), nullptr);
    if (eglSurface_ == EGL_NO_SURFACE) {
        RTC_LOG(LS_ERROR) << "Failed to create window surface: " << eglGetError();
        return false;
    }

    return true;
}

void EglEnv::ReleaseSurface()
{
    if (eglSurface_ != EGL_NO_SURFACE) {
        eglDestroySurface(eglDisplay_, eglSurface_);
        eglSurface_ = EGL_NO_SURFACE;
    }
}

bool EglEnv::MakeCurrent()
{
    if (eglGetCurrentContext() == eglContext_) {
        return true;
    }

    if (!eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_)) {
        RTC_LOG(LS_ERROR) << "Failed to make current, errno: " << eglGetError();
        return false;
    }

    return true;
}

bool EglEnv::DetachCurrent()
{
    if (!eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        RTC_LOG(LS_ERROR) << "Failed to detach Current, errno: " << eglGetError();
        return false;
    }

    return true;
}

bool EglEnv::SwapBuffers()
{
    if (eglSurface_ == EGL_NO_SURFACE) {
        return false;
    }

    EGLBoolean ret = eglSwapBuffers(eglDisplay_, eglSurface_);
    if (ret == EGL_FALSE) {
        RTC_LOG(LS_ERROR) << "Failed to swap buffers, errno: " << eglGetError();
        return false;
    }

    return true;
}

bool EglEnv::SwapBuffers(uint64_t timestampNs)
{
    if (eglSurface_ == EGL_NO_SURFACE) {
        RTC_LOG(LS_ERROR) << "No egl surface";
        return false;
    }

    if (eglPresentationTimeANDROID_) {
        if (eglPresentationTimeANDROID_(eglDisplay_, eglSurface_, timestampNs) == EGL_FALSE) {
            RTC_LOG(LS_WARNING) << "Failed to eglPresentationTimeANDROID, errno: " << eglGetError();
        }
    }

    if (eglSwapBuffers(eglDisplay_, eglSurface_) == EGL_FALSE) {
        RTC_LOG(LS_ERROR) << "Failed to swap buffers, errno: " << eglGetError();
        return false;
    }

    return true;
}

int EglEnv::GetSurfaceWidth() const
{
    int width;
    if (eglQuerySurface(eglDisplay_, eglSurface_, EGL_WIDTH, &width) == EGL_FALSE) {
        RTC_LOG(LS_ERROR) << "Failed to query surface width, errno: " << eglGetError();
        return -1;
    }
    return width;
}

int EglEnv::GetSurfaceHeight() const
{
    int height;
    if (eglQuerySurface(eglDisplay_, eglSurface_, EGL_HEIGHT, &height) == EGL_FALSE) {
        RTC_LOG(LS_ERROR) << "Failed to query surface height, errno: " << eglGetError();
        return -1;
    }
    return height;
}

bool EglEnv::Init(EGLContext sharedContext, const std::vector<int32_t>& configAttributes)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    // 获取当前的显示设备
    eglDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay_ == EGL_NO_DISPLAY) {
        RTC_LOG(LS_ERROR) << "Failed to create EGLDisplay gl errno: " << eglGetError();
        return false;
    }
    RTC_DLOG(LS_VERBOSE) << "eglDisplay_: " << eglDisplay_;

    EGLint major, minor;
    // 初始化EGLDisplay
    if (eglInitialize(eglDisplay_, &major, &minor) == EGL_FALSE) {
        RTC_LOG(LS_ERROR) << "Failed to initialize EGLDisplay";
        return false;
    }
    RTC_LOG(LS_VERBOSE) << "eglInitialize success, version: " << major << "." << minor;

    SetupExtensions();

    // 绑定图形绘制的API为OpenGLES
    if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
        RTC_LOG(LS_ERROR) << "Failed to bind OpenGL ES API";
        return false;
    }

    unsigned int ret;
    EGLint count = 0;

    // 获取一个有效的系统配置信息
    ret = eglChooseConfig(eglDisplay_, configAttributes.data(), &eglConfig_, 1, &count);
    if (!(ret && static_cast<unsigned int>(count) >= 1)) {
        RTC_LOG(LS_ERROR) << "Failed to choose config";
        return false;
    }
    RTC_LOG(LS_VERBOSE) << "eglChooseConfig success, config: " << eglConfig_;

    const EGLint contextAttrs_[] = {
        EGL_CONTEXT_CLIENT_VERSION, GetOpenGLESVersionFromConfig(configAttributes), EGL_NONE};

    // 创建上下文
    eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, sharedContext, contextAttrs_);
    if (eglContext_ == EGL_NO_CONTEXT) {
        RTC_LOG(LS_ERROR) << "Failed to create egl context: " << GetEglErrorString();
        return false;
    }
    RTC_DLOG(LS_VERBOSE) << "eglContext_: " << eglContext_;

    // EGL环境初始化完成
    RTC_LOG(LS_VERBOSE) << "Create EGL context successfully";

    return true;
}

void EglEnv::Release()
{
    eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (eglContext_ != EGL_NO_CONTEXT) {
        if (!eglDestroyContext(eglDisplay_, eglContext_)) {
            RTC_LOG(LS_ERROR) << "Failed to destroy egl context: " << GetEglErrorString();
        }
        eglContext_ = EGL_NO_CONTEXT;
    }

    eglReleaseThread();
    eglTerminate(eglDisplay_);
    eglDisplay_ = EGL_NO_DISPLAY;
}

void EglEnv::SetupExtensions()
{
    const char* extensions = eglQueryString(eglDisplay_, EGL_EXTENSIONS);
    if (extensions == nullptr) {
        RTC_LOG(LS_WARNING) << "Egl no extensions";
        return;
    }

    RTC_DLOG(LS_VERBOSE) << "Egl extensions: " << extensions;

    if (CheckEglExtension(extensions, "EGL_ANDROID_presentation_time")) {
        eglPresentationTimeANDROID_ =
            reinterpret_cast<PFNEGLPRESENTATIONTIMEANDROIDPROC>(eglGetProcAddress("eglPresentationTimeANDROID"));
        if (!eglPresentationTimeANDROID_) {
            RTC_LOG(LS_WARNING) << "Failed to get proc address of eglPresentationTimeANDROID";
        }
    } else {
        RTC_LOG(LS_WARNING) << "No egl extension of eglPresentationTimeANDROID";
    }

    if (!CheckEglExtension(extensions, "GL_OES_EGL_image_external")) {
        RTC_DLOG(LS_VERBOSE) << "No egl extension: GL_OES_EGL_image_external";
    }
}

using namespace Napi;

FunctionReference NapiEglEnv::constructor_;

void NapiEglEnv::Init(Napi::Env env, Napi::Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            InstanceMethod<&NapiEglEnv::GetContext>(kMethodNameGetContext),
            InstanceMethod<&NapiEglEnv::ToJson>(kMethodNameToJson),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiEglEnv::NapiEglEnv(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NapiEglEnv>(info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    eglEnv_ = EglEnv::Create(nullptr, EglConfigAttributes::DEFAULT);
}

NapiEglEnv::~NapiEglEnv() = default;

Napi::Value NapiEglEnv::GetContext(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    return NapiEglContext::NewInstance(info.Env(), eglEnv_->GetContext());
}

Napi::Value NapiEglEnv::ToJson(const Napi::CallbackInfo& info)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    auto json = Object::New(info.Env());
#ifndef NDEBUG
    json.Set("__native_class__", String::New(info.Env(), "NapiEglEnv"));
#endif

    return json;
}

} // namespace webrtc
