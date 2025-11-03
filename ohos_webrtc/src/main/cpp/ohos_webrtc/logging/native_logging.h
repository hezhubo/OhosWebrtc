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

#ifndef WEBRTC_NATIVE_LOGGING_H
#define WEBRTC_NATIVE_LOGGING_H

#include "napi.h"

namespace webrtc {

class NapiNativeLogging : public Napi::ObjectWrap<NapiNativeLogging> {
public:
    static void Init(Napi::Env env, Napi::Object exports);

    explicit NapiNativeLogging(const Napi::CallbackInfo& info);

protected:
    static Napi::Value InjectLoggable(const Napi::CallbackInfo& info);
    static Napi::Value DeleteLoggable(const Napi::CallbackInfo& info);
    static Napi::Value EnableLogToDebugOutput(const Napi::CallbackInfo& info);
    static Napi::Value EnableLogThreads(const Napi::CallbackInfo& info);
    static Napi::Value EnableLogTimeStamps(const Napi::CallbackInfo& info);
    static Napi::Value Log(const Napi::CallbackInfo& info);

private:
    static Napi::FunctionReference constructor_;
};

} // namespace webrtc

#endif // WEBRTC_NATIVE_LOGGING_H
