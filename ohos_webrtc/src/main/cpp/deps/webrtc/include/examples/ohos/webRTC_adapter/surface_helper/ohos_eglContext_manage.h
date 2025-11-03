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
#ifndef OHOS_EGLCONTEXT_MANAGE_H
#define OHOS_EGLCONTEXT_MANAGE_H

#include <map>
#include <mutex>
#include <queue>

#include "GLES3/gl32.h"
#include "EGL/egl.h"

#include "commom/ohos_video_buffer.h"

namespace webrtc {
namespace ohos {

struct EGLContextResource {
    EGLContext eglContext_ = {EGL_NO_CONTEXT};
    mutable std::mutex textureIDMutex_;

    explicit EGLContextResource(EGLContext eglContext = EGL_NO_CONTEXT): eglContext_(eglContext) {}
};

class OhosEGLContextManage {
public:
    static OhosEGLContextManage& GetInstance() { return ohosEGLContextManage_; }

    // contextMap_ 相关操作
    // AddEGLContext执行时，make_shared可能会抛出std::bad_alloc异常
    bool AddEGLContext(OHOSVideoBuffer::VideoSourceType sourceType, EGLContext eglContext);
    std::shared_ptr<EGLContextResource> GetEGLContextResource(OHOSVideoBuffer::VideoSourceType sourceType);
    bool DelEGLContext(OHOSVideoBuffer::VideoSourceType sourceType);

private:
    OhosEGLContextManage() {};
    ~OhosEGLContextManage() {};
    OhosEGLContextManage(OhosEGLContextManage&) = delete;
    OhosEGLContextManage& operator=(const  OhosEGLContextManage&) = delete;
    static OhosEGLContextManage ohosEGLContextManage_;

    // EGLContext map and mutex
    mutable std::mutex mapMutex_;
    std::map<OHOSVideoBuffer::VideoSourceType, std::shared_ptr<EGLContextResource>> eglContextMap_;
};

} // namespace ohos
} // namespace webrtc

#endif // OHOS_EGLCONTEXT_MANAGE_H
