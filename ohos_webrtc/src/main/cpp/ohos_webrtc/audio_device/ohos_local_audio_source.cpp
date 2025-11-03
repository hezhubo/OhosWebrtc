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

#include "ohos_local_audio_source.h"

#include <thread>

#include "api/audio/audio_frame.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

namespace webrtc {

rtc::scoped_refptr<OhosLocalAudioSource> OhosLocalAudioSource::Create(cricket::AudioOptions audioOptions,
    std::shared_ptr<AudioInput> audioInput)
{
    RTC_DCHECK(audioInput);
    if (!audioInput) {
        RTC_LOG(LS_WARNING) << "Invalid parameter";
        return nullptr;
    }

    auto source = rtc::make_ref_counted<OhosLocalAudioSource>();
    source->Initialize(std::move(audioOptions), std::move(audioInput));

    return source;
}

void OhosLocalAudioSource::Initialize(cricket::AudioOptions audioOptions, std::shared_ptr<AudioInput> audioInput)
{
    audioOptions_ = std::move(audioOptions);
    audioInput_ = std::move(audioInput);
}

std::string OhosLocalAudioSource::GetLabel() const
{
    return audioInput_->GetLabel();
}

void OhosLocalAudioSource::SetMute(bool mute)
{
    audioInput_->SetMute(mute);
}

} // namespace webrtc
