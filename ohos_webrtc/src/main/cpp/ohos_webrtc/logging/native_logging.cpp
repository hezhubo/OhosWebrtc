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

#include "native_logging.h"
#include "hilog_sink.h"
#include "log_sink.h"

#include "hilog/log.h"

namespace webrtc {

using namespace Napi;

const char kClassName[] = "NativeLogging";

const char kMethodNameInjectLoggable[] = "injectLoggable";
const char kMethodNameDeleteLoggable[] = "deleteLoggable";
const char kMethodNameEnableLogToDebugOutput[] = "enableLogToDebugOutput";
const char kMethodNameEnableLogThreads[] = "enableLogThreads";
const char kMethodNameEnableLogTimeStamps[] = "enableLogTimeStamps";
const char kMethodNameLog[] = "log";

struct StaticObjectContainer {
    std::unique_ptr<LogSink> logSink;
    std::unique_ptr<HilogSink> hilogSink;
};

StaticObjectContainer& GetStaticObjects()
{
    static StaticObjectContainer* static_objects = new StaticObjectContainer();
    return *static_objects;
}

FunctionReference NapiNativeLogging::constructor_;

void NapiNativeLogging::Init(class Env env, Object exports)
{
    Function func = DefineClass(
        env, kClassName,
        {
            StaticMethod<&NapiNativeLogging::InjectLoggable>(kMethodNameInjectLoggable),
            StaticMethod<&NapiNativeLogging::DeleteLoggable>(kMethodNameDeleteLoggable),
            StaticMethod<&NapiNativeLogging::EnableLogToDebugOutput>(kMethodNameEnableLogToDebugOutput),
            StaticMethod<&NapiNativeLogging::EnableLogThreads>(kMethodNameEnableLogThreads),
            StaticMethod<&NapiNativeLogging::EnableLogTimeStamps>(kMethodNameEnableLogTimeStamps),
            StaticMethod<&NapiNativeLogging::Log>(kMethodNameLog),
        });
    exports.Set(kClassName, func);

    constructor_ = Persistent(func);
}

NapiNativeLogging::NapiNativeLogging(const Napi::CallbackInfo& info) : ObjectWrap<NapiNativeLogging>(info) {}

Napi::Value NapiNativeLogging::InjectLoggable(const Napi::CallbackInfo& info)
{
    OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_DOMAIN, kClassName, __FUNCTION__);

    auto loggable = info[0].As<Object>();
    auto severity = info[1].As<Number>().Int32Value();

    auto newLogSink = std::make_unique<LogSink>(info.Env(), loggable);

    auto& logSink = GetStaticObjects().logSink;
    if (logSink) {
        // If there is already a LogSink, remove it from LogMessage.
        rtc::LogMessage::RemoveLogToStream(logSink.get());
    }

    logSink = std::move(newLogSink);
    rtc::LogMessage::AddLogToStream(logSink.get(), static_cast<rtc::LoggingSeverity>(severity));

    auto& hilogSink = GetStaticObjects().hilogSink;
    if (hilogSink) {
        // remove debug logger
        rtc::LogMessage::RemoveLogToStream(hilogSink.get());
        hilogSink.reset();
    }

    return info.Env().Undefined();
}

Napi::Value NapiNativeLogging::DeleteLoggable(const Napi::CallbackInfo& info)
{
    OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_DOMAIN, kClassName, __FUNCTION__);

    auto& log_sink = GetStaticObjects().logSink;
    if (log_sink) {
        // If there is already a LogSink, remove it from LogMessage.
        rtc::LogMessage::RemoveLogToStream(log_sink.get());
        log_sink.reset();
    }

    return info.Env().Undefined();
}

Napi::Value NapiNativeLogging::EnableLogToDebugOutput(const Napi::CallbackInfo& info)
{
    OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_DOMAIN, kClassName, __FUNCTION__);

    auto& hilogSink = GetStaticObjects().hilogSink;
    if (hilogSink) {
        rtc::LogMessage::RemoveLogToStream(hilogSink.get());
    }

    auto severity = info[0].As<Number>().Int32Value();
    if (severity >= rtc::LS_VERBOSE && severity <= rtc::LS_NONE) {
        if (!hilogSink) {
            hilogSink = std::make_unique<HilogSink>();
        }
        rtc::LogMessage::AddLogToStream(hilogSink.get(), rtc::LS_VERBOSE);
    }
    return info.Env().Undefined();
}

Napi::Value NapiNativeLogging::EnableLogThreads(const Napi::CallbackInfo& info)
{
    rtc::LogMessage::LogThreads(true);
    return info.Env().Undefined();
}

Napi::Value NapiNativeLogging::EnableLogTimeStamps(const Napi::CallbackInfo& info)
{
    rtc::LogMessage::LogTimestamps(true);
    return info.Env().Undefined();
}

Napi::Value NapiNativeLogging::Log(const Napi::CallbackInfo& info)
{
    if (info.Length() != 3) {
        NAPI_THROW(Error::New(info.Env(), "Wrong number of arguments"), info.Env().Undefined());
    }

    auto message = info[0].As<String>().Utf8Value();
    auto severity = info[1].As<Number>().Int32Value();
    auto tag = info[2].As<String>().Utf8Value();

    LogLevel level = LOG_DEBUG;
    switch (severity) {
        case rtc::LS_VERBOSE:
            level = LOG_DEBUG;
            break;
        case rtc::LS_INFO:
            level = LOG_INFO;
            break;
        case rtc::LS_WARNING:
            level = LOG_WARN;
            break;
        case rtc::LS_ERROR:
            level = LOG_ERROR;
            break;
        case rtc::LS_NONE:
            return info.Env().Undefined();
        default:
            break;
    }

    OH_LOG_Print(LOG_APP, level, LOG_DOMAIN, tag.c_str(), "%{public}s", message.c_str());

    return info.Env().Undefined();
}

} // namespace webrtc
