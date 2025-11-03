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

#include "hilog_sink.h"

#include "hilog/log.h"

HilogSink::HilogSink() = default;
HilogSink::~HilogSink() = default;

void HilogSink::OnLogMessage(const std::string& msg)
{
    RTC_DCHECK_NOTREACHED();
}

void HilogSink::OnLogMessage(const std::string& msg, rtc::LoggingSeverity severity, const char* tag)
{
    OnLogMessage(absl::string_view{msg}, severity, tag);
}

void HilogSink::OnLogMessage(absl::string_view msg, rtc::LoggingSeverity severity, const char* tag)
{
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
            return;
        default:
            break;
    }
    OH_LOG_Print(LOG_APP, level, LOG_DOMAIN, tag, "%{public}s", msg.data());
}

void HilogSink::OnLogMessage(const rtc::LogLineRef& line)
{
    OnLogMessage(line.DefaultLogLine(), line.severity(), line.tag().data());
}
