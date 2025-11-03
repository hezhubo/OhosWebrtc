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

#include "camera.h"

namespace ohos {

std::map<Camera_PreviewOutput*, std::list<CameraPreviewOutput::Observer*>> CameraPreviewOutput::observersMap_;
std::mutex CameraPreviewOutput::mutex_;

void CameraPreviewOutput::AddObserver(Observer* observer)
{
    NATIVE_THROW_IF_FAILED(observer != nullptr, -1, "OH_Camera", "Null argument");

    std::lock_guard<std::mutex> lock(mutex_);

    auto& observers = observersMap_[Raw()];
    if (observers.size() == 0) {
        PreviewOutput_Callbacks callback;
        callback.onFrameStart = OnPreviewOutputFrameStart;
        callback.onFrameEnd = OnPreviewOutputFrameEnd;
        callback.onError = OnPreviewOutputError;
        RegisterCallback(&callback);
    }

    observers.push_back(observer);
}

void CameraPreviewOutput::RemoveObserver(Observer* observer)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto& observers = observersMap_[Raw()];
    if (observer) {
        observers.clear();
    } else {
        observers.remove(observer);
    }

    if (observers.size() == 0) {
        PreviewOutput_Callbacks callback;
        callback.onFrameStart = OnPreviewOutputFrameStart;
        callback.onFrameEnd = OnPreviewOutputFrameEnd;
        callback.onError = OnPreviewOutputError;
        UnregisterCallback(&callback);
    }
}

void CameraPreviewOutput::OnPreviewOutputFrameStart(Camera_PreviewOutput* previewOutput)
{
    std::list<Observer*> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers = observersMap_[previewOutput];
    }

    for (Observer* obs : observers) {
        if (obs) {
            obs->OnPreviewOutputFrameStart();
        }
    }
}

void CameraPreviewOutput::OnPreviewOutputFrameEnd(Camera_PreviewOutput* previewOutput, int32_t frameCount)
{
    std::list<Observer*> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers = observersMap_[previewOutput];
    }

    for (Observer* obs : observers) {
        if (obs) {
            obs->OnPreviewOutputFrameEnd(frameCount);
        }
    }
}

void CameraPreviewOutput::OnPreviewOutputError(Camera_PreviewOutput* previewOutput, Camera_ErrorCode errorCode)
{
    std::list<Observer*> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers = observersMap_[previewOutput];
    }

    for (Observer* obs : observers) {
        if (obs) {
            obs->OnPreviewOutputError(errorCode);
        }
    }
}

std::map<Camera_VideoOutput*, std::list<CameraVideoOutput::Observer*>> CameraVideoOutput::observersMap_;
std::mutex CameraVideoOutput::mutex_;

void CameraVideoOutput::AddObserver(Observer* observer)
{
    NATIVE_THROW_IF_FAILED(observer != nullptr, -1, "OH_Camera", "Null argument");

    std::lock_guard<std::mutex> lock(mutex_);

    auto& observers = observersMap_[Raw()];
    if (observers.size() == 0) {
        VideoOutput_Callbacks callback;
        callback.onFrameStart = OnVideoOutputFrameStart;
        callback.onFrameEnd = OnVideoOutputFrameEnd;
        callback.onError = OnVideoOutputError;
        RegisterCallback(&callback);
    }

    observers.push_back(observer);
}

void CameraVideoOutput::RemoveObserver(Observer* observer)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto& observers = observersMap_[Raw()];
    if (observer) {
        observers.clear();
    } else {
        observers.remove(observer);
    }

    if (observers.size() == 0) {
        VideoOutput_Callbacks callback;
        callback.onFrameStart = OnVideoOutputFrameStart;
        callback.onFrameEnd = OnVideoOutputFrameEnd;
        callback.onError = OnVideoOutputError;
        UnregisterCallback(&callback);
    }
}

void CameraVideoOutput::OnVideoOutputFrameStart(Camera_VideoOutput* videoOutput)
{
    std::list<Observer*> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers = observersMap_[videoOutput];
    }

    for (Observer* obs : observers) {
        if (obs) {
            obs->OnVideoOutputFrameStart();
        }
    }
}

void CameraVideoOutput::OnVideoOutputFrameEnd(Camera_VideoOutput* videoOutput, int32_t frameCount)
{
    std::list<Observer*> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers = observersMap_[videoOutput];
    }

    for (Observer* obs : observers) {
        if (obs) {
            obs->OnVideoOutputFrameEnd(frameCount);
        }
    }
}

void CameraVideoOutput::OnVideoOutputError(Camera_VideoOutput* videoOutput, Camera_ErrorCode errorCode)
{
    std::list<Observer*> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers = observersMap_[videoOutput];
    }

    for (Observer* obs : observers) {
        if (obs) {
            obs->OnVideoOutputError(errorCode);
        }
    }
}

void CameraManager::AddObserver(Observer* observer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    observers_.push_back(observer);
}

void CameraManager::RemoveObserver(Observer* observer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    observers_.remove(observer);
}

void CameraManager::OnCameraManagerStatusCallback1(Camera_Manager* cameraManager, Camera_StatusInfo* status)
{
    GetInstance().OnCameraManagerStatusCallback(status);
}

void CameraManager::OnCameraManagerStatusCallback(Camera_StatusInfo* status)
{
    std::list<Observer*> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers = observers_;
    }

    for (Observer* obs : observers) {
        if (obs) {
            obs->OnDeviceStatus();
        }
    }
}

} // namespace ohos
