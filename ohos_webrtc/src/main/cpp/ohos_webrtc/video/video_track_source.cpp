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

#include "video_track_source.h"
#include "utils/marcos.h"
#include "video_frame_receiver_gl.h"

#include "api/video/i420_buffer.h"
#include "rtc_base/logging.h"

namespace webrtc {

namespace {

const int kRequiredResolutionAlignment = 2;

} // namespace

rtc::scoped_refptr<OhosVideoTrackSource> OhosVideoTrackSource::Create(
    std::unique_ptr<VideoCapturer> capturer, rtc::Thread* signalingThread, std::shared_ptr<EglContext> sharedContext)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;

    if (!capturer) {
        RTC_LOG(LS_ERROR) << "The capturer is nullptr";
        return nullptr;
    }

    return rtc::make_ref_counted<OhosVideoTrackSource>(std::move(capturer), signalingThread, sharedContext);
}

OhosVideoTrackSource::OhosVideoTrackSource(
    std::unique_ptr<VideoCapturer> capturer, rtc::Thread* signalingThread, std::shared_ptr<EglContext> sharedContext)
    : thread_(rtc::Thread::Create()),
      signalingThread_(signalingThread),
      capturer_(std::move(capturer)),
      sharedContext_(sharedContext),
      videoAdapter_(kRequiredResolutionAlignment)
{
    RTC_LOG(LS_INFO) << "OhosVideoTrackSource ctor: " << this;

    SetState(kInitializing);

    thread_->SetName("v-track-source", capturer_.get());
    thread_->Start();
    thread_->PostTask(
        [this] { capturer_->Init(VideoFrameReceiverGl::Create("v-frame-receiver", sharedContext_), this); });
}

OhosVideoTrackSource::~OhosVideoTrackSource()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;

    thread_->PostTask([this] {
        capturer_->Stop();
        capturer_->Release();
        capturer_.reset();
    });
    thread_->Stop();
}

void OhosVideoTrackSource::Start()
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    thread_->PostTask([this] { capturer_->Start(); });
}

void OhosVideoTrackSource::Stop()
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    thread_->PostTask([this] { capturer_->Stop(); });
}

void OhosVideoTrackSource::SetCapturerObserver(VideoCapturer::Observer* observer)
{
    UNUSED std::lock_guard<std::mutex> lock(obsMutex_);
    capturerObserver_ = observer;
}

void OhosVideoTrackSource::SetState(SourceState state)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__ << " state: " << state;

    if (state_.exchange(state) != state) {
        if (rtc::Thread::Current() == signalingThread_) {
            FireOnChanged();
        } else {
            signalingThread_->PostTask([this] { FireOnChanged(); });
        }
    }
}

void OhosVideoTrackSource::SetState(bool isLive)
{
    SetState(isLive ? kLive : kEnded);
}

bool OhosVideoTrackSource::ApplyRotation()
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return broadcaster_.wants().rotation_applied;
}

bool OhosVideoTrackSource::AdaptFrame(
    int width, int height, int64_t time_us, int* out_width, int* out_height, int* crop_width, int* crop_height,
    int* crop_x, int* crop_y)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    {
        UNUSED MutexLock lock(&statsMutex_);
        stats_ = Stats{width, height};
    }

    if (!broadcaster_.frame_wanted()) {
        return false;
    }

    if (!videoAdapter_.AdaptFrameResolution(
            width, height, time_us * rtc::kNumNanosecsPerMicrosec, crop_width, crop_height, out_width, out_height))
    {
        // VideoAdapter dropped the frame.
        return false;
    }

    *crop_x = (width - *crop_width) / 2;
    *crop_y = (height - *crop_height) / 2;

    return true;
}

void OhosVideoTrackSource::AddOrUpdateSink(rtc::VideoSinkInterface<VideoFrame>* sink, const rtc::VideoSinkWants& wants)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    broadcaster_.AddOrUpdateSink(sink, wants);
    videoAdapter_.OnSinkWants(broadcaster_.wants());

    if (broadcaster_.frame_wanted()) {
        thread_->PostTask([this] { capturer_->Start(); });
    }
}

void OhosVideoTrackSource::RemoveSink(rtc::VideoSinkInterface<VideoFrame>* sink)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    broadcaster_.RemoveSink(sink);
    videoAdapter_.OnSinkWants(broadcaster_.wants());

    if (!broadcaster_.frame_wanted()) {
        thread_->PostTask([this] { capturer_->Stop(); });
    }
}

MediaSourceInterface::SourceState OhosVideoTrackSource::state() const
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return state_.load();
}

bool OhosVideoTrackSource::remote() const
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return false;
}

bool OhosVideoTrackSource::is_screencast() const
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return capturer_->IsScreencast();
}

absl::optional<bool> OhosVideoTrackSource::needs_denoising() const
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    return false;
}

bool OhosVideoTrackSource::GetStats(Stats* stats)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    UNUSED MutexLock lock(&statsMutex_);

    if (!stats_) {
        return false;
    }

    *stats = *stats_;
    return true;
}

void OhosVideoTrackSource::ProcessConstraints(const VideoTrackSourceConstraints& constraints)
{
    RTC_LOG(LS_VERBOSE) << __FUNCTION__;

    broadcaster_.ProcessConstraints(constraints);
}

void OhosVideoTrackSource::OnCapturerStarted(bool success)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    SetState(success);

    VideoCapturer::Observer* observer = nullptr;
    {
        UNUSED std::lock_guard<std::mutex> lock(obsMutex_);
        observer = capturerObserver_;
    }

    if (observer) {
        observer->OnCapturerStarted(success);
    }
}

void OhosVideoTrackSource::OnCapturerStopped()
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__;
    SetState(false);

    VideoCapturer::Observer* observer = nullptr;
    {
        UNUSED std::lock_guard<std::mutex> lock(obsMutex_);
        observer = capturerObserver_;
    }

    if (observer) {
        observer->OnCapturerStopped();
    }
}

void OhosVideoTrackSource::OnFrameCaptured(
    rtc::scoped_refptr<VideoFrameBuffer> buffer, int64_t timestampUs, VideoRotation rotation)
{
    RTC_DLOG(LS_VERBOSE) << __FUNCTION__ << " timestampUs=" << timestampUs << ", rotation=" << rotation;
    auto alignedTimestampUs = timestampAligner_.TranslateTimestamp(timestampUs, rtc::TimeMicros());
    RTC_DLOG(LS_VERBOSE) << "alignedTimestampUs=" << alignedTimestampUs;

    int adapted_width = 0;
    int adapted_height = 0;
    int crop_width = 0;
    int crop_height = 0;
    int crop_x = 0;
    int crop_y = 0;
    bool drop;

    if (rotation % kVideoRotation_180 == 0) {
        drop = !AdaptFrame(
            buffer->width(), buffer->height(), timestampUs, &adapted_width, &adapted_height, &crop_width, &crop_height,
            &crop_x, &crop_y);
    } else {
        // Swap all width/height and x/y.
        drop = !AdaptFrame(
            buffer->height(), buffer->width(), timestampUs, &adapted_height, &adapted_width, &crop_height, &crop_width,
            &crop_y, &crop_x);
    }

    RTC_DLOG(LS_VERBOSE) << "adapted_width=" << adapted_width << ", adapted_height=" << adapted_height
                         << ", crop_width=" << crop_width << ", crop_height=" << crop_height << ", crop_x=" << crop_x
                         << ", crop_y=" << crop_y;

    if (drop) {
        RTC_DLOG(LS_VERBOSE) << "dropped";
        broadcaster_.OnDiscardedFrame();
        return;
    }

    if (adapted_height != buffer->height() || adapted_width != buffer->width()) {
        buffer = buffer->CropAndScale(crop_x, crop_y, crop_width, crop_height, adapted_width, adapted_height);
    } else {
        // No adaptations needed, just return the frame as is.
    }

    auto frame = VideoFrame::Builder()
                     .set_id(1)
                     .set_video_frame_buffer(buffer)
                     .set_rotation(rotation)
                     .set_timestamp_us(alignedTimestampUs)
                     .build();
    if (ApplyRotation() && frame.rotation() != kVideoRotation_0 && buffer->type() == VideoFrameBuffer::Type::kI420) {
        /* Apply pending rotation. */
        VideoFrame rotatedFrame(frame);
        rotatedFrame.set_video_frame_buffer(I420Buffer::Rotate(*buffer->GetI420(), frame.rotation()));
        rotatedFrame.set_rotation(kVideoRotation_0);
        broadcaster_.OnFrame(rotatedFrame);
    } else {
        broadcaster_.OnFrame(frame);
    }
}

} // namespace webrtc
