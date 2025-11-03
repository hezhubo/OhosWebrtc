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

#include "media_constraints_util.h"

#include <vector>
#include <cmath>
#include <optional>
#include <algorithm>

#include "rtc_base/logging.h"
#include "rtc_base/strings/string_builder.h"

namespace webrtc {

// Number of default settings to be used as final tie-breaking criteria.
constexpr int kNumDefaultDistanceEntries = 3;

constexpr int kMaxDimension = std::numeric_limits<int>::max();
constexpr int kMaxFrameRate = 1000;

class CandidateSettings;

template <typename ConstraintType>
bool ConstraintHasMax(const ConstraintType& constraint)
{
    return constraint.HasMax() || constraint.HasExact();
}

template <typename ConstraintType>
bool ConstraintHasMin(const ConstraintType& constraint)
{
    return constraint.HasMin() || constraint.HasExact();
}

template <typename ConstraintType>
auto ConstraintMax(const ConstraintType& constraint) -> decltype(constraint.Max())
{
    RTC_DCHECK(ConstraintHasMax(constraint));
    return constraint.HasExact() ? constraint.Exact() : constraint.Max();
}

template <typename ConstraintType>
auto ConstraintMin(const ConstraintType& constraint) -> decltype(constraint.Min())
{
    RTC_DCHECK(ConstraintHasMin(constraint));
    return constraint.HasExact() ? constraint.Exact() : constraint.Min();
}

template <typename T>
class NumericRangeSet {
public:
    NumericRangeSet() = default;
    NumericRangeSet(std::optional<T> min, std::optional<T> max) : min_(std::move(min)), max_(std::move(max)) {}
    NumericRangeSet(const NumericRangeSet& other) = default;
    NumericRangeSet& operator=(const NumericRangeSet& other) = default;
    ~NumericRangeSet() = default;

    const std::optional<T>& Min() const
    {
        return min_;
    }
    const std::optional<T>& Max() const
    {
        return max_;
    }
    bool IsEmpty() const
    {
        return max_ && min_ && *max_ < *min_;
    }

    NumericRangeSet Intersection(const NumericRangeSet& other) const
    {
        std::optional<T> min = min_;
        if (other.Min())
            min = min ? std::max(*min, *other.Min()) : other.Min();

        std::optional<T> max = max_;
        if (other.Max())
            max = max ? std::min(*max, *other.Max()) : other.Max();

        return NumericRangeSet(min, max);
    }

    bool Contains(T value) const
    {
        return (!Min() || value >= *Min()) && (!Max() || value <= *Max());
    }

    template <typename ConstraintType>
    static NumericRangeSet<T> FromConstraint(ConstraintType constraint, T lowerBound, T upperBound)
    {
        RTC_DCHECK_LE(lowerBound, upperBound);
        if ((ConstraintHasMax(constraint) && ConstraintMax(constraint) < lowerBound) ||
            (ConstraintHasMin(constraint) && ConstraintMin(constraint) > upperBound))
        {
            return NumericRangeSet<decltype(constraint.Min())>(1, 0);
        }

        return NumericRangeSet<T>(
            ConstraintHasMin(constraint) && ConstraintMin(constraint) >= lowerBound ? ConstraintMin(constraint)
                                                                                    : std::optional<T>(),
            ConstraintHasMax(constraint) && ConstraintMax(constraint) <= upperBound ? ConstraintMax(constraint)
                                                                                    : std::optional<T>());
    }

    template <typename ConstraintType>
    static NumericRangeSet<T> FromConstraint(ConstraintType constraint)
    {
        return NumericRangeSet<T>(
            ConstraintHasMin(constraint) ? ConstraintMin(constraint) : std::optional<T>(),
            ConstraintHasMax(constraint) ? ConstraintMax(constraint) : std::optional<T>());
    }

    static NumericRangeSet<T> FromValue(T value)
    {
        return NumericRangeSet<T>(value, value);
    }

    static NumericRangeSet<T> EmptySet()
    {
        return NumericRangeSet(1, 0);
    }

private:
    std::optional<T> min_;
    std::optional<T> max_;
};

using DoubleRangeSet = NumericRangeSet<double>;
using IntRangeSet = NumericRangeSet<int32_t>;

double SquareEuclideanDistance(double x1, double y1, double x2, double y2)
{
    // return the dot product between (x1, y1) and (x2, y2)
    double x = x1 - x2;
    double y = y1 - y2;
    return x * x + y * y;
}

std::string FacingModeToString(FacingMode facingMode)
{
    switch (facingMode) {
        case FacingMode::kUser:
            return "user";
        case FacingMode::kEnvironment:
            return "environment";
        default:
            return "";
    }
}

void UpdateFailedConstraintName(const BaseConstraint& constraint, std::string* failedConstraintName)
{
    if (failedConstraintName) {
        *failedConstraintName = constraint.GetName();
    }
}

double NumericConstraintFitnessDistance(double value1, double value2)
{
    if (std::fabs(value1 - value2) <= DoubleConstraint::kConstraintEpsilon) {
        return 0.0;
    }

    return std::fabs(value1 - value2) / std::max(std::fabs(value1), std::fabs(value2));
}

template <typename NumericConstraint>
double NumericValueFitness(const NumericConstraint& constraint, decltype(constraint.Min()) value)
{
    return constraint.HasIdeal() ? NumericConstraintFitnessDistance(value, constraint.Ideal()) : 0.0;
}

template <typename NumericConstraint>
double
NumericRangeFitness(const NumericConstraint& constraint, decltype(constraint.Min()) min, decltype(constraint.Min()) max)
{
    if (constraint.HasIdeal()) {
        auto ideal = constraint.Ideal();
        if (ideal < min) {
            return NumericConstraintFitnessDistance(min, ideal);
        } else if (ideal > max) {
            return NumericConstraintFitnessDistance(max, ideal);
        }
    }

    return 0.0;
}

double StringConstraintFitnessDistance(const std::string& value, const StringConstraint& constraint)
{
    if (!constraint.HasIdeal()) {
        return 0.0;
    }

    for (auto& idealValue : constraint.Ideal()) {
        if (value == idealValue) {
            return 0.0;
        }
    }

    return 1.0;
}

class CandidateSettings {
public:
    CandidateSettings(std::string deviceId, std::string groupId, FacingMode facingMode, video::VideoProfile profile)
        : deviceId_(deviceId),
          groupId_(groupId),
          facingMode_(facingMode),
          profile_(profile),
          targetWidth_(profile.resolution.width),
          targetHeight_(profile.resolution.height),
          targetAspectRatio_(static_cast<double>(profile.resolution.width) / profile.resolution.height),
          targetFrameRate_({profile.frameRateRange.min, profile.frameRateRange.max})
    {
    }

    double Fitness(const MediaTrackConstraintSet& constraintSet)
    {
        return DeviceFitness(constraintSet) + ProfileFitness(constraintSet);
    }

    double DeviceFitness(const MediaTrackConstraintSet& constraintSet)
    {
        return StringConstraintFitnessDistance(deviceId_, constraintSet.deviceId) +
               StringConstraintFitnessDistance(groupId_, constraintSet.groupId) +
               StringConstraintFitnessDistance(FacingModeToString(facingMode_), constraintSet.facingMode);
    }

    double ProfileFitness(const MediaTrackConstraintSet& constraintSet)
    {
        return NumericValueFitness(constraintSet.width, targetWidth_) +
               NumericValueFitness(constraintSet.height, targetHeight_) +
               NumericValueFitness(constraintSet.aspectRatio, targetAspectRatio_) +
               NumericRangeFitness(constraintSet.frameRate, targetFrameRate_.min, targetFrameRate_.max);
    }

    bool ApplyConstraintSet(const MediaTrackConstraintSet& constraintSet, std::string* failedConstraintName = nullptr)
    {
        // resizeMode is not supported
        auto constrainedWidth = IntRangeSet::FromConstraint(constraintSet.width);
        if (!constrainedWidth.Contains(targetWidth_)) {
            UpdateFailedConstraintName(constraintSet.width, failedConstraintName);
            return false;
        }

        auto constrainedHeight = IntRangeSet::FromConstraint(constraintSet.height);
        if (!constrainedHeight.Contains(targetHeight_)) {
            UpdateFailedConstraintName(constraintSet.height, failedConstraintName);
            return false;
        }

        auto constrainedAspectRatio = DoubleRangeSet::FromConstraint(constraintSet.aspectRatio);
        if (!constrainedAspectRatio.Contains(targetAspectRatio_)) {
            UpdateFailedConstraintName(constraintSet.aspectRatio, failedConstraintName);
            return false;
        }

        auto constrainedFrameRate =
            DoubleRangeSet::FromConstraint(constraintSet.frameRate, MinFrameRate(), MaxFrameRate());
        if (constrainedFrameRate.IsEmpty()) {
            UpdateFailedConstraintName(constraintSet.frameRate, failedConstraintName);
            return false;
        }
        if (constrainedFrameRate.Min()) {
            profile_.frameRateRange.min = constrainedFrameRate.Min().value();
        }
        if (constrainedFrameRate.Max()) {
            profile_.frameRateRange.max = constrainedFrameRate.Max().value();
        }

        constrainedWidth_ =
            constrainedWidth_.Intersection(IntRangeSet::FromConstraint(constraintSet.width, 1L, kMaxDimension));
        constrainedHeight_ =
            constrainedHeight_.Intersection(IntRangeSet::FromConstraint(constraintSet.height, 1L, kMaxDimension));
        constrainedAspectRatio_ = constrainedAspectRatio_.Intersection(
            DoubleRangeSet::FromConstraint(constraintSet.aspectRatio, 0.0, HUGE_VAL));
        constrainedFrameRate_ = constrainedFrameRate_.Intersection(
            DoubleRangeSet::FromConstraint(constraintSet.frameRate, 0.0, kMaxFrameRate));

        return true;
    }

    bool SatisfiesFrameRateConstraint(const DoubleConstraint& constraint) const
    {
        double constraintMin = ConstraintHasMin(constraint) ? ConstraintMin(constraint) : -1.0;
        double constraintMax =
            ConstraintHasMax(constraint) ? ConstraintMax(constraint) : static_cast<double>(kMaxFrameRate);
        bool constraintMinOutOfRange = constraintMin > MaxFrameRate();
        bool constraintMaxOutOfRange = constraintMax < MinFrameRate();
        bool constraintSelfContradicts = constraintMin > constraintMax;

        return !constraintMinOutOfRange && !constraintMaxOutOfRange && !constraintSelfContradicts;
    }

    CameraCaptureSettings GetSetting()
    {
        CameraCaptureSettings setting;

        setting.deviceId = deviceId_;
        setting.profile.resolution.width = targetWidth_;
        setting.profile.resolution.height = targetHeight_;
        setting.profile.frameRateRange.min = targetFrameRate_.min;
        setting.profile.frameRateRange.max = targetFrameRate_.max;
        setting.profile.format = profile_.format;

        return setting;
    }

    int NativeWidth() const
    {
        return profile_.resolution.width;
    }
    int NativeHeight() const
    {
        return profile_.resolution.height;
    }
    double NativeAspectRatio() const
    {
        RTC_DCHECK(NativeWidth() > 0 || NativeHeight() > 0);
        return static_cast<double>(NativeWidth()) / NativeHeight();
    }
    video::FrameRateRange NativeFrameRateRange() const
    {
        return profile_.frameRateRange;
    }

    const std::optional<double>& MinFrameRateConstraint() const
    {
        return constrainedFrameRate_.Min();
    }
    const std::optional<double>& MaxFrameRateConstraint() const
    {
        return constrainedFrameRate_.Max();
    }

    double MaxFrameRate() const
    {
        if (MaxFrameRateConstraint()) {
            return std::min(*MaxFrameRateConstraint(), static_cast<double>(profile_.frameRateRange.max));
        }
        return static_cast<double>(profile_.frameRateRange.max);
    }

    double MinFrameRate() const
    {
        if (MinFrameRateConstraint()) {
            return std::max(*MinFrameRateConstraint(), static_cast<double>(profile_.frameRateRange.min));
        }
        return static_cast<double>(profile_.frameRateRange.min);
    }

private:
    std::string deviceId_;
    std::string groupId_;
    FacingMode facingMode_;
    video::VideoProfile profile_;

    uint32_t targetWidth_;
    uint32_t targetHeight_;
    double targetAspectRatio_;
    video::FrameRateRange targetFrameRate_;

    NumericRangeSet<int> constrainedWidth_;
    NumericRangeSet<int> constrainedHeight_;
    NumericRangeSet<double> constrainedAspectRatio_;
    NumericRangeSet<double> constrainedFrameRate_;
};

bool FacingModeSatisfiesConstraint(webrtc::FacingMode value, const webrtc::StringConstraint& constraint)
{
    std::string stringValue = FacingModeToString(value);
    if (stringValue.empty()) {
        return constraint.Exact().empty();
    }

    return constraint.Matches(stringValue);
}

bool DeviceSatisfiesConstraintSet(
    const CameraDeviceInfo& device, const MediaTrackConstraintSet& constraintSet, std::string* failedConstraintName)
{
    if (!constraintSet.deviceId.Matches(device.deviceId)) {
        UpdateFailedConstraintName(constraintSet.deviceId, failedConstraintName);
        return false;
    }

    if (!constraintSet.groupId.Matches(device.groupId)) {
        UpdateFailedConstraintName(constraintSet.groupId, failedConstraintName);
        return false;
    }

    if (!FacingModeSatisfiesConstraint(device.facingMode, constraintSet.facingMode)) {
        UpdateFailedConstraintName(constraintSet.facingMode, failedConstraintName);
        return false;
    }

    return true;
}

std::string CameraCaptureSettings::ToString() const
{
    std::ostringstream ss;

    ss << "CameraCaptureSettings {";
    ss << "captureMode: " << deviceId << ", ";
    ss << "resolution: " << profile.resolution.width << "x" << profile.resolution.height << ", ";
    ss << "format: " << static_cast<int32_t>(profile.format) << ", ";
    ss << "framerate: " << profile.frameRateRange.min << "-" << profile.frameRateRange.max;
    ss << "}";

    return ss.str();
}

bool SelectSettingsForVideo(
    const std::vector<CameraDeviceInfo>& devices, const MediaTrackConstraints& constraints, int defaultWidth,
    int defaultHeight, double defaultFrameRate, CameraCaptureSettings& setting, std::string& failedConstraintName)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    RTC_DLOG(LS_VERBOSE) << "Constraints: " << constraints.ToString();

    // This function works only if infinity is defined for the double type.
    static_assert(std::numeric_limits<double>::has_infinity, "Requires infinity");

    uint32_t success = 0;
    std::vector<double> bestDistance(constraints.Advanced().size() + 1 + kNumDefaultDistanceEntries);
    std::fill(bestDistance.begin(), bestDistance.end(), HUGE_VAL);

    for (auto& device : devices) {
        RTC_DLOG(LS_INFO) << "device: " << device.deviceId << ", " << device.groupId << ", "
                          << FacingModeToString(device.facingMode);
        if (!DeviceSatisfiesConstraintSet(device, constraints.Basic(), &failedConstraintName)) {
            RTC_DLOG(LS_ERROR) << "Failed to satisfies basic constraints: " << failedConstraintName;
            continue;
        }

        for (auto& profile : device.profiles) {
            RTC_DLOG(LS_INFO) << "-- profile: " << profile.resolution.width << "x" << profile.resolution.height << ","
                              << profile.frameRateRange.min << "-" << profile.frameRateRange.max << ", "
                              << profile.format;
            CandidateSettings candidate(device.deviceId, device.groupId, device.facingMode, profile);
            if (!candidate.ApplyConstraintSet(constraints.Basic(), &failedConstraintName)) {
                RTC_DLOG(LS_ERROR) << "Failed to apply basic constraints: " << failedConstraintName;
                continue;
            }

            // At this point we have a candidate that satisfies all basic constraints.
            std::vector<double> candidateDistanceVector;

            // 1. satisfaction of advanced constraint sets.
            for (const auto& advancedSet : constraints.Advanced()) {
                bool satisfiesAdvancedSet = false;

                if (DeviceSatisfiesConstraintSet(device, advancedSet, nullptr)) {
                    if (candidate.ApplyConstraintSet(advancedSet, nullptr)) {
                        satisfiesAdvancedSet = true;
                    } else {
                        RTC_DLOG(LS_ERROR) << "Failed to apply advanced constraints";
                    }
                } else {
                    RTC_DLOG(LS_ERROR) << "Failed to satisfies advanced constraints";
                }

                candidateDistanceVector.push_back(satisfiesAdvancedSet ? 0 : HUGE_VAL);
            }

            // 2. fitness distance.
            candidateDistanceVector.push_back(candidate.Fitness(constraints.Basic()));

            // 3. default resolution
            candidateDistanceVector.push_back(SquareEuclideanDistance(
                candidate.NativeWidth(), candidate.NativeHeight(), defaultWidth, defaultHeight));

            // 4. default frame rate
            double frameRateDistance = 0.0;
            if (defaultFrameRate < candidate.NativeFrameRateRange().min) {
                frameRateDistance =
                    NumericConstraintFitnessDistance(candidate.NativeFrameRateRange().min, defaultFrameRate);
            } else if (defaultFrameRate > candidate.NativeFrameRateRange().max) {
                frameRateDistance =
                    NumericConstraintFitnessDistance(candidate.NativeFrameRateRange().max, defaultFrameRate);
            }
            candidateDistanceVector.push_back(frameRateDistance);

            // 5. order in devices
            for (std::size_t i = 0; i < devices.size(); ++i) {
                if (device.deviceId == devices[i].deviceId) {
                    candidateDistanceVector.push_back(i);
                    break;
                }
            }

#ifndef NDEBUG
            {
                rtc::StringBuilder builder;
                builder << "[";
                for (auto& distance : candidateDistanceVector) {
                    if (builder.size() > 1) {
                        builder << ", ";
                    }
                    builder << distance;
                }
                builder << "]";
                RTC_DLOG(LS_INFO) << "candidateDistanceVector: " << builder.str();
            }
#endif
            RTC_DCHECK_EQ(bestDistance.size(), candidateDistanceVector.size());
            if (std::lexicographical_compare(
                    candidateDistanceVector.begin(), candidateDistanceVector.end(), bestDistance.begin(),
                    bestDistance.end()))
            {
                bestDistance = candidateDistanceVector;
#ifndef NDEBUG
                {
                    rtc::StringBuilder builder;
                    builder << "[";
                    for (auto& distance : bestDistance) {
                        if (builder.size() > 1) {
                            builder << ", ";
                        }
                        builder << distance;
                    }
                    builder << "]";
                    RTC_DLOG(LS_INFO) << "bestDistance: " << builder.str();
                }
#endif
                setting = candidate.GetSetting();
                success = 1;
            }
        }
    }
    RTC_DLOG(LS_INFO) << "success: " << success;

    return success > 0;
}

template <typename T, typename ConstraintType>
bool GetConstraint(const ConstraintType& constraint, T* value)
{
    if (constraint.HasExact()) {
        *value = constraint.Exact();
        return true;
    }
    if (constraint.HasIdeal()) {
        *value = constraint.Ideal();
        return true;
    }
    return false;
}

template <typename T, typename ConstraintType>
void ConstraintToOptional(const ConstraintType& constraint, absl::optional<T>* valueOut)
{
    T value;
    bool present = GetConstraint(constraint, &value);
    if (present) {
        *valueOut = value;
    }
}

void CopyConstraintsIntoAudioOptions(const MediaTrackConstraints& constraints, cricket::AudioOptions& options)
{
    if (constraints.IsNull()) {
        return;
    }

    // ignore advanced constraints
    auto& basicSet = constraints.Basic();
    ConstraintToOptional<bool>(basicSet.echoCancellation, &options.echo_cancellation);
    ConstraintToOptional<bool>(basicSet.autoGainControl, &options.auto_gain_control);
    ConstraintToOptional<bool>(basicSet.noiseSuppression, &options.noise_suppression);
}

void GetScreenCaptureOptionsFromConstraints(const MediaTrackConstraints& constraints, ScreenCaptureOptions& options)
{
    if (constraints.IsNull()) {
        return;
    }

    // ignore advanced constraints
    auto& basicSet = constraints.Basic();

    ConstraintToOptional<int32_t>(basicSet.width, &options.videoFrameWidth);
    ConstraintToOptional<int32_t>(basicSet.height, &options.videoFrameHeight);

    std::vector<std::string> captureModes;
    GetConstraint(basicSet.ohosScreenCaptureMode, &captureModes);
    if (captureModes.size() > 0) {
        options.captureMode = ScreenCaptureOptions::CaptureModeFromString(captureModes[0]);
    }

    ConstraintToOptional<uint64_t>(basicSet.ohosScreenCaptureDisplayId, &options.displayId);

    std::vector<std::string> missionIds;
    GetConstraint(basicSet.ohosScreenCaptureMissionId, &missionIds);
    for (const auto& missionId : missionIds) {
        options.missionIds.push_back(std::stoull(missionId));
    }

    std::vector<std::string> filteredWindowIds;
    GetConstraint(basicSet.ohosScreenCaptureWindowFilter, &filteredWindowIds);
    for (const auto& windowId : filteredWindowIds) {
        options.filteredWindowIds.push_back(std::stoull(windowId));
    }

    std::vector<std::string> filteredAudioContents;
    GetConstraint(basicSet.ohosScreenCaptureAudioFilter, &filteredAudioContents);
    options.filteredAudioContents = ScreenCaptureOptions::FilterableAudioContentFromString(filteredAudioContents);

    std::vector<std::string> skipPrivacyModeWindowIds;
    GetConstraint(basicSet.ohosScreenCaptureSkipPrivacyMode, &skipPrivacyModeWindowIds);
    for (const auto& windowId : skipPrivacyModeWindowIds) {
        options.skipPrivacyModeWindowIds.push_back(std::stoull(windowId));
    }

    ConstraintToOptional<bool>(basicSet.ohosScreenCaptureAutoRotation, &options.autoRotation);
}

} // namespace webrtc
