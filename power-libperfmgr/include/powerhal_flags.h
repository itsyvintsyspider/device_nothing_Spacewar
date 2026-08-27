#pragma once

#include <memory>

namespace powerhal {
namespace flags {

class flag_provider_interface {
  public:
    virtual ~flag_provider_interface() = default;
    virtual bool test_flag() { return false; }
    virtual bool gpu_load_up_for_blurs() { return false; }
    virtual bool ramp_down_sf_prefer_high_cap() { return false; }
    virtual bool chrome_profile_bypass_hboost_severe_jank_level() { return false; }
};

inline bool test_flag() {
    return false;
}

inline bool gpu_load_up_for_blurs() {
    return false;
}

inline bool ramp_down_sf_prefer_high_cap() {
    return false;
}

inline bool chrome_profile_bypass_hboost_severe_jank_level() {
    return false;
}

inline std::unique_ptr<flag_provider_interface> provider_ =
    std::make_unique<flag_provider_interface>();

}  // namespace flags
}  // namespace powerhal
