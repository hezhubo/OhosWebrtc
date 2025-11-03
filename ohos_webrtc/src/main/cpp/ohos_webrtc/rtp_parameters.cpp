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

#include "rtp_parameters.h"

#include <sstream>

#include "rtc_base/logging.h"
#include "rtc_base/string_encode.h"
#include "absl/strings/ascii.h"

namespace webrtc {

const char kAttributeNameRid[] = "rid";

const char kAttributeNameActive[] = "active";
const char kAttributeNameMaxBitrate[] = "maxBitrate";
const char kAttributeNameMaxFramerate[] = "maxFramerate";
const char kAttributeNameScaleResolutionDownBy[] = "scaleResolutionDownBy";

const char kAttributeNameClockRate[] = "clockRate";
const char kAttributeNameChannels[] = "channels";
const char kAttributeNameMimeType[] = "mimeType";
const char kAttributeNameSdpFmtpLine[] = "sdpFmtpLine";
const char kAttributeNamePayloadType[] = "payloadType";

const char kAttributeNameId[] = "id";
const char kAttributeNameUri[] = "uri";
const char kAttributeNameEncrypted[] = "encrypted";

const char kAttributeNameCname[] = "cname";
const char kAttributeNameReducedSize[] = "reducedSize";

const char kAttributeNameCodecs[] = "codecs";
const char kAttributeNameHeaderExtensions[] = "headerExtensions";
const char kAttributeNameRtcp[] = "rtcp";

const char kAttributeNameEncodings[] = "encodings";
const char kAttributeNameTransactionId[] = "transactionId";

using namespace Napi;

void WriteFmtpParameter(absl::string_view parameter_name, absl::string_view parameter_value, std::ostringstream& ss)
{
    if (parameter_name.empty()) {
        // RFC 2198 and RFC 4733 don't use key-value pairs.
        ss << parameter_value;
    } else {
        // fmtp parameters: `parameter_name`=`parameter_value`
        ss << parameter_name << "=" << parameter_value;
    }
}

bool WriteFmtpParameters(const std::map<std::string, std::string>& parameters, std::ostringstream& ss)
{
    bool empty = true;
    const char* delimiter = ""; // No delimiter before first parameter.
    for (const auto& entry : parameters) {
        const std::string& key = entry.first;
        const std::string& value = entry.second;

        ss << delimiter;
        // A semicolon before each subsequent parameter.
        delimiter = ";";
        WriteFmtpParameter(key, value, ss);
        empty = false;
    }

    return !empty;
}

bool ParseFmtpParam(absl::string_view line, std::string* parameter, std::string* value)
{
    if (!rtc::tokenize_first(line, '=', parameter, value)) {
        // Support for non-key-value lines like RFC 2198 or RFC 4733.
        *parameter = "";
        *value = std::string(line);
        return true;
    }
    // a=fmtp:<payload_type> <param1>=<value1>; <param2>=<value2>; ...
    return true;
}

bool ParseFmtpLine(const std::string& line_params, std::map<std::string, std::string>& parameters)
{
    if (line_params.size() == 0) {
        return false;
    }

    for (absl::string_view param : rtc::split(line_params, ';')) {
        if (param.size() == 0) {
            continue;
        }

        std::string name;
        std::string value;
        if (!ParseFmtpParam(absl::StripAsciiWhitespace(param), &name, &value)) {
            return false;
        }
        if (parameters.find(name) != parameters.end()) {
            RTC_LOG(LS_INFO) << "Overwriting duplicate fmtp parameter with key \"" << name << "\".";
        }
        parameters[name] = value;
    }

    return true;
}

bool ParseMimeType(const std::string& mimeType, std::string& kind, std::string& name)
{
    auto results = rtc::split(mimeType, '/');
    if (results.size() != 2) {
        // split failed, no '/' in mimeType
        return false;
    }

    kind = results[0];
    name = results[1];

    return true;
}

cricket::MediaType MediaTypeFromString(const std::string& kind)
{
    if (kind == cricket::kMediaTypeAudio) {
        return cricket::MEDIA_TYPE_AUDIO;
    } else if (kind == cricket::kMediaTypeVideo) {
        return cricket::MEDIA_TYPE_VIDEO;
    } else if (kind == cricket::kMediaTypeData) {
        return cricket::MEDIA_TYPE_DATA;
    }

    return cricket::MEDIA_TYPE_UNSUPPORTED;
}

struct NapiRtpCodingParameters {
    static void JsToNative(const Napi::Object& js, RtpEncodingParameters& native)
    {
        // rid?: string;
        if (js.Has(kAttributeNameRid)) {
            auto jsRid = js.Get(kAttributeNameRid);
            if (jsRid.IsString()) {
                native.rid = jsRid.As<String>().Utf8Value();
            }
        }
    }

    static void NativeToJs(const RtpEncodingParameters& native, Napi::Object& js)
    {
        js.Set(kAttributeNameRid, String::New(js.Env(), native.rid));
    }
};

struct NapiRtpCodecParameters {
    // clockRate: number;
    // channels?: number;
    // mimeType: string;
    // sdpFmtpLine: string;
    // payloadType: number;

    static void JsToNative(const Napi::Object& js, RtpCodecParameters& native)
    {
        if (!js.Has(kAttributeNameClockRate)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No clockRate"));
        }
        if (!js.Has(kAttributeNameMimeType)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No mimeType"));
        }
        if (!js.Has(kAttributeNameSdpFmtpLine)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No sdpFmtpLine"));
        }
        if (!js.Has(kAttributeNamePayloadType)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No payloadType"));
        }

        auto jsClockRate = js.Get(kAttributeNameClockRate);
        if (!jsClockRate.IsNumber()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The clockRate is not number"));
        }

        auto jsMimeType = js.Get(kAttributeNameMimeType);
        if (!jsMimeType.IsString()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The mimeType is not string"));
        }

        auto jsSdpFmtpLine = js.Get(kAttributeNameSdpFmtpLine);
        if (!jsSdpFmtpLine.IsString()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The sdpFmtpLine is not string"));
        }

        auto jsPayloadType = js.Get(kAttributeNamePayloadType);
        if (!jsPayloadType.IsNumber()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The payloadType is not number"));
        }

        native.clock_rate = jsClockRate.As<Number>().Int32Value();
        native.payload_type = jsPayloadType.As<Number>().Int32Value();

        std::string kind;
        std::string name;
        if (!ParseMimeType(jsMimeType.As<String>().Utf8Value(), kind, name)) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "Invalid mimeType"));
        }
        native.kind = MediaTypeFromString(kind);
        native.name = name;

        ParseFmtpLine(jsSdpFmtpLine.As<String>().Utf8Value(), native.parameters);

        if (js.Has(kAttributeNameChannels)) {
            auto jsChannels = js.Get(kAttributeNameChannels);
            if (jsChannels.IsNumber()) {
                native.num_channels = jsChannels.As<Number>().Int32Value();
            }
        }
    }

    static void NativeToJs(const RtpCodecParameters& native, Napi::Object& js)
    {
        js.Set(kAttributeNameMimeType, String::New(js.Env(), native.mime_type()));
        js.Set(kAttributeNamePayloadType, Number::New(js.Env(), native.payload_type));
        if (native.num_channels) {
            js.Set(kAttributeNameChannels, Number::New(js.Env(), native.num_channels.value()));
        }

        if (native.clock_rate) {
            js.Set(kAttributeNameClockRate, Number::New(js.Env(), native.clock_rate.value()));
        } else {
            // unset
            js.Set(kAttributeNameClockRate, Number::New(js.Env(), -1));
        }

        std::ostringstream ss;
        WriteFmtpParameters(native.parameters, ss);
        js.Set(kAttributeNameSdpFmtpLine, String::New(js.Env(), ss.str()));
    }
};

struct NapiRtpHeaderExtensionParameters {
    // id: number;
    // uri: string;
    // encrypted?: boolean;

    static void JsToNative(const Napi::Object& js, RtpExtension& native)
    {
        if (!js.Has(kAttributeNameId)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No id"));
        }
        if (!js.Has(kAttributeNameUri)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No uri"));
        }

        auto jsId = js.Get(kAttributeNameId);
        if (!jsId.IsNumber()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The id is not number"));
        }

        auto jsUri = js.Get(kAttributeNameUri);
        if (!jsUri.IsString()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The uri is not string"));
        }

        native.id = jsId.As<Number>().Int32Value();
        native.uri = jsUri.As<String>().Utf8Value();

        if (js.Has(kAttributeNameEncrypted)) {
            auto jsEncrypted = js.Get(kAttributeNameEncrypted);
            if (jsEncrypted.IsNumber()) {
                native.encrypt = jsEncrypted.As<Number>().Int32Value();
            }
        }
    }

    static void NativeToJs(const RtpExtension& native, Napi::Object& js)
    {
        js.Set(kAttributeNameId, Number::New(js.Env(), native.id));
        js.Set(kAttributeNameUri, String::New(js.Env(), native.uri));
        js.Set(kAttributeNameEncrypted, Boolean::New(js.Env(), native.encrypt));
    }
};

struct NapiRtcpParameters {
    // cname?: string;
    // reducedSize?: boolean;

    static void JsToNative(const Napi::Object& js, RtcpParameters& native)
    {
        if (js.Has(kAttributeNameCname)) {
            auto jsEncrypted = js.Get(kAttributeNameCname);
            if (jsEncrypted.IsString()) {
                native.cname = jsEncrypted.As<String>().Utf8Value();
            }
        }

        if (js.Has(kAttributeNameReducedSize)) {
            auto jsEncrypted = js.Get(kAttributeNameReducedSize);
            if (jsEncrypted.IsBoolean()) {
                native.reduced_size = jsEncrypted.As<Boolean>().Value();
            }
        }
    }

    static void NativeToJs(const RtcpParameters& native, Napi::Object& js)
    {
        js.Set(kAttributeNameCname, String::New(js.Env(), native.cname));
        js.Set(kAttributeNameReducedSize, Boolean::New(js.Env(), native.reduced_size));
    }
};

struct NapiRtpParameters {
    // codecs: RTCRtpCodecParameters[];
    // headerExtensions: RTCRtpHeaderExtensionParameters[];
    // rtcp: RTCRtcpParameters;

    static void JsToNative(const Napi::Object& js, RtpParameters& native)
    {
        if (!js.Has(kAttributeNameCodecs)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No codecs"));
        }
        if (!js.Has(kAttributeNameHeaderExtensions)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No headerExtensions"));
        }
        if (!js.Has(kAttributeNameRtcp)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No rtcp"));
        }

        auto jsCodecs = js.Get(kAttributeNameCodecs);
        if (!jsCodecs.IsArray()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The codecs is not array"));
        }

        auto jsHeaderExtensions = js.Get(kAttributeNameHeaderExtensions);
        if (!jsHeaderExtensions.IsArray()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The headerExtensions is not string"));
        }

        auto jsRtcp = js.Get(kAttributeNameRtcp);
        if (!jsRtcp.IsObject()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The rtcp is not object"));
        }

        auto jsCodecsArray = jsCodecs.As<Array>();
        for (uint32_t i = 0; i < jsCodecsArray.Length(); i++) {
            Value jsCodec = jsCodecsArray[i];
            if (!jsCodec.IsObject()) {
                NAPI_THROW_VOID(TypeError::New(js.Env(), "The element of codecs is not object"));
            }

            RtpCodecParameters parameters;
            NapiRtpCodecParameters::JsToNative(jsCodec.As<Object>(), parameters);
            if (js.Env().IsExceptionPending()) {
                NAPI_THROW_VOID(js.Env().GetAndClearPendingException());
            }
            native.codecs.push_back(parameters);
        }

        auto jsHeaderExtensionsArray = jsHeaderExtensions.As<Array>();
        for (uint32_t i = 0; i < jsHeaderExtensionsArray.Length(); i++) {
            Value jsHeaderExtension = jsHeaderExtensionsArray[i];
            if (!jsHeaderExtension.IsObject()) {
                NAPI_THROW_VOID(TypeError::New(js.Env(), "The element of headerExtensions is not object"));
            }

            RtpExtension headerExtension;
            NapiRtpHeaderExtensionParameters::JsToNative(jsHeaderExtension.As<Object>(), headerExtension);
            if (js.Env().IsExceptionPending()) {
                NAPI_THROW_VOID(js.Env().GetAndClearPendingException());
            }
            native.header_extensions.push_back(headerExtension);
        }

        NapiRtcpParameters::JsToNative(jsRtcp.As<Object>(), native.rtcp);
    }

    static void NativeToJs(const RtpParameters& native, Napi::Object& js)
    {
        auto jsCodecsArray = Array::New(js.Env(), native.codecs.size());
        for (uint32_t i = 0; i < jsCodecsArray.Length(); i++) {
            auto jsCodec = Object::New(js.Env());
            NapiRtpCodecParameters::NativeToJs(native.codecs[i], jsCodec);
            jsCodecsArray[i] = jsCodec;
        }
        js.Set(kAttributeNameCodecs, jsCodecsArray);

        auto jsHeaderExtensionsArray = Array::New(js.Env(), native.header_extensions.size());
        for (uint32_t i = 0; i < jsHeaderExtensionsArray.Length(); i++) {
            auto jsHeaderExtension = Object::New(js.Env());
            NapiRtpHeaderExtensionParameters::NativeToJs(native.header_extensions[i], jsHeaderExtension);
            jsHeaderExtensionsArray[i] = jsHeaderExtension;
        }
        js.Set(kAttributeNameHeaderExtensions, jsHeaderExtensionsArray);

        auto jsRtcp = Object::New(js.Env());
        NapiRtcpParameters::NativeToJs(native.rtcp, jsRtcp);
        js.Set(kAttributeNameRtcp, jsRtcp);
    }
};

struct NapiRtpHeaderExtensionCapability {
    // uri: string;
    static void JsToNative(const Napi::Object& js, RtpHeaderExtensionCapability& native)
    {
        if (!js.Has(kAttributeNameUri)) {
            NAPI_THROW_VOID(Error::New(js.Env(), "No uri"));
        }

        auto jsUri = js.Get(kAttributeNameUri);
        if (!jsUri.IsString()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The uri is not string"));
        }

        native.uri = jsUri.As<String>().Utf8Value();
    }

    static void NativeToJs(const RtpHeaderExtensionCapability& native, Napi::Object& js)
    {
        js.Set(kAttributeNameUri, String::New(js.Env(), native.uri));
    }
};

// encodings: RTCRtpEncodingParameters[];
// transactionId: string;
void NapiRtpSendParameters::JsToNative(const Napi::Object& js, RtpParameters& native)
{
    if (!js.Has(kAttributeNameTransactionId)) {
        NAPI_THROW_VOID(Error::New(js.Env(), "No transactionId"));
    }

    if (!js.Has(kAttributeNameEncodings)) {
        NAPI_THROW_VOID(Error::New(js.Env(), "No encodings"));
    }

    auto jsTransactionId = js.Get(kAttributeNameTransactionId);
    if (!jsTransactionId.IsString()) {
        NAPI_THROW_VOID(TypeError::New(js.Env(), "The transactionId is not string"));
    }

    auto jsEncodings = js.Get(kAttributeNameEncodings);
    if (!jsEncodings.IsArray()) {
        NAPI_THROW_VOID(TypeError::New(js.Env(), "The encodings is not array"));
    }

    native.transaction_id = jsTransactionId.As<String>().Utf8Value();

    auto jsEncodingsArray = jsEncodings.As<Array>();
    for (uint32_t i = 0; i < jsEncodingsArray.Length(); i++) {
        Value jsEncodingParameters = jsEncodingsArray[i];
        if (!jsEncodingParameters.IsObject()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The element of encodings is not object"));
        }

        RtpEncodingParameters rtpEncodingParameters;
        NapiRtpEncodingParameters::JsToNative(jsEncodingParameters.As<Object>(), rtpEncodingParameters);
        native.encodings.push_back(rtpEncodingParameters);
    }

    NapiRtpParameters::JsToNative(js, native);
    if (js.Env().IsExceptionPending()) {
        NAPI_THROW_VOID(js.Env().GetAndClearPendingException());
    }
}

void NapiRtpSendParameters::NativeToJs(const RtpParameters& native, Napi::Object& js)
{
    js.Set(kAttributeNameTransactionId, native.transaction_id);

    auto jsEncodingsArray = Array::New(js.Env(), native.encodings.size());
    for (uint32_t i = 0; i < jsEncodingsArray.Length(); i++) {
        auto jsEncoding = Object::New(js.Env());
        NapiRtpEncodingParameters::NativeToJs(native.encodings[i], jsEncoding);
        jsEncodingsArray[i] = jsEncoding;
    }
    js.Set(kAttributeNameEncodings, jsEncodingsArray);

    NapiRtpParameters::NativeToJs(native, js);
}

void NapiRtpReceiveParameters::JsToNative(const Napi::Object& js, RtpParameters& native)
{
    NapiRtpParameters::JsToNative(js, native);
    if (js.Env().IsExceptionPending()) {
        NAPI_THROW_VOID(js.Env().GetAndClearPendingException());
    }
}

void NapiRtpReceiveParameters::NativeToJs(const RtpParameters& native, Napi::Object& js)
{
    NapiRtpParameters::NativeToJs(native, js);
}

// codecs: RTCRtpCodecCapability[];
// headerExtensions: RTCRtpHeaderExtensionCapability[];
void NapiRtpCapabilities::JsToNative(const Napi::Object& js, RtpCapabilities& native)
{
    if (!js.Has(kAttributeNameCodecs)) {
        NAPI_THROW_VOID(Error::New(js.Env(), "No codecs"));
    }

    if (!js.Has(kAttributeNameHeaderExtensions)) {
        NAPI_THROW_VOID(Error::New(js.Env(), "No headerExtensions"));
    }

    auto jsCodecs = js.Get(kAttributeNameCodecs);
    if (!jsCodecs.IsArray()) {
        NAPI_THROW_VOID(TypeError::New(js.Env(), "The codecs is not array"));
    }

    auto jsHeaderExtensions = js.Get(kAttributeNameHeaderExtensions);
    if (!jsHeaderExtensions.IsArray()) {
        NAPI_THROW_VOID(TypeError::New(js.Env(), "The headerExtensions is not string"));
    }

    auto jsCodecsArray = jsCodecs.As<Array>();
    for (uint32_t i = 0; i < jsCodecsArray.Length(); i++) {
        Value jsCodec = jsCodecsArray[i];
        if (!jsCodec.IsObject()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The element of codecs is not object"));
        }

        RtpCodecCapability parameters;
        NapiRtpCodecCapability::JsToNative(jsCodec.As<Object>(), parameters);
        native.codecs.push_back(parameters);
    }

    auto jsHeaderExtensionsArray = jsHeaderExtensions.As<Array>();
    for (uint32_t i = 0; i < jsHeaderExtensionsArray.Length(); i++) {
        Value jsHeaderExtension = jsHeaderExtensionsArray[i];
        if (!jsHeaderExtension.IsObject()) {
            NAPI_THROW_VOID(TypeError::New(js.Env(), "The element of headerExtensions is not object"));
        }

        RtpHeaderExtensionCapability headerExtension;
        NapiRtpHeaderExtensionCapability::JsToNative(jsHeaderExtension.As<Object>(), headerExtension);
        if (js.Env().IsExceptionPending()) {
            NAPI_THROW_VOID(js.Env().GetAndClearPendingException());
        }
        native.header_extensions.push_back(headerExtension);
    }
}

void NapiRtpCapabilities::NativeToJs(const RtpCapabilities& native, Napi::Object& js)
{
    auto jsCodecsArray = Array::New(js.Env(), native.codecs.size());
    for (uint32_t i = 0; i < jsCodecsArray.Length(); i++) {
        auto jsCodec = Object::New(js.Env());
        NapiRtpCodecCapability::NativeToJs(native.codecs[i], jsCodec);
        jsCodecsArray[i] = jsCodec;
    }
    js.Set(kAttributeNameCodecs, jsCodecsArray);

    auto jsHeaderExtensionsArray = Array::New(js.Env(), native.header_extensions.size());
    for (uint32_t i = 0; i < jsHeaderExtensionsArray.Length(); i++) {
        auto jsHeaderExtension = Object::New(js.Env());
        NapiRtpHeaderExtensionCapability::NativeToJs(native.header_extensions[i], jsHeaderExtension);
        jsHeaderExtensionsArray[i] = jsHeaderExtension;
    }
    js.Set(kAttributeNameHeaderExtensions, jsHeaderExtensionsArray);
}

// mimeType: string;
// clockRate: number;
// channels?: number;
// sdpFmtpLine?: string;
void NapiRtpCodecCapability::JsToNative(const Napi::Object& js, RtpCodecCapability& native)
{
    if (!js.Has(kAttributeNameClockRate)) {
        NAPI_THROW_VOID(Error::New(js.Env(), "No clockRate"));
    }
    if (!js.Has(kAttributeNameMimeType)) {
        NAPI_THROW_VOID(Error::New(js.Env(), "No mimeType"));
    }

    auto jsClockRate = js.Get(kAttributeNameClockRate);
    if (!jsClockRate.IsNumber()) {
        NAPI_THROW_VOID(TypeError::New(js.Env(), "The clockRate is not number"));
    }

    auto jsMimeType = js.Get(kAttributeNameMimeType);
    if (!jsMimeType.IsString()) {
        NAPI_THROW_VOID(TypeError::New(js.Env(), "The mimeType is not string"));
    }

    native.clock_rate = jsClockRate.As<Number>().Int32Value();

    std::string kind;
    std::string name;
    if (!ParseMimeType(jsMimeType.As<String>().Utf8Value(), kind, name)) {
        NAPI_THROW_VOID(TypeError::New(js.Env(), "Invalid mimeType"));
    }
    native.kind = MediaTypeFromString(kind);
    native.name = name;

    if (js.Has(kAttributeNameSdpFmtpLine)) {
        auto jsSdpFmtpLine = js.Get(kAttributeNameSdpFmtpLine);
        if (jsSdpFmtpLine.IsString()) {
            ParseFmtpLine(jsSdpFmtpLine.As<String>().Utf8Value(), native.parameters);
        }
    }

    if (js.Has(kAttributeNameChannels)) {
        auto jsChannels = js.Get(kAttributeNameChannels);
        if (jsChannels.IsNumber()) {
            native.num_channels = jsChannels.As<Number>().Int32Value();
        }
    }
}

void NapiRtpCodecCapability::NativeToJs(const RtpCodecCapability& native, Napi::Object& js)
{
    js.Set(kAttributeNameMimeType, String::New(js.Env(), native.mime_type()));

    if (native.clock_rate) {
        js.Set(kAttributeNameClockRate, Number::New(js.Env(), native.clock_rate.value()));
    } else {
        // unset
        js.Set(kAttributeNameClockRate, Number::New(js.Env(), -1));
    }

    if (native.num_channels) {
        js.Set(kAttributeNameChannels, Number::New(js.Env(), native.num_channels.value()));
    }

    std::ostringstream ss;
    WriteFmtpParameters(native.parameters, ss);
    js.Set(kAttributeNameSdpFmtpLine, String::New(js.Env(), ss.str()));
}

// active?: boolean;
// maxBitrate?: number;
// maxFramerate?: number;
// scaleResolutionDownBy?: number;
void NapiRtpEncodingParameters::JsToNative(const Napi::Object& js, RtpEncodingParameters& native)
{
    if (js.Has(kAttributeNameActive)) {
        auto jsActive = js.Get(kAttributeNameActive);
        if (jsActive.IsBoolean()) {
            native.active = jsActive.As<Boolean>().Value();
        }
    }

    if (js.Has(kAttributeNameMaxBitrate)) {
        auto jsMaxBitrate = js.Get(kAttributeNameMaxBitrate);
        if (jsMaxBitrate.IsNumber()) {
            native.max_bitrate_bps = jsMaxBitrate.As<Number>().Int32Value();
        }
    }

    if (js.Has(kAttributeNameMaxFramerate)) {
        auto jsMaxFramerate = js.Get(kAttributeNameMaxFramerate);
        if (jsMaxFramerate.IsNumber()) {
            native.max_framerate = jsMaxFramerate.As<Number>().DoubleValue();
        }
    }

    if (js.Has(kAttributeNameScaleResolutionDownBy)) {
        auto jsScaleResolutionDownBy = js.Get(kAttributeNameScaleResolutionDownBy);
        if (jsScaleResolutionDownBy.IsNumber()) {
            native.scale_resolution_down_by = jsScaleResolutionDownBy.As<Number>().DoubleValue();
        }
    }
    if (js.Has(kAttributeNameSsrc)) {
        auto jsSsrc = js.Get(kAttributeNameSsrc);
        if (jsSsrc.IsNumber()) {
            native.ssrc = jsSsrc.As<Number>().Uint32Value();
        }
    }

    NapiRtpCodingParameters::JsToNative(js, native);
}

void NapiRtpEncodingParameters::NativeToJs(const RtpEncodingParameters& native, Napi::Object& js)
{
    js.Set(kAttributeNameActive, Boolean::New(js.Env(), native.active));
    if (native.max_bitrate_bps) {
        js.Set(kAttributeNameMaxBitrate, Number::New(js.Env(), native.max_bitrate_bps.value()));
    }
    if (native.max_framerate) {
        js.Set(kAttributeNameMaxFramerate, Number::New(js.Env(), native.max_framerate.value()));
    }
    if (native.scale_resolution_down_by) {
        js.Set(kAttributeNameScaleResolutionDownBy, Number::New(js.Env(), native.scale_resolution_down_by.value()));
    }
    if (native.ssrc) {
        js.Set(kAttributeNameSsrc, Number::New(js.Env(), native.ssrc.value()));
    }

    NapiRtpCodingParameters::NativeToJs(native, js);
}

} // namespace webrtc
