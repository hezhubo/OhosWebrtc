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

#include "configuration.h"
#include "certificate.h"

#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

const char kAttributeNameIceServers[] = "iceServers";
const char kAttributeNameUrls[] = "urls";
const char kAttributeNameUserName[] = "username";
const char kAttributeNameCredential[] = "credential";
const char kAttributeNameIceTransportPolicy[] = "iceTransportPolicy";
const char kAttributeNameBundlePolicy[] = "bundlePolicy";
const char kAttributeNameRtcpMuxPolicy[] = "rtcpMuxPolicy";
const char kAttributeNameCertificates[] = "certificates";
const char kAttributeNameIceCandidatePoolSize[] = "iceCandidatePoolSize";

const char kEnumIceTransportPolicyAll[] = "all";
const char kEnumIceTransportPolicyRelay[] = "relay";
const char kEnumBundlePolicyBalanced[] = "balanced";
const char kEnumBundlePolicyMaxBundle[] = "max-bundle";
const char kEnumBundlePolicyMaxCompact[] = "max-compat";
const char kEnumRtcpMuxPolicyRequire[] = "require";

bool JsToNativeIceServer(const Object& jsIceServer, PeerConnectionInterface::IceServer& iceServer)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    auto jsUrls = jsIceServer.Get(kAttributeNameUrls);
    if (jsUrls.IsString()) {
        auto url = jsUrls.As<String>().Utf8Value();
        iceServer.urls.push_back(url);
    } else if (jsUrls.IsArray()) {
        auto jsUrlArray = jsUrls.As<Array>();
        for (uint32_t i = 0; i < jsUrlArray.Length(); i++) {
            Napi::Value jsUrl = jsUrlArray[i];
            if (jsUrl.IsString()) {
                auto url = jsUrl.As<String>().Utf8Value();
                iceServer.urls.push_back(url);
            } else {
                RTC_LOG(LS_WARNING) << "element of urls is not string";
            }
        }
    } else {
        RTC_LOG(LS_WARNING) << "urls is not string nor array";
    }

    if (jsIceServer.Has(kAttributeNameUserName)) {
        auto jsUserName = jsIceServer.Get(kAttributeNameUserName);
        if (jsUserName.IsString()) {
            auto username = jsUserName.As<String>().Utf8Value();
            iceServer.username = username;
        } else {
            RTC_LOG(LS_WARNING) << "username is not string";
        }
    }

    if (jsIceServer.Has(kAttributeNameCredential)) {
        auto jsCredential = jsIceServer.Get(kAttributeNameCredential);
        if (jsCredential.IsString()) {
            auto password = jsCredential.As<String>().Utf8Value();
            iceServer.password = password;
        } else {
            RTC_LOG(LS_WARNING) << "credential is not string";
        }
    }

    return true;
}

bool NativeToJsIceServer(const PeerConnectionInterface::IceServer& iceServer, Object& jsIceServer)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Napi::Env env = jsIceServer.Env();

    if (iceServer.urls.size() == 1) {
        jsIceServer.Set(kAttributeNameUrls, String::New(env, iceServer.urls[0]));
    } else {
        auto jsUrlArray = Array::New(env, iceServer.urls.size());
        for (uint32_t i = 0; i < iceServer.urls.size(); i++) {
            jsUrlArray[i] = String::New(env, iceServer.urls[i]);
        }
        jsIceServer.Set(kAttributeNameUrls, jsUrlArray);
    }

    if (!iceServer.username.empty()) {
        jsIceServer.Set(kAttributeNameUserName, String::New(env, iceServer.username));
    }

    if (!iceServer.password.empty()) {
        jsIceServer.Set(kAttributeNameCredential, String::New(env, iceServer.password));
    }

    return true;
}

bool JsToNativeConfiguration(
    const Napi::Object& jsConfiguration, PeerConnectionInterface::RTCConfiguration& configuration)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    if (jsConfiguration.Has(kAttributeNameIceServers)) {
        auto jsIceServers = jsConfiguration.Get(kAttributeNameIceServers);
        if (jsIceServers.IsArray()) {
            auto array = jsIceServers.As<Array>();
            for (size_t i = 0; i < array.Length(); ++i) {
                Napi::Value jsIceServer = array[i];
                if (jsIceServer.IsObject()) {
                    PeerConnectionInterface::IceServer iceServer;
                    JsToNativeIceServer(jsIceServer.As<Object>(), iceServer);
                    configuration.servers.push_back(iceServer);
                } else {
                    RTC_LOG(LS_WARNING) << "element of iceServers is not object";
                }
            }
        } else {
            RTC_LOG(LS_WARNING) << "iceServers is not array";
        }
    }

    if (jsConfiguration.Has(kAttributeNameIceTransportPolicy)) {
        auto jsIceTransportPolicy = jsConfiguration.Get(kAttributeNameIceTransportPolicy);
        if (jsIceTransportPolicy.IsString()) {
            auto iceTransportPolicy = jsIceTransportPolicy.As<String>().Utf8Value();
            if (iceTransportPolicy == kEnumIceTransportPolicyAll) {
                configuration.type = PeerConnectionInterface::kAll;
            } else if (iceTransportPolicy == kEnumIceTransportPolicyRelay) {
                configuration.type = PeerConnectionInterface::kRelay;
            } else {
                RTC_LOG(LS_WARNING) << "Invalid iceTransportPolicy";
            }
        } else {
            RTC_LOG(LS_WARNING) << "iceTransportPolicy is not string";
        }
    }

    if (jsConfiguration.Has(kAttributeNameBundlePolicy)) {
        auto jsBundlePolicy = jsConfiguration.Get(kAttributeNameBundlePolicy);
        if (jsBundlePolicy.IsString()) {
            auto bundlePolicy = jsBundlePolicy.As<String>().Utf8Value();
            if (bundlePolicy == kEnumBundlePolicyBalanced) {
                configuration.bundle_policy = PeerConnectionInterface::kBundlePolicyBalanced;
            } else if (bundlePolicy == kEnumBundlePolicyMaxCompact) {
                configuration.bundle_policy = PeerConnectionInterface::kBundlePolicyMaxCompat;
            } else if (bundlePolicy == kEnumBundlePolicyMaxBundle) {
                configuration.bundle_policy = PeerConnectionInterface::kBundlePolicyMaxBundle;
            } else {
                RTC_LOG(LS_WARNING) << "Invalid bundlePolicy";
            }
        } else {
            RTC_LOG(LS_WARNING) << "bundlePolicy is not string";
        }
    }

    if (jsConfiguration.Has(kAttributeNameRtcpMuxPolicy)) {
        auto jsRtcpMuxPolicy = jsConfiguration.Get(kAttributeNameRtcpMuxPolicy);
        if (jsRtcpMuxPolicy.IsString()) {
            auto rtcpMuxPolicy = jsRtcpMuxPolicy.As<String>().Utf8Value();
            if (rtcpMuxPolicy == kEnumRtcpMuxPolicyRequire) {
                configuration.rtcp_mux_policy = PeerConnectionInterface::kRtcpMuxPolicyRequire;
            } else {
                RTC_LOG(LS_WARNING) << "Invalid rtcpMuxPolicy";
            }
        } else {
            RTC_LOG(LS_WARNING) << "rtcpMuxPolicy is not string";
        }
    }

    if (jsConfiguration.Has(kAttributeNameCertificates)) {
        auto jsCertificates = jsConfiguration.Get(kAttributeNameCertificates);
        if (jsCertificates.IsArray()) {
            auto jsCertificateArray = jsCertificates.As<Array>();
            for (uint32_t i = 0; i < jsCertificateArray.Length(); i++) {
                Napi::Value jsCertificate = jsCertificateArray[i];
                auto certificate = NapiCertificate::Unwrap(jsCertificate.As<Object>());
                if (certificate) {
                    configuration.certificates.push_back(certificate->Get());
                }
            }
        }
    }

    if (jsConfiguration.Has(kAttributeNameIceCandidatePoolSize)) {
        auto jsIceCandidatePoolSize = jsConfiguration.Get(kAttributeNameIceCandidatePoolSize);
        if (jsIceCandidatePoolSize.IsNumber()) {
            configuration.ice_candidate_pool_size = jsIceCandidatePoolSize.As<Number>().Int32Value();
        }
    }

    return true;
}

bool NativeToJsConfiguration(
    const PeerConnectionInterface::RTCConfiguration& configuration, Napi::Object& jsConfiguration)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    Napi::Env env = jsConfiguration.Env();

    if (configuration.servers.size() > 0) {
        auto jsIceServerArray = Array::New(env, configuration.servers.size());
        for (uint32_t i = 0; i < configuration.servers.size(); i++) {
            auto jsIceServer = Object::New(env);
            NativeToJsIceServer(configuration.servers[i], jsIceServer);
            jsIceServerArray[i] = jsIceServer;
        }
        jsConfiguration.Set(kAttributeNameIceServers, jsIceServerArray);
    }

    switch (configuration.type) {
        case PeerConnectionInterface::kAll:
            jsConfiguration.Set(kAttributeNameIceTransportPolicy, String::New(env, kEnumIceTransportPolicyAll));
            break;
        case PeerConnectionInterface::kRelay:
            jsConfiguration.Set(kAttributeNameIceTransportPolicy, String::New(env, kEnumIceTransportPolicyRelay));
            break;
        default:
            RTC_LOG(LS_ERROR) << "Invalid value of " << kAttributeNameIceTransportPolicy;
            break;
    }

    switch (configuration.bundle_policy) {
        case PeerConnectionInterface::kBundlePolicyBalanced:
            jsConfiguration.Set(kAttributeNameBundlePolicy, String::New(env, kEnumBundlePolicyBalanced));
            break;
        case PeerConnectionInterface::kBundlePolicyMaxBundle:
            jsConfiguration.Set(kAttributeNameBundlePolicy, String::New(env, kEnumBundlePolicyMaxBundle));
            break;
        case PeerConnectionInterface::kBundlePolicyMaxCompat:
            jsConfiguration.Set(kAttributeNameBundlePolicy, String::New(env, kEnumBundlePolicyMaxCompact));
            break;
        default:
            RTC_LOG(LS_ERROR) << "Invalid value of " << kAttributeNameBundlePolicy;
            break;
    }

    switch (configuration.rtcp_mux_policy) {
        case PeerConnectionInterface::kRtcpMuxPolicyRequire:
            jsConfiguration.Set(kAttributeNameRtcpMuxPolicy, String::New(env, kEnumRtcpMuxPolicyRequire));
            break;
        default:
            RTC_LOG(LS_ERROR) << "Invalid value of " << kAttributeNameRtcpMuxPolicy;
            break;
    }

    if (configuration.certificates.size() > 0) {
        auto jsCertificateArray = Array::New(env, configuration.certificates.size());
        for (uint32_t i = 0; i < configuration.certificates.size(); i++) {
            auto certificate = configuration.certificates[i];
            auto jsCertificate = NapiCertificate::NewInstance(env, certificate);
            jsCertificateArray[i] = jsCertificate;
        }
        jsConfiguration.Set(kAttributeNameCertificates, jsCertificateArray);
    }

    jsConfiguration.Set(kAttributeNameIceCandidatePoolSize, Number::New(env, configuration.ice_candidate_pool_size));

    return true;
}

} // namespace webrtc
