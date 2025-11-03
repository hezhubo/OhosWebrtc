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

#ifndef WEBRTC_ASYNC_WORKER_CERTIFICATE_H
#define WEBRTC_ASYNC_WORKER_CERTIFICATE_H

#include "certificate.h"

#include "api/rtc_error.h"
#include "napi.h"
#include "api/peer_connection_interface.h"

namespace webrtc {

class AsyncWorkerCertificate : public Napi::AsyncWorker {
public:
    AsyncWorkerCertificate(Napi::Env env, const char* resourceName)
        : AsyncWorker(env, resourceName), deferred_(Napi::Promise::Deferred::New(env)), error_(RTCError::OK())
    {
    }

    Napi::Promise GetPromise()
    {
        return deferred_.Promise();
    }

    Napi::Promise::Deferred GetDeferred()
    {
        return deferred_;
    }

    void SetError(RTCError error)
    {
        error_ = std::move(error);
        AsyncWorker::SetError(std::string(error_.message()));
    }

    void start(const rtc::KeyParams& key_params, absl::optional<uint64_t> expires_ms)
    {
        key_params_ = key_params;
        expires_ms_ = expires_ms;
        Queue();
    }

protected:
    void Execute() override
    {
        certificate_ = certificateGenerator->GenerateCertificate(key_params_, expires_ms_);
    }

    void OnOK() override
    {
        auto result = NapiCertificate::NewInstance(Env(), certificate_);

        deferred_.Resolve(result);
    }

    void OnError(const Napi::Error& e) override
    {
        deferred_.Reject(e.Value());
    }

private:
    Napi::Promise::Deferred deferred_;
    RTCError error_;
    std::unique_ptr<rtc::RTCCertificateGenerator> certificateGenerator;
    rtc::scoped_refptr<rtc::RTCCertificate> certificate_;
    rtc::KeyParams key_params_;
    absl::optional<uint64_t> expires_ms_;
};

} // namespace webrtc

#endif // WEBRTC_ASYNC_WORKER_CERTIFICATE_H
