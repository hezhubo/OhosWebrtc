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

#include "media_track_constraints.h"

#include <map>
#include <sstream>
#include <vector>

#include "rtc_base/strings/string_builder.h"
#include "rtc_base/logging.h"

namespace webrtc {

using namespace Napi;

// A naked value is treated as an "ideal" value in the basic constraints,
// but as an exact value in "advanced" constraints.
// https://www.w3.org/TR/mediacapture-streams/#constrainable-interface
enum class NakedValueDisposition {
    kTreatAsIdeal,
    kTreatAsExact
};

const size_t kMaxConstraintStringLength = 500;
const size_t kMaxConstraintStringSeqLength = 100;

const char kConstraintsMin[] = "min";
const char kConstraintsMax[] = "max";
const char kConstraintsExact[] = "exact";
const char kConstraintsIdeal[] = "ideal";
const char kConstraintsAdvanced[] = "advanced";

const std::map<std::string, bool> SUPPORTED_CONSTRAINTS_MAP = {
    {NapiMediaConstraints::kAttributeNameWidth, true},
    {NapiMediaConstraints::kAttributeNameHeight, true},
    {NapiMediaConstraints::kAttributeNameAspectRatio, true},
    {NapiMediaConstraints::kAttributeNameFrameRate, true},
    {NapiMediaConstraints::kAttributeNameFacingMode, true},
    {NapiMediaConstraints::kAttributeNameResizeMode, false},
    {NapiMediaConstraints::kAttributeNameSampleRate, false},
    {NapiMediaConstraints::kAttributeNameSampleSize, false},
    {NapiMediaConstraints::kAttributeNameEchoCancellation, true},
    {NapiMediaConstraints::kAttributeNameAutoGainControl, true},
    {NapiMediaConstraints::kAttributeNameNoiseSuppression, true},
    {NapiMediaConstraints::kAttributeNameLatency, false},
    {NapiMediaConstraints::kAttributeNameChannelCount, false},
    {NapiMediaConstraints::kAttributeNameDeviceId, true},
    {NapiMediaConstraints::kAttributeNameGroupId, true},
    {NapiMediaConstraints::kAttributeNameDisplaySurface, false},
    {NapiMediaConstraints::kAttributeNameBackgroundBlur, false},
    {NapiMediaConstraints::kAttributeNameGoogEchoCancellation, false},
    {NapiMediaConstraints::kAttributeNameGoogAutoGainControl, false},
    {NapiMediaConstraints::kAttributeNameGoogNoiseSuppression, false},
    {NapiMediaConstraints::kAttributeNameGoogHighpassFilter, false},
    {NapiMediaConstraints::kAttributeNameGoogAudioMirroring, false},
    {NapiMediaConstraints::kAttributeNameOhosScreenCaptureMode, true},
    {NapiMediaConstraints::kAttributeNameOhosScreenCaptureDisplayId, true},
    {NapiMediaConstraints::kAttributeNameOhosScreenCaptureMissionId, true},
    {NapiMediaConstraints::kAttributeNameOhosScreenCaptureAudioFilter, true},
    {NapiMediaConstraints::kAttributeNameOhosScreenCaptureWindowFilter, true},
    {NapiMediaConstraints::kAttributeNameOhosScreenCaptureSkipPrivacyMode, true},
    {NapiMediaConstraints::kAttributeNameOhosScreenCaptureAutoRotation, true},
};

bool IsConstraintSupported(const std::string& name)
{
    auto it = SUPPORTED_CONSTRAINTS_MAP.find(name);
    if (it != SUPPORTED_CONSTRAINTS_MAP.end()) {
        return it->second;
    }
    return true;
}

void CopyLongConstraint(const Napi::Value& jsValue, NakedValueDisposition nakedTreatment, LongConstraint& nativeValue)
{
    if (jsValue.IsNumber()) {
        switch (nakedTreatment) {
            case NakedValueDisposition::kTreatAsIdeal:
                nativeValue.SetIdeal(jsValue.As<Number>().Int32Value());
                break;
            case NakedValueDisposition::kTreatAsExact:
                nativeValue.SetExact(jsValue.As<Number>().Int32Value());
                break;
        }
    } else if (jsValue.IsObject()) {
        auto jsConstrainULongRange = jsValue.As<Object>();
        if (jsConstrainULongRange.Has(kConstraintsMin)) {
            nativeValue.SetMin(jsConstrainULongRange.Get(kConstraintsMin).As<Number>().Int32Value());
        }
        if (jsConstrainULongRange.Has(kConstraintsMax)) {
            nativeValue.SetMax(jsConstrainULongRange.Get(kConstraintsMax).As<Number>().Int32Value());
        }
        if (jsConstrainULongRange.Has(kConstraintsIdeal)) {
            nativeValue.SetIdeal(jsConstrainULongRange.Get(kConstraintsIdeal).As<Number>().Int32Value());
        }
        if (jsConstrainULongRange.Has(kConstraintsExact)) {
            nativeValue.SetExact(jsConstrainULongRange.Get(kConstraintsExact).As<Number>().Int32Value());
        }
    }
}

void CopyDoubleConstraint(
    const Napi::Value& jsValue, NakedValueDisposition nakedTreatment, DoubleConstraint& nativeValue)
{
    if (jsValue.IsNumber()) {
        switch (nakedTreatment) {
            case NakedValueDisposition::kTreatAsIdeal:
                nativeValue.SetIdeal(jsValue.As<Number>().DoubleValue());
                break;
            case NakedValueDisposition::kTreatAsExact:
                nativeValue.SetExact(jsValue.As<Number>().DoubleValue());
                break;
        }
    } else if (jsValue.IsObject()) {
        auto jsConstrainDoubleRange = jsValue.As<Object>();
        if (jsConstrainDoubleRange.Has(kConstraintsMin)) {
            nativeValue.SetMin(jsConstrainDoubleRange.Get(kConstraintsMin).As<Number>().DoubleValue());
        }
        if (jsConstrainDoubleRange.Has(kConstraintsMax)) {
            nativeValue.SetMax(jsConstrainDoubleRange.Get(kConstraintsMax).As<Number>().DoubleValue());
        }
        if (jsConstrainDoubleRange.Has(kConstraintsIdeal)) {
            nativeValue.SetIdeal(jsConstrainDoubleRange.Get(kConstraintsIdeal).As<Number>().DoubleValue());
        }
        if (jsConstrainDoubleRange.Has(kConstraintsExact)) {
            nativeValue.SetExact(jsConstrainDoubleRange.Get(kConstraintsExact).As<Number>().DoubleValue());
        }
    }
}

bool ValidateString(const std::string& str, std::string& errorMessage)
{
    if (str.length() > kMaxConstraintStringLength) {
        errorMessage = "Constraint string too long.";
        return false;
    }
    return true;
}

bool ValidateStringSeq(const std::vector<std::string>& strs, std::string& errorMessage)
{
    if (strs.size() > kMaxConstraintStringSeqLength) {
        errorMessage = "Constraint string sequence too long.";
        return false;
    }

    for (const std::string& str : strs) {
        if (!ValidateString(str, errorMessage)) {
            return false;
        }
    }

    return true;
}

bool ValidateStringConstraint(const Value& jsValue, std::string& errorMessage)
{
    if (jsValue.IsString()) {
        return ValidateString(jsValue.As<String>().Utf8Value(), errorMessage);
    } else if (jsValue.IsArray()) {
        auto jsArray = jsValue.As<Array>();
        if (jsArray.Length() > kMaxConstraintStringSeqLength) {
            errorMessage = "Constraint string sequence too long.";
            return false;
        }

        for (uint32_t index = 0; index < jsArray.Length(); index++) {
            if (!ValidateString(jsArray.Get(index).As<String>().Utf8Value(), errorMessage)) {
                return false;
            }
        }
        return true;
    } else if (jsValue.IsObject()) {
        auto jsObject = jsValue.As<Object>();
        if (jsObject.Has(kConstraintsIdeal) &&
            !ValidateStringConstraint(jsObject.Get(kConstraintsIdeal).As<String>(), errorMessage))
        {
            return false;
        }
        if (jsObject.Has(kConstraintsExact) &&
            !ValidateStringConstraint(jsObject.Get(kConstraintsExact).As<String>(), errorMessage))
        {
            return false;
        }
        return true;
    }

    return false;
}

bool ValidateAndCopyStringConstraint(
    const Value& jsValue, NakedValueDisposition nakedTreatment, StringConstraint& nativeValue,
    std::string& errorMessage)
{
    if (!ValidateStringConstraint(jsValue, errorMessage)) {
        return false;
    }

    if (jsValue.IsString()) {
        switch (nakedTreatment) {
            case NakedValueDisposition::kTreatAsIdeal:
                nativeValue.SetIdeal(std::vector<std::string>(1, jsValue.As<String>().Utf8Value()));
                break;
            case NakedValueDisposition::kTreatAsExact:
                nativeValue.SetExact(std::vector<std::string>(1, jsValue.As<String>().Utf8Value()));
                break;
        }
    } else if (jsValue.IsArray()) {
        std::vector<std::string> nativeArray;
        auto jsArray = jsValue.As<Array>();
        for (uint32_t index = 0; index < jsArray.Length(); index++) {
            nativeArray.push_back(((Napi::Value)jsArray[index]).As<String>().Utf8Value());
        }

        switch (nakedTreatment) {
            case NakedValueDisposition::kTreatAsIdeal:
                nativeValue.SetIdeal(nativeArray);
                break;
            case NakedValueDisposition::kTreatAsExact:
                nativeValue.SetExact(nativeArray);
                break;
        }
    } else if (jsValue.IsObject()) {
        auto jsObject = jsValue.As<Object>();
        if (jsObject.Has(kConstraintsIdeal)) {
            auto jsIdeal = jsObject.Get(kConstraintsIdeal);
            if (jsIdeal.IsString()) {
                nativeValue.SetIdeal(std::vector<std::string>(1, jsValue.As<String>().Utf8Value()));
            } else if (jsIdeal.IsArray()) {
                std::vector<std::string> nativeArray;
                auto jsArray = jsValue.As<Array>();
                for (uint32_t index = 0; index < jsArray.Length(); index++) {
                    nativeArray.push_back(((Napi::Value)jsArray[index]).As<String>().Utf8Value());
                }
                nativeValue.SetIdeal(nativeArray);
            }
        }
        if (jsObject.Has(kConstraintsExact)) {
            auto jsIdeal = jsObject.Get(kConstraintsExact);
            if (jsIdeal.IsString()) {
                nativeValue.SetExact(std::vector<std::string>(1, jsValue.As<String>().Utf8Value()));
            } else if (jsIdeal.IsArray()) {
                std::vector<std::string> nativeArray;
                auto jsArray = jsValue.As<Array>();
                for (uint32_t index = 0; index < jsArray.Length(); index++) {
                    nativeArray.push_back(((Napi::Value)jsArray[index]).As<String>().Utf8Value());
                }
                nativeValue.SetExact(nativeArray);
            }
        }
    }
    return true;
}

void CopyBooleanConstraint(
    const Napi::Value& jsValue, NakedValueDisposition nakedTreatment, BooleanConstraint& nativeValue)
{
    if (jsValue.IsBoolean()) {
        switch (nakedTreatment) {
            case NakedValueDisposition::kTreatAsIdeal:
                nativeValue.SetIdeal(jsValue.As<Boolean>());
                break;
            case NakedValueDisposition::kTreatAsExact:
                nativeValue.SetExact(jsValue.As<Boolean>());
                break;
        }
    } else if (jsValue.IsObject()) {
        auto jsObject = jsValue.As<Object>();
        if (jsObject.Has(kConstraintsIdeal)) {
            nativeValue.SetIdeal(jsObject.Get(kConstraintsIdeal).As<Boolean>());
        }
        if (jsObject.Has(kConstraintsExact)) {
            nativeValue.SetIdeal(jsObject.Get(kConstraintsExact).As<Boolean>());
        }
    }
}

void ValidateAndCopyConstraint(
    const Object& jsTrackConstraints, NakedValueDisposition nakedTreatment, BooleanConstraint& constraint)
{
    if (jsTrackConstraints.Has(constraint.GetName()) && IsConstraintSupported(constraint.GetName())) {
        CopyBooleanConstraint(jsTrackConstraints.Get(constraint.GetName()), nakedTreatment, constraint);
    }
}

void ValidateAndCopyConstraint(
    const Object& jsTrackConstraints, NakedValueDisposition nakedTreatment, LongConstraint& constraint)
{
    if (jsTrackConstraints.Has(constraint.GetName()) && IsConstraintSupported(constraint.GetName())) {
        CopyLongConstraint(jsTrackConstraints.Get(constraint.GetName()), nakedTreatment, constraint);
    }
}

void ValidateAndCopyConstraint(
    const Object& jsTrackConstraints, NakedValueDisposition nakedTreatment, DoubleConstraint& constraint)
{
    if (jsTrackConstraints.Has(constraint.GetName()) && IsConstraintSupported(constraint.GetName())) {
        CopyDoubleConstraint(jsTrackConstraints.Get(constraint.GetName()), nakedTreatment, constraint);
    }
}

bool ValidateAndCopyConstraint(
    const Object& jsTrackConstraints, NakedValueDisposition nakedTreatment, StringConstraint& constraint,
    std::string& errorMessage)
{
    if (jsTrackConstraints.Has(constraint.GetName()) && IsConstraintSupported(constraint.GetName())) {
        if (!ValidateAndCopyStringConstraint(
                jsTrackConstraints.Get(constraint.GetName()), nakedTreatment, constraint, errorMessage))
        {
            return false;
        }
    }

    return true;
}

bool ValidateAndCopyConstraintSetExtension(
    const Object& jsTrackConstraints, NakedValueDisposition nakedTreatment, MediaTrackConstraintSet& trackConstraints,
    std::string& errorMessage)
{
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.ohosScreenCaptureDisplayId);

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment,
                                   trackConstraints.ohosScreenCaptureMode, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment,
                                   trackConstraints.ohosScreenCaptureMissionId, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment,
                                   trackConstraints.ohosScreenCaptureWindowFilter, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment,
                                   trackConstraints.ohosScreenCaptureAudioFilter, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment,
                                   trackConstraints.ohosScreenCaptureSkipPrivacyMode, errorMessage)) {
        return false;
    }

    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.ohosScreenCaptureAutoRotation);

    return true;
}

bool ValidateAndCopyConstraintSet(
    const Object& jsTrackConstraints, NakedValueDisposition nakedTreatment, MediaTrackConstraintSet& trackConstraints,
    std::string& errorMessage)
{
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.width);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.height);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.aspectRatio);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.frameRate);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.sampleRate);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.sampleSize);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.echoCancellation);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.autoGainControl);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.noiseSuppression);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.latency);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.channelCount);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.backgroundBlur);
    ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.ohosScreenCaptureDisplayId);

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.facingMode, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.resizeMode, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.deviceId, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.groupId, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraint(jsTrackConstraints, nakedTreatment, trackConstraints.displaySurface, errorMessage)) {
        return false;
    }

    if (!ValidateAndCopyConstraintSetExtension(jsTrackConstraints, nakedTreatment, trackConstraints, errorMessage)) {
        return false;
    }

    return true;
}

MediaTrackConstraints ParseTrackConstraints(const Napi::Object& jsTrackConstraints, std::string& errorMessage)
{
    MediaTrackConstraintSet basic;
    if (!ValidateAndCopyConstraintSet(jsTrackConstraints, NakedValueDisposition::kTreatAsIdeal, basic, errorMessage)) {
        RTC_LOG(LS_ERROR) << "Failed to parse track constraints: " << errorMessage;
        return MediaTrackConstraints();
    }

    std::vector<MediaTrackConstraintSet> advanced;
    if (jsTrackConstraints.Has(kConstraintsAdvanced)) {
        MediaTrackConstraintSet advancedElement;
        auto jsArray = jsTrackConstraints.Get(kConstraintsAdvanced).As<Array>();
        for (uint32_t index = 0; index < jsArray.Length(); index++) {
            auto jsElement = (Napi::Value)jsArray[index];
            if (!ValidateAndCopyConstraintSet(
                    jsElement.As<Object>(), NakedValueDisposition::kTreatAsExact, advancedElement, errorMessage))
            {
                RTC_LOG(LS_ERROR) << "Failed to parse track constraints: " << errorMessage;
                return MediaTrackConstraints();
            }
            advanced.push_back(advancedElement);
        }
    }

    MediaTrackConstraints constraints;
    constraints.Initialize(basic, advanced);

    return constraints;
}

std::vector<std::string> NapiMediaConstraints::GetSupportedConstraints()
{
    std::vector<std::string> result;
    for (const auto& constraints : SUPPORTED_CONSTRAINTS_MAP) {
        if (constraints.second) {
            result.push_back(constraints.first);
        }
    }
    return result;
}

bool NapiMediaConstraints::IsConstraintSupported(const std::string& name)
{
    return ::webrtc::IsConstraintSupported(name);
}

void NapiMediaConstraints::JsToNative(
    const Napi::Value& jsTrackConstraints, MediaTrackConstraints& nativeTrackConstraints)
{
    if (jsTrackConstraints.IsBoolean()) {
        if (jsTrackConstraints.As<Boolean>()) {
            MediaTrackConstraints constraints;
            constraints.Initialize();
            nativeTrackConstraints = constraints;
        } else {
            nativeTrackConstraints = MediaTrackConstraints();
        }
        return;
    } else if (jsTrackConstraints.IsObject()) {
        std::string errorMessage;
        auto constraints = ParseTrackConstraints(jsTrackConstraints.As<Object>(), errorMessage);
        if (constraints.IsNull()) {
            NAPI_THROW_VOID(TypeError::New(jsTrackConstraints.Env(), errorMessage));
        }
        nativeTrackConstraints = constraints;
        return;
    }

    nativeTrackConstraints = MediaTrackConstraints();
}

} // namespace webrtc
