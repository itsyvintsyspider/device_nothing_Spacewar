/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "powerhal-libperfmgr"
#include "TgidTypeChecker.h"

#include <android-base/file.h>
#include <android-base/logging.h>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

static constexpr char typeCheckNodePath[] = "/proc/vendor_sched/check_tgid_type";

TgidTypeChecker::TgidTypeChecker() {
    if (access(typeCheckNodePath, W_OK) != 0) {
        mTypeCheckerFd = -1;
        LOG(WARNING) << "Can't find vendor node: " << typeCheckNodePath;
        return;
    }

    int flags = O_WRONLY | O_TRUNC | O_CLOEXEC;
    mTypeCheckerFd = TEMP_FAILURE_RETRY(open(typeCheckNodePath, flags));
    if (mTypeCheckerFd < 0) {
        LOG(ERROR) << "Failed to open the node: " << mTypeCheckerFd;
    }
}

TgidTypeChecker::~TgidTypeChecker() {
    if (mTypeCheckerFd >= 0) {
        ::close(mTypeCheckerFd);
    }
}

ProcessTag TgidTypeChecker::getProcessTag(int32_t tgid) {
    std::lock_guard lock(mMutex);
    if (mTypeCheckerFd < 0) {
        LOG(WARNING) << "Invalid tigd type checker, skipping the check";
        return ProcessTag::DEFAULT;
    }

    auto val = std::to_string(tgid);
    int ret = TEMP_FAILURE_RETRY(write(mTypeCheckerFd, val.c_str(), val.length()));

    switch (ret) {
        case 1:
            return ProcessTag::SYSTEM_UI;
        case 2:
            return ProcessTag::CHROME;
        default:
            return ProcessTag::DEFAULT;
    }
    return ProcessTag::DEFAULT;
}

bool TgidTypeChecker::isValid() const {
    return mTypeCheckerFd >= 0;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
