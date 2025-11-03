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

#include "native_window_renderer.h"

namespace webrtc {
namespace adapter {

NativeWindowRenderer::~NativeWindowRenderer() = default;

uint64_t NativeWindowRenderer::GetSurfaceId() const
{
    return window_.GetSurfaceId();
}

NativeWindowRenderer::NativeWindowRenderer(ohos::NativeWindow window) : window_(std::move(window)) {}

} // namespace adapter
} // namespace webrtc