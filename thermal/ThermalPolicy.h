// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <cstddef>

namespace mako {

// Battery temperature is in tenths of a degree. These thresholds track the
// first three performance limits in thermald-mako.conf. Later stages only
// tighten charging limits, so they keep the same performance severity.
inline constexpr std::array<int, 3> kBatteryHot{370, 380, 390};
inline constexpr std::array<int, 3> kBatteryCold{360, 370, 380};

inline constexpr size_t batterySeverity(int decicelsius, size_t previous) {
    size_t level = previous;
    while (level > 0 && decicelsius <= kBatteryCold[level - 1]) --level;
    while (level < kBatteryHot.size() && decicelsius >= kBatteryHot[level]) ++level;
    return level;
}

}  // namespace mako
