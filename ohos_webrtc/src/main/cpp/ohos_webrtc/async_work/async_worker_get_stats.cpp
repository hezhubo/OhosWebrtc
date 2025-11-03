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

#include "async_worker_get_stats.h"

namespace webrtc {

using namespace Napi;

const char kAttributeNameId[] = "id";
const char kAttributeNameType[] = "type";
const char kAttributeNameTimestamp[] = "timestamp";

class NapiMap : public Napi::Object {
public:
    NapiMap(napi_env env, napi_value value) : Object(env, value) {}

    static NapiMap Create(Napi::Env env)
    {
        return env.Global().Get("Map").As<Napi::Function>().New({}).As<NapiMap>();
    }

    void Set(Napi::Value key, Napi::Value value)
    {
        Get("set").As<Napi::Function>().Call(*this, {key, value});
    }
};

class GetStatsCallback : public RTCStatsCollectorCallback {
public:
    explicit GetStatsCallback(AsyncWorkerGetStats* asyncWorker) : asyncWorker_(asyncWorker) {}

protected:
    void OnStatsDelivered(const rtc::scoped_refptr<const RTCStatsReport>& report) override
    {
        if (!asyncWorker_) {
            return;
        }

        asyncWorker_->SetReport(report);
        asyncWorker_->Queue();
        asyncWorker_ = nullptr;
    }

private:
    AsyncWorkerGetStats* asyncWorker_;
};

AsyncWorkerGetStats* AsyncWorkerGetStats::Create(Napi::Env env, const char* resourceName)
{
    auto asyncWorker = new AsyncWorkerGetStats(env, resourceName);
    asyncWorker->callback_ = rtc::make_ref_counted<GetStatsCallback>(asyncWorker);
    return asyncWorker;
}

AsyncWorkerGetStats::AsyncWorkerGetStats(Napi::Env env, const char* resourceName)
    : AsyncWorker(env, resourceName), deferred_(Napi::Promise::Deferred::New(env)), callback_()
{
}

rtc::scoped_refptr<RTCStatsCollectorCallback> AsyncWorkerGetStats::GetCallback() const
{
    return callback_;
}

void AsyncWorkerGetStats::SetReport(const rtc::scoped_refptr<const RTCStatsReport>& report)
{
    report_ = report;
}

void AsyncWorkerGetStats::Execute()
{
    // do nothing
}

void AsyncWorkerGetStats::OnOK()
{
    auto jsStatsReportObj = Napi::Object::New(Env());
    auto jsStatsMap = NapiMap::Create(Env());

    if (report_) {
        for (auto it = report_->begin(); it != report_->end(); it++) {
            auto jsStats = Napi::Object::New(Env());
            jsStats.Set(kAttributeNameId, Napi::String::New(Env(), it->id()));
            jsStats.Set(kAttributeNameType, Napi::String::New(Env(), it->type()));
            jsStats.Set(kAttributeNameTimestamp, Napi::Number::New(Env(), it->timestamp().ms()));

            for (const auto& member : it->Members()) {
                if (!member || !member->is_defined()) {
                    continue;
                }

                switch (member->type()) {
                    case RTCStatsMemberInterface::kBool:
                        jsStats.Set(
                            member->name(), Napi::Boolean::New(Env(), *member->cast_to<RTCStatsMember<bool>>()));
                        break;
                    case RTCStatsMemberInterface::kInt32:
                        jsStats.Set(
                            member->name(), Napi::Number::New(Env(), *member->cast_to<RTCStatsMember<int32_t>>()));
                        break;
                    case RTCStatsMemberInterface::kUint32:
                        jsStats.Set(
                            member->name(), Napi::Number::New(Env(), *member->cast_to<RTCStatsMember<uint32_t>>()));
                        break;
                    case RTCStatsMemberInterface::kInt64:
                        jsStats.Set(
                            member->name(), Napi::Number::New(Env(), *member->cast_to<RTCStatsMember<int64_t>>()));
                        break;
                    case RTCStatsMemberInterface::kUint64:
                        jsStats.Set(
                            member->name(), Napi::Number::New(Env(), *member->cast_to<RTCStatsMember<uint64_t>>()));
                        break;
                    case RTCStatsMemberInterface::kDouble:
                        jsStats.Set(
                            member->name(), Napi::Number::New(Env(), *member->cast_to<RTCStatsMember<double>>()));
                        break;
                    case RTCStatsMemberInterface::kString:
                        jsStats.Set(member->name(), Napi::String::New(Env(), member->ValueToString()));
                        break;
                    default:
                        jsStats.Set(member->name(), Napi::String::New(Env(), member->ValueToJson()));
                        break;
                }
            }

            jsStatsMap.Set(Napi::String::New(Env(), it->id()), jsStats);
        }
    }

    jsStatsReportObj.Set("stats", jsStatsMap);
    deferred_.Resolve(jsStatsReportObj);
}

void AsyncWorkerGetStats::OnError(const Napi::Error& e)
{
    deferred_.Reject(e.Value());
}

} // namespace webrtc
