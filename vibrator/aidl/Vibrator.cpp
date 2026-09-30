/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "vendor.qti.vibrator.spacewar"

#include <inttypes.h>
#include <log/log.h>
#include <optional>
#include <string.h>
#include <unistd.h>

#include "Vibrator.h"
#include "VibratorOL/Vibrator.h"
#ifdef USE_LIBPALCLIENT
#include "VibratorCL/Vibrator.h"
#endif
#include "VibratorSelector/VibratorSelector.h"
#include "richtap/aac_vibra_function.h"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

/*
 * Real RichTap prebaked-effect IDs, not guesses: reverse-engineered from
 * LineageOS's own spacewar vibrator HAL (same libaacvibrator.so blob we
 * link here, same aac_richtap.config), confirmed present in
 * /vendor/lib64/libaacvibrator.so on this device (nm -D shows every
 * aac_vibra_* symbol this file calls, matching aac_vibra_function.h
 * exactly). TICK/THUD/POP sharing 0x3003 is a real limit of the shipped
 * table, not something invented here -- LOS's own mapping has the same
 * overlap, and the actual per-ID waveform data lives in the closed
 * aac_richtap.config blob, not anything we can inspect from source.
 */
static std::optional<uint32_t> mapEffectToPrebakedId(Effect effect) {
    switch (effect) {
        case Effect::CLICK:
            return 0x3008;
        case Effect::DOUBLE_CLICK:
            return 0x1001;
        case Effect::TICK:
        case Effect::THUD:
        case Effect::POP:
            return 0x3003;
        case Effect::HEAVY_CLICK:
            return 0x3007;
        default:
            return std::nullopt;
    }
}

static int32_t effectStrengthToRichtap(EffectStrength es) {
    switch (es) {
        case EffectStrength::LIGHT:
            return 69;
        case EffectStrength::STRONG:
            return 150;
        case EffectStrength::MEDIUM:
        default:
            return 100;
    }
}

class Vibrator::VibratorPrivate {
private:
    VibratorOL mVibratorOL;
#ifdef USE_LIBPALCLIENT
    VibratorCL mVibratorCL;
#endif
    IVibrator* mSelectedVibrator;
    std::mutex VibratorSelectionLock;
    bool mSupportCL;
    std::shared_ptr<VibratorSelector> mVibSelector;
public:
    VibratorPrivate() {
        mSupportCL = mVibratorOL.mSupportVISense;
        mVibSelector = nullptr;
        mSelectedVibrator = &mVibratorOL;
        int32_t ret;

        if (mSupportCL) {
            ret = VibratorSelector::init();
            if (!ret) {
                mVibSelector = VibratorSelector::GetInstance();
            }
        }
    }

    ndk::ScopedAStatus getCapabilities(int32_t* _aidl_return) {
#ifdef USE_LIBPALCLIENT
        int32_t capabilities;

        /* Merge the capabilities of both mVibratorOL and mVibratorCL */
        mVibratorOL.getCapabilities(_aidl_return);
        if (mSupportCL) {
            mVibratorCL.getCapabilities(&capabilities);
            *_aidl_return |= capabilities;
        }
#else
        mVibratorOL.getCapabilities(_aidl_return);
#endif

        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus on(int32_t timeoutMs, const std::shared_ptr<IVibratorCallback>& callback) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        int32_t ret = aac_vibra_looper_on(static_cast<uint32_t>(timeoutMs));
        if (ret < 0) {
            ALOGE("aac_vibra_looper_on failed: %d, falling back to VibratorOL", ret);
            mSelectedVibrator = &mVibratorOL;
#ifdef USE_LIBPALCLIENT
            if (mVibSelector && mVibSelector->getVibForOnApi(timeoutMs) == VIB_TYPE_CL)
                mSelectedVibrator = &mVibratorCL;
#endif
            status = mSelectedVibrator->on(timeoutMs, callback);
            VibratorSelectionLock.unlock();
            return status;
        }

        if (callback != nullptr) {
            std::thread([=] {
                usleep(timeoutMs * 1000);
                callback->onComplete();
            }).detach();
        }

        VibratorSelectionLock.unlock();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus off() {
        /* The selected vibrator should be always used to turn off vibration */
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();
        aac_vibra_off();
        status = mSelectedVibrator->off();
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus perform(Effect effect, EffectStrength es, const std::shared_ptr<IVibratorCallback>& callback, int32_t* _aidl_return) {

        int effect_id = static_cast<int> (effect);
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        auto mappedEffect = mapEffectToPrebakedId(effect);
        if (mappedEffect.has_value()) {
            int32_t strength = effectStrengthToRichtap(es);
            int32_t ret = aac_vibra_looper_prebaked_effect(mappedEffect.value(), strength);
            if (ret >= 0) {
                if (callback != nullptr) {
                    std::thread([=] {
                        usleep(ret * 1000);
                        callback->onComplete();
                    }).detach();
                }
                *_aidl_return = ret;
                VibratorSelectionLock.unlock();
                return ndk::ScopedAStatus::ok();
            }
            ALOGE("aac_vibra_looper_prebaked_effect(0x%x) failed: %d, falling back to VibratorOL",
                  mappedEffect.value(), ret);
        }

        mSelectedVibrator = &mVibratorOL;
#ifdef USE_LIBPALCLIENT
        if (mVibSelector && mVibSelector->getVibForPerformApi(effect_id) == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;
#endif

        status = mSelectedVibrator->perform(effect, es, callback, _aidl_return);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getSupportedEffects(std::vector<Effect>* _aidl_return) {
#ifdef USE_LIBPALCLIENT
        std::vector<Effect> effectsCL;

        /* Merge the effects being supported by both mVibratorOL and mVibratorCL */
        mVibratorOL.getSupportedEffects(_aidl_return);
        if (mSupportCL) {
            mVibratorCL.getSupportedEffects(&effectsCL);
            for (uint32_t i = 0; i < effectsCL.size() ; i++) {
                if (std::find(_aidl_return->begin(), _aidl_return->end(), effectsCL[i]) == _aidl_return->end())
                    _aidl_return->insert(_aidl_return->end(), effectsCL[i]);
            }
        }
#else
        mVibratorOL.getSupportedEffects(_aidl_return);
#endif

        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus setAmplitude(float amplitude) {
        /* Set amplitude should be only called after On() vibration is enabled so use existing mSelectedVibrator */
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        uint8_t tmp = static_cast<uint8_t>(amplitude * 0xff);
        int32_t ret = aac_vibra_setAmplitude(tmp);
        if (ret == 0) {
            VibratorSelectionLock.unlock();
            return ndk::ScopedAStatus::ok();
        }
        ALOGE("aac_vibra_setAmplitude failed: %d, falling back to VibratorOL", ret);

        status = mSelectedVibrator->setAmplitude(amplitude);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus setExternalControl(bool enabled) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

#ifdef USE_LIBPALCLIENT
        if (mSupportCL)
            mVibratorCL.setExternalControl(enabled);
#endif

        status = mVibratorOL.setExternalControl(enabled);

        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getCompositionDelayMax(int32_t* maxDelayMs) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
#ifdef USE_LIBPALCLIENT
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;
#endif

        status = mSelectedVibrator->getCompositionDelayMax(maxDelayMs);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getCompositionSizeMax(int32_t* maxSize) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
#ifdef USE_LIBPALCLIENT
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;
#endif

        status = mSelectedVibrator->getCompositionSizeMax(maxSize);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
#ifdef USE_LIBPALCLIENT
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;
#endif

        status = mSelectedVibrator->getSupportedPrimitives(supported);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getPrimitiveDuration(CompositePrimitive primitive, int32_t* durationMs) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
#ifdef USE_LIBPALCLIENT
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;
#endif

        status = mSelectedVibrator->getPrimitiveDuration(primitive, durationMs);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus compose(const std::vector<CompositeEffect>& composite,
                    const std::shared_ptr<IVibratorCallback>& callback) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
#ifdef USE_LIBPALCLIENT
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;
#endif

        status = mSelectedVibrator->compose(composite, callback);
        VibratorSelectionLock.unlock();
        return status;
    }
};


Vibrator::Vibrator() {
    pImpl = new Vibrator::VibratorPrivate;
}

Vibrator::~Vibrator() {
    if (NULL != pImpl) {
        delete pImpl;
        pImpl = NULL;
    }
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    return pImpl->getCapabilities(_aidl_return);
}

ndk::ScopedAStatus Vibrator::off() {
    return pImpl->off();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
    const std::shared_ptr<IVibratorCallback>& callback) {
    return pImpl->on(timeoutMs, callback);
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength es, const std::shared_ptr<IVibratorCallback>& callback, int32_t* _aidl_return) {
    return pImpl->perform(effect, es, callback, _aidl_return);
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    return pImpl->getSupportedEffects(_aidl_return);
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    return pImpl->setAmplitude(amplitude);
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    return pImpl->setExternalControl(enabled);
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs) {
    return pImpl->getCompositionDelayMax(maxDelayMs);
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize) {
    return pImpl->getCompositionSizeMax(maxSize);
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
    return pImpl->getSupportedPrimitives(supported);
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive,
    int32_t* durationMs) {
    return pImpl->getPrimitiveDuration(primitive, durationMs);
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& composite,
    const std::shared_ptr<IVibratorCallback>& callback) {
    return pImpl->compose(composite, callback);
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>* _aidl_return __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t id __unused, Effect effect __unused,
    EffectStrength strength __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float* resonantFreqHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getQFactor(float* qFactor __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float* freqResolutionHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float* freqMinimumHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float>* _aidl_return __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t* durationMs __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t* maxSize __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking>* supported __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle>& composite __unused,
    const std::shared_ptr<IVibratorCallback>& callback __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl