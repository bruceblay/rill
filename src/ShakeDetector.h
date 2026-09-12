// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <math.h>

// Two acceleration peaks within 700 ms, separated by a return toward 1 g.
// Magnitude makes the gesture independent of how the device is held.
class ShakeDetector {
  bool armed = false, firstPeak = false, cooling = false;
  uint32_t firstAt = 0, changedAt = 0;
public:
  bool update(float x, float y, float z, uint32_t now) {
    const float g2 = x*x + y*y + z*z;
    if (!isfinite(g2)) return false;
    if (cooling) {
      if (uint32_t(now - changedAt) < 1500) return false;
      cooling = false;
    }
    if (firstPeak && uint32_t(now - firstAt) > 700) firstPeak = false;
    if (g2 < 1.25f * 1.25f) armed = true;
    if (!armed || g2 < 1.8f * 1.8f) return false;
    armed = false;
    if (!firstPeak) { firstPeak = true; firstAt = now; return false; }
    firstPeak = false;
    cooling = true;
    changedAt = now;
    return true;
  }
};
