/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "vendor.qti.hardware.vibrator.service.spacewar"

#include <log/log.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "Vibrator.h"
#include "richtap/RichtapVibrator.h"
#include "richtap/aac_vibra_function.h"

using aidl::android::hardware::vibrator::Vibrator;
using aidl::vendor::aac::hardware::richtap::vibrator::RichtapVibrator;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Vibrator> vib = ndk::SharedRefBase::make<Vibrator>();
    ndk::SpAIBinder vibBinder = vib->asBinder();

    std::shared_ptr<RichtapVibrator> cvib = ndk::SharedRefBase::make<RichtapVibrator>();
    CHECK(STATUS_OK == AIBinder_setExtension(vibBinder.get(), cvib->asBinder().get()));

    const std::string instance = std::string() + Vibrator::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(vibBinder.get(), instance.c_str());
    CHECK(status == STATUS_OK);

    cvib->init(nullptr);

    /*
     * RichtapVibrator::init() brings the AAC engine up but leaves its
     * internal master gain at whatever the engine defaults to (very low
     * in practice -- predefined effects and setAmplitude() calls from
     * Vibrator.cpp were both audible/tactile at only a fraction of their
     * intended strength until this was set). LOS's own reference sets
     * this once at startup too (0x7F there); maxed here at the user's
     * request. Must run after cvib->init(), which is what actually calls
     * aac_vibra_init() -- Vibrator's own constructor runs earlier than
     * that in this same main(), so it can't be the one to set this.
     */
    aac_vibra_setAmplitude(0xFF);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
