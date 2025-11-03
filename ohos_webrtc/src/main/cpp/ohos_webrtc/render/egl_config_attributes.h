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

#ifndef WEBRTC_RENDER_EGL_CONFIG_ATTRIBUTES_H
#define WEBRTC_RENDER_EGL_CONFIG_ATTRIBUTES_H

#include <cstdint>
#include <vector>

#include <EGL/egl.h>

namespace webrtc {

constexpr int32_t kEGLPixelComponentBits = 8;
constexpr int32_t kOpenGLVersion_1 = 1;
constexpr int32_t kOpenGLVersion_2 = 2;
constexpr int32_t kOpenGLVersion_3 = 3;

class EglConfigAttributes {
public:
    class Builder {
    public:
        Builder& SetVersion(int version)
        {
            version_ = version;
            return *this;
        }

        Builder& SetHasAlphaChannel(bool hasAlphaChannel)
        {
            hasAlphaChannel_ = hasAlphaChannel;
            return *this;
        }

        Builder& SetSupportsPixelBuffer(bool supportsPixelBuffer)
        {
            supportsPixelBuffer_ = supportsPixelBuffer;
            return *this;
        }

        std::vector<int32_t> Build() const
        {
            std::vector<int32_t> attrs;
            attrs.push_back(EGL_RED_SIZE);
            attrs.push_back(kEGLPixelComponentBits);
            attrs.push_back(EGL_GREEN_SIZE);
            attrs.push_back(kEGLPixelComponentBits);
            attrs.push_back(EGL_BLUE_SIZE);
            attrs.push_back(kEGLPixelComponentBits);
            if (hasAlphaChannel_) {
                attrs.push_back(EGL_ALPHA_SIZE);
                attrs.push_back(kEGLPixelComponentBits);
            }
            if (version_ == kOpenGLVersion_2 || version_ == kOpenGLVersion_3) {
                attrs.push_back(EGL_RENDERABLE_TYPE);
                attrs.push_back(version_ == kOpenGLVersion_3 ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT);
            }
            if (supportsPixelBuffer_) {
                attrs.push_back(EGL_SURFACE_TYPE);
                attrs.push_back(EGL_PBUFFER_BIT);
            }
            attrs.push_back(EGL_NONE);

            return attrs;
        }

    private:
        int version_{kOpenGLVersion_3};
        bool hasAlphaChannel_{false};
        bool supportsPixelBuffer_{false};
    };

    static const std::vector<int32_t> DEFAULT;
    static const std::vector<int32_t> RGBA;
    static const std::vector<int32_t> PIXEL_BUFFER;
    static const std::vector<int32_t> RGBA_PIXEL_BUFFER;
};

inline const std::vector<int32_t> EglConfigAttributes::DEFAULT = Builder().Build();

inline const std::vector<int32_t> EglConfigAttributes::RGBA = Builder().SetHasAlphaChannel(true).Build();

inline const std::vector<int32_t> EglConfigAttributes::PIXEL_BUFFER = Builder().SetSupportsPixelBuffer(true).Build();

inline const std::vector<int32_t> EglConfigAttributes::RGBA_PIXEL_BUFFER =
    Builder().SetHasAlphaChannel(true).SetSupportsPixelBuffer(true).Build();

} // namespace webrtc

#endif // WEBRTC_RENDER_EGL_CONFIG_ATTRIBUTES_H
