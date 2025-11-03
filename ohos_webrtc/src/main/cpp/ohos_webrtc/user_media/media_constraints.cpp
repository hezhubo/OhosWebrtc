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

#include "media_constraints.h"

#include <map>
#include <sstream>
#include <vector>

#include "rtc_base/strings/string_builder.h"
#include "rtc_base/logging.h"

namespace webrtc {

template <typename T>
void MaybeEmitNamedValue(rtc::StringBuilder& builder, const char* name, std::optional<T> value)
{
    if (!value) {
        return;
    }
    if (builder.size() > 1) {
        builder << ", ";
    }
    builder << name;
    builder << ": ";
    builder << *value;
}

class MediaTrackConstraintsPrivate final {
public:
    static std::shared_ptr<MediaTrackConstraintsPrivate> Create();
    static std::shared_ptr<MediaTrackConstraintsPrivate>
    Create(const MediaTrackConstraintSet& basic, const std::vector<MediaTrackConstraintSet>& advanced);

    MediaTrackConstraintsPrivate(
        const MediaTrackConstraintSet& basic, const std::vector<MediaTrackConstraintSet>& advanced);

    bool IsConstrained() const;
    const MediaTrackConstraintSet& Basic() const;
    MediaTrackConstraintSet& MutableBasic();
    const std::vector<MediaTrackConstraintSet>& Advanced() const;
    const std::string ToString() const;

private:
    MediaTrackConstraintSet basic_;
    std::vector<MediaTrackConstraintSet> advanced_;
};

BaseConstraint::BaseConstraint(const char* name) : name_(name) {}

BaseConstraint::~BaseConstraint() = default;

bool BaseConstraint::HasMandatory() const
{
    return HasMin() || HasMax() || HasExact();
}

LongConstraint::LongConstraint(const char* name) : BaseConstraint(name) {}

bool LongConstraint::IsConstrained() const
{
    return min_.has_value() || max_.has_value() || exact_.has_value() || ideal_.has_value();
}

bool LongConstraint::Matches(int32_t value) const
{
    if (min_.has_value() && value < *min_) {
        return false;
    }
    if (max_.has_value() && value > *max_) {
        return false;
    }
    if (exact_.has_value() && value != *exact_) {
        return false;
    }
    return true;
}

void LongConstraint::Reset()
{
    *this = LongConstraint(GetName());
}

std::string LongConstraint::ToString() const
{
    rtc::StringBuilder builder;

    builder << "{";
    MaybeEmitNamedValue(builder, "min", min_);
    MaybeEmitNamedValue(builder, "max", max_);
    MaybeEmitNamedValue(builder, "exact", exact_);
    MaybeEmitNamedValue(builder, "ideal", ideal_);
    builder << "}";

    return builder.str();
}

const double DoubleConstraint::kConstraintEpsilon = 0.00001;

DoubleConstraint::DoubleConstraint(const char* name) : BaseConstraint(name) {}

bool DoubleConstraint::IsConstrained() const
{
    return min_.has_value() || max_.has_value() || exact_.has_value() || ideal_.has_value();
}

void DoubleConstraint::Reset()
{
    *this = DoubleConstraint(GetName());
}

std::string DoubleConstraint::ToString() const
{
    rtc::StringBuilder builder;

    builder << "{";
    MaybeEmitNamedValue(builder, "min", min_);
    MaybeEmitNamedValue(builder, "max", max_);
    MaybeEmitNamedValue(builder, "exact", exact_);
    MaybeEmitNamedValue(builder, "ideal", ideal_);
    builder << "}";

    return builder.str();
}

bool DoubleConstraint::Matches(int32_t value) const
{
    if (min_.has_value() && value < *min_ - kConstraintEpsilon) {
        return false;
    }
    if (max_.has_value() && value > *max_ + kConstraintEpsilon) {
        return false;
    }
    if (exact_.has_value() && fabs(static_cast<double>(value) - *exact_) > kConstraintEpsilon) {
        return false;
    }
    return true;
}

StringConstraint::StringConstraint(const char* name) : BaseConstraint(name) {}

bool StringConstraint::IsConstrained() const
{
    return !exact_.empty() || !ideal_.empty();
}

void StringConstraint::Reset()
{
    *this = StringConstraint(GetName());
}

std::string StringConstraint::ToString() const
{
    rtc::StringBuilder builder;
    builder << "{";
    if (!ideal_.empty()) {
        builder << "ideal: [";
        bool first = true;
        for (const auto& iter : ideal_) {
            if (!first) {
                builder << ", ";
            }
            builder << "\"";
            builder << iter;
            builder << "\"";
            first = false;
        }
        builder << "]";
    }
    if (!exact_.empty()) {
        if (builder.size() > 1) {
            builder << ", ";
        }
        builder << "exact: [";
        bool first = true;
        for (const auto& iter : exact_) {
            if (!first) {
                builder << ", ";
            }
            builder << "\"";
            builder << iter;
            builder << "\"";
        }
        builder << "]";
    }
    builder << "}";

    return builder.str();
}

bool StringConstraint::Matches(std::string value) const
{
    if (exact_.empty()) {
        return true;
    }
    for (const auto& choice : exact_) {
        if (value == choice) {
            return true;
        }
    }
    return false;
}

BooleanConstraint::BooleanConstraint(const char* name) : BaseConstraint(name) {}

bool BooleanConstraint::IsConstrained() const
{
    return ideal_.has_value() || exact_.has_value();
}

bool BooleanConstraint::Matches(bool value) const
{
    if (exact_ && *exact_ != value) {
        return false;
    }
    return true;
}

void BooleanConstraint::Reset()
{
    *this = BooleanConstraint(GetName());
}

std::string BooleanConstraint::ToString() const
{
    rtc::StringBuilder builder;
    builder << "{";
    if (exact_) {
        builder << "exact: " << (*exact_ ? "true" : "false");
    }

    if (ideal_) {
        if (exact_) {
            builder << ", ";
        }
        builder << "ideal: " << (*ideal_ ? "true" : "false");
    }
    builder << "}";

    return builder.str();
}

MediaTrackConstraintSet::MediaTrackConstraintSet()
    : width("width"),
      height("height"),
      aspectRatio("aspectRatio"),
      frameRate("frameRate"),
      facingMode("facingMode"),
      resizeMode("resizeMode"),
      sampleRate("sampleRate"),
      sampleSize("sampleSize"),
      echoCancellation("echoCancellation"),
      autoGainControl("autoGainControl"),
      noiseSuppression("noiseSuppression"),
      latency("latency"),
      channelCount("channelCount"),
      deviceId("deviceId"),
      groupId("groupId"),
      backgroundBlur("backgroundBlur"),
      displaySurface("displaySurface"),
      googEchoCancellation("googEchoCancellation"),
      googAutoGainControl("autoGainControl"),
      googNoiseSuppression("noiseSuppression"),
      googHighpassFilter("googHighpassFilter"),
      googAudioMirroring("googAudioMirroring"),
      ohosScreenCaptureMode("ohosScreenCaptureMode"),
      ohosScreenCaptureDisplayId("ohosScreenCaptureDisplayId"),
      ohosScreenCaptureMissionId("ohosScreenCaptureMissionId"),
      ohosScreenCaptureWindowFilter("ohosScreenCaptureWindowFilter"),
      ohosScreenCaptureAudioFilter("ohosScreenCaptureAudioFilter"),
      ohosScreenCaptureSkipPrivacyMode("ohosScreenCaptureSkipPrivacyMode"),
      ohosScreenCaptureAutoRotation("ohosScreenCaptureAutoRotation")
{
}

bool MediaTrackConstraintSet::IsConstrained() const
{
    for (auto* const constraint : AllConstraints()) {
        if (constraint->IsConstrained()) {
            return true;
        }
    }
    return false;
}

std::vector<const BaseConstraint*> MediaTrackConstraintSet::AllConstraints() const
{
    return {
        &width,
        &height,
        &aspectRatio,
        &frameRate,
        &facingMode,
        &resizeMode,
        &sampleRate,
        &sampleSize,
        &echoCancellation,
        &autoGainControl,
        &noiseSuppression,
        &latency,
        &channelCount,
        &deviceId,
        &groupId,
        &googEchoCancellation,
        &googAutoGainControl,
        &googNoiseSuppression,
        &googHighpassFilter,
        &googAudioMirroring,
        &ohosScreenCaptureMode,
        &ohosScreenCaptureDisplayId,
        &ohosScreenCaptureMissionId,
        &ohosScreenCaptureWindowFilter,
        &ohosScreenCaptureAudioFilter,
        &ohosScreenCaptureSkipPrivacyMode,
        &ohosScreenCaptureAutoRotation,
    };
}

bool MediaTrackConstraintSet::HasMin() const
{
    for (auto* const constraint : AllConstraints()) {
        if (constraint->HasMin()) {
            return true;
        }
    }
    return false;
}

bool MediaTrackConstraintSet::HasExact() const
{
    for (auto* const constraint : AllConstraints()) {
        if (constraint->HasExact()) {
            return true;
        }
    }
    return false;
}

std::string MediaTrackConstraintSet::ToString() const
{
    rtc::StringBuilder builder;
    bool first = true;
    for (auto* const constraint : AllConstraints()) {
        if (constraint->IsConstrained()) {
            if (!first) {
                builder << ", ";
            }
            builder << constraint->GetName();
            builder << ": ";
            builder << constraint->ToString();
            first = false;
        }
    }

    return builder.str();
}

std::shared_ptr<MediaTrackConstraintsPrivate> MediaTrackConstraintsPrivate::Create()
{
    MediaTrackConstraintSet basic;
    std::vector<MediaTrackConstraintSet> advanced;
    return Create(basic, advanced);
}

std::shared_ptr<MediaTrackConstraintsPrivate> MediaTrackConstraintsPrivate::Create(
    const MediaTrackConstraintSet& basic, const std::vector<MediaTrackConstraintSet>& advanced)
{
    return std::make_shared<MediaTrackConstraintsPrivate>(basic, advanced);
}

MediaTrackConstraintsPrivate::MediaTrackConstraintsPrivate(
    const MediaTrackConstraintSet& basic, const std::vector<MediaTrackConstraintSet>& advanced)
    : basic_(basic), advanced_(advanced)
{
}

bool MediaTrackConstraintsPrivate::IsConstrained() const
{
    return basic_.IsConstrained() || !advanced_.empty();
}

const MediaTrackConstraintSet& MediaTrackConstraintsPrivate::Basic() const
{
    return basic_;
}

MediaTrackConstraintSet& MediaTrackConstraintsPrivate::MutableBasic()
{
    return basic_;
}

const std::vector<MediaTrackConstraintSet>& MediaTrackConstraintsPrivate::Advanced() const
{
    return advanced_;
}

const std::string MediaTrackConstraintsPrivate::ToString() const
{
    rtc::StringBuilder builder;
    if (IsConstrained()) {
        builder << "{";
        builder << Basic().ToString();
        if (!Advanced().empty()) {
            if (builder.size() > 1) {
                builder << ", ";
            }
            builder << "advanced: [";
            bool first = true;
            for (const auto& constraint_set : Advanced()) {
                if (!first) {
                    builder << ", ";
                }
                builder << "{";
                builder << constraint_set.ToString();
                builder << "}";
                first = false;
            }
            builder << "]";
        }
        builder << "}";
    }

    return builder.str();
}

void MediaTrackConstraints::Assign(const MediaTrackConstraints& other)
{
    private_ = other.private_;
}

MediaTrackConstraints::MediaTrackConstraints() = default;

MediaTrackConstraints::MediaTrackConstraints(const MediaTrackConstraints& other)
{
    Assign(other);
}

void MediaTrackConstraints::Reset()
{
    private_.reset();
}

bool MediaTrackConstraints::IsConstrained() const
{
    return private_ && private_->IsConstrained();
}

void MediaTrackConstraints::Initialize()
{
    RTC_DCHECK(IsNull());
    private_ = MediaTrackConstraintsPrivate::Create();
}

void MediaTrackConstraints::Initialize(
    const MediaTrackConstraintSet& basic, const std::vector<MediaTrackConstraintSet>& advanced)
{
    RTC_DCHECK(IsNull());
    private_ = MediaTrackConstraintsPrivate::Create(basic, advanced);
}

const MediaTrackConstraintSet& MediaTrackConstraints::Basic() const
{
    RTC_DCHECK(!IsNull());
    return private_->Basic();
}

MediaTrackConstraintSet& MediaTrackConstraints::MutableBasic()
{
    RTC_DCHECK(!IsNull());
    return private_->MutableBasic();
}

const std::vector<MediaTrackConstraintSet>& MediaTrackConstraints::Advanced() const
{
    RTC_DCHECK(!IsNull());
    return private_->Advanced();
}

const std::string MediaTrackConstraints::ToString() const
{
    if (IsNull()) {
        return std::string("");
    }
    return private_->ToString();
}

} // namespace webrtc
