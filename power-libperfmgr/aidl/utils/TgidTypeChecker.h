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

#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <mutex>

#include "../AdpfTypes.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

class TgidTypeChecker {
  public:
    static std::shared_ptr<TgidTypeChecker> getInstance() {
        static std::shared_ptr<TgidTypeChecker> instance(new TgidTypeChecker());
        return instance;
    }

    ProcessTag getProcessTag(int32_t tgid);
    bool isValid() const;

    ~TgidTypeChecker();

  private:
    // singleton
    TgidTypeChecker();
    TgidTypeChecker(TgidTypeChecker const &) = delete;
    TgidTypeChecker &operator=(TgidTypeChecker const &) = delete;

    std::mutex mMutex;
    int mTypeCheckerFd = -1;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
