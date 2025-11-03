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

#ifndef WEBRTC_HILOG_SINK_H
#define WEBRTC_HILOG_SINK_H

#include <string>

#include "absl/strings/string_view.h"
#include "rtc_base/logging.h"

class HilogSink : public rtc::LogSink {
public:
    HilogSink();
    ~HilogSink() override;

    void OnLogMessage(const std::string& msg) override;
    void OnLogMessage(const std::string& msg, rtc::LoggingSeverity severity, const char* tag) override;
    void OnLogMessage(absl::string_view msg, rtc::LoggingSeverity severity, const char* tag) override;
    void OnLogMessage(const rtc::LogLineRef& line) override;
};

#endif // WEBRTC_HILOG_SINK_H
