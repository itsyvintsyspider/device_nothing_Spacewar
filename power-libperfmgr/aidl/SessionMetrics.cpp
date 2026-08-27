/*
 * Copyright 2025 The Android Open Source Project
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

#include "SessionMetrics.h"

#include <android-base/logging.h>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

static constexpr int32_t TIME_BUCKETS_SIZE = 100;

void SessionMetrics::resetMetric(ThrottlingSeverity newThermalState, ScenarioType newScenario) {
    scenarioType = newScenario;
    thermalThrotStat = newThermalState;
    metricSessionCompleted = false;
    totalFrameNumber = 0;
    metricStartTime = std::chrono::system_clock::now();
    appFrameMetrics = std::nullopt;
    gameFrameMetrics = std::nullopt;
}

void SessionMetrics::addNewFrames(const GameFrameMetrics &newFrameMetrics) {
    if (!gameFrameMetrics) {
        GameFrameMetrics newGameMetrics;
        newGameMetrics.frameTimingMs.resize(TIME_BUCKETS_SIZE, 0);
        newGameMetrics.frameTimingDeltaMs.resize(TIME_BUCKETS_SIZE, 0);
        gameFrameMetrics = newGameMetrics;
    }

    gameFrameMetrics.value().totalFrameTimeMs += newFrameMetrics.totalFrameTimeMs;
    gameFrameMetrics.value().numOfFrames += newFrameMetrics.numOfFrames;
    totalFrameNumber += newFrameMetrics.numOfFrames;

    for (const auto &frameDur : newFrameMetrics.frameTimingMs) {
        if (frameDur >= TIME_BUCKETS_SIZE) {
            gameFrameMetrics.value().frameTimingMs[TIME_BUCKETS_SIZE - 1]++;
            // Because we're going to use the total time to compute the total average
            // FPS, limiting the maximum value of the outlier's frame duration here.
            // Deducting the parts that's over the maximum value "TIME_BUCKETS_SIZE"
            // which has been added above.
            gameFrameMetrics.value().totalFrameTimeMs -= frameDur - TIME_BUCKETS_SIZE;
        } else if (frameDur >= 0) {
            gameFrameMetrics.value().frameTimingMs[frameDur]++;
        }
    }

    for (const auto &frameDurDelta : newFrameMetrics.frameTimingDeltaMs) {
        if (frameDurDelta >= TIME_BUCKETS_SIZE) {
            gameFrameMetrics.value().frameTimingDeltaMs[TIME_BUCKETS_SIZE - 1]++;
        } else if (frameDurDelta >= 0) {
            gameFrameMetrics.value().frameTimingDeltaMs[frameDurDelta]++;
        }
    }
}

void SessionMetrics::addNewFrames(const FrameBuckets &newFrameMetrics) {
    if (!appFrameMetrics) {
        appFrameMetrics = newFrameMetrics;
        totalFrameNumber += newFrameMetrics.totalNumOfFrames;
        return;
    }

    appFrameMetrics.value().addUpNewFrames(newFrameMetrics);
    totalFrameNumber += newFrameMetrics.totalNumOfFrames;
}

std::ostream &SessionMetrics::dump(std::ostream &os) const {
    os << "Session uid: " << std::to_string(uid.value_or(-1)) << ", ";
    os << "Scenario: " << toString(scenarioType) << ", ";
    os << "FrameTimelineType: " << toString(frameTimelineType) << ", ";
    os << "Thermal throttling status: " << ::android::internal::ToString(thermalThrotStat) << "\n";

    std::time_t startTime = std::chrono::system_clock::to_time_t(metricStartTime);
    os << "    Start time: " << std::ctime(&startTime);

    if (metricSessionCompleted) {
        std::time_t endTime = std::chrono::system_clock::to_time_t(metricEndTime);
        os << "    End time: " << std::ctime(&endTime);
    }

    if (appFrameMetrics) {
        os << "    ";
        os << appFrameMetrics.value().toString();
        os << "\n";
    }

    if (gameFrameMetrics) {
        os << "    frameTimingHistogram: [";
        bool notEmpty = false;
        for (int i = 0; i < TIME_BUCKETS_SIZE; i++) {
            if (gameFrameMetrics.value().frameTimingMs[i] > 0) {
                if (notEmpty)
                    os << ", ";
                os << i << ":" << gameFrameMetrics.value().frameTimingMs[i];
                notEmpty = true;
            }
        }
        os << "]\n";
        os << "    frameTimingDeltaHistogram: [";
        notEmpty = false;
        for (int i = 0; i < TIME_BUCKETS_SIZE; i++) {
            if (gameFrameMetrics.value().frameTimingDeltaMs[i] > 0) {
                if (notEmpty)
                    os << ", ";
                os << i << ":" << gameFrameMetrics.value().frameTimingDeltaMs[i];
                notEmpty = true;
            }
        }
        os << "]\n";
        auto avgFPS = gameFrameMetrics.value().totalFrameTimeMs > 0
                              ? gameFrameMetrics.value().numOfFrames * 1000.0 /
                                        gameFrameMetrics.value().totalFrameTimeMs
                              : -1;
        os << "    Average FPS: " << avgFPS << "\n";
        os << "    Total number of frames: " << gameFrameMetrics.value().numOfFrames << "\n";
    }
    return os;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
