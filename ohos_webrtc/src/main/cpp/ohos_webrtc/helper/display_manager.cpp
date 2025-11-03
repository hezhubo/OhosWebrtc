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

#include "display_manager.h"

namespace ohos {

void DisplayManager::DisplayChangeCallback1(uint64_t displayId)
{
    GetInstance().DisplayChangeCallback(displayId);
}

void DisplayManager::RegisterChangeCallback(ChangeObserver* observer)
{
    if (!observer) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (changeObservers_.size() == 0) {
        RegisterDisplayChangeListener();
    }
    changeObservers_.push_back(observer);
}

void DisplayManager::UnregisterChangeCallback(ChangeObserver* observer)
{
    if (!observer) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    changeObservers_.remove(observer);
    if (changeObservers_.size() == 0) {
        UnregisterDisplayChangeListener();
    }
}

void DisplayManager::DisplayChangeCallback(uint64_t displayId)
{
    RTC_DLOG_F(LS_INFO) << "displayId=" << displayId;

    std::lock_guard<std::mutex> lock(mutex_);
    std::for_each(changeObservers_.begin(), changeObservers_.end(),
        [displayId](auto* observer) { observer->OnDisplayChange(displayId); });
}

bool DisplayManager::RegisterDisplayChangeListener()
{
    NativeDisplayManager_ErrorCode errCode =
        OH_NativeDisplayManager_RegisterDisplayChangeListener(DisplayManager::DisplayChangeCallback1, &listenerIndex_);
    RTC_DLOG_F(LS_INFO) << "listenerIndex=" << listenerIndex_;
    if (errCode == DISPLAY_MANAGER_OK) {
        return true;
    } else {
        RTC_DLOG_F(LS_ERROR) << "Failed to register display change listener: " << errCode;
        return false;
    }
}

bool DisplayManager::UnregisterDisplayChangeListener()
{
    RTC_DLOG_F(LS_INFO) << "listenerIndex=" << listenerIndex_;
    NativeDisplayManager_ErrorCode errCode = OH_NativeDisplayManager_UnregisterDisplayChangeListener(listenerIndex_);
    if (errCode == DISPLAY_MANAGER_OK) {
        return true;
    } else {
        RTC_DLOG_F(LS_ERROR) << "Failed to unregister display change listener: " << errCode;
        return false;
    }
}

} // namespace ohos
