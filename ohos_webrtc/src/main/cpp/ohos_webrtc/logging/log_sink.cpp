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

#include "log_sink.h"

namespace webrtc {

using namespace std;
using namespace Napi;

const char kMethodNameLogMessage[] = "logMessage";

LogSink::LogSink(napi_env env, Napi::Object loggable)
{
    if (!loggable.Has(kMethodNameLogMessage)) {
        NAPI_THROW_VOID(Error::New(env, "Invalid argument"));
    }

    tsfn_ = TSFN::New(
        env, loggable.Get(kMethodNameLogMessage).As<Function>(), kMethodNameLogMessage, 0, 1,
        new Context(Napi::Persistent(loggable)), [](Napi::Env, FinalizerDataType*, Context* ctx) { delete ctx; });
}

LogSink::~LogSink()
{
    tsfn_.Release();
}

void LogSink::OnLogMessage(const string& msg)
{
    RTC_DCHECK_NOTREACHED();
}

void LogSink::OnLogMessage(const string& msg, rtc::LoggingSeverity severity, const char* tag)
{
    tsfn_.BlockingCall(new DataType{severity, msg, tag});
}

void LogSink::OnLogMessage(absl::string_view msg, rtc::LoggingSeverity severity, const char* tag)
{
    tsfn_.BlockingCall(new DataType{severity, std::string(msg), tag});
}

void LogSink::OnLogMessage(const rtc::LogLineRef& line)
{
    // Inefficient
    tsfn_.BlockingCall(new DataType{line.severity(), line.DefaultLogLine(), std::string(line.tag())});
}

void LogSink::CallJs(Napi::Env env, Napi::Function callback, Context* context, DataType* data)
{
    if (env == nullptr) {
        // Javascript environment is not available to call into
        return;
    }

    callback.Call(
        context->Value(),
        {String::New(env, data->msg.data()), Number::New(env, data->severity), String::New(env, data->tag)});

    delete data;
}

} // namespace webrtc
