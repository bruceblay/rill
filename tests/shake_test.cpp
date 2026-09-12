// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ShakeDetector.h"
#include <cassert>
#include <limits>
int main() {
  ShakeDetector s;
  // Rest and slow orientation changes should not count as shakes.
  for (uint32_t t = 0; t < 1000; t += 20) assert(!s.update(0, 0, 1, t));
  assert(!s.update(1, 0, 0, 1000));
  assert(!s.update(2, 0, 0, 1100));
  assert(!s.update(2, 0, 0, 1120)); // Sustained acceleration is only one peak.
  assert(!s.update(1, 0, 0, 1140));
  assert(s.update(-2, 0, 0, 1200));
  assert(!s.update(0, 0, 1, 1300));
  assert(!s.update(0, 0, 3, 1400)); // Cooldown.
  assert(!s.update(0, 0, 1, 2800));
  assert(!s.update(0, 0, 2, 2900));
  assert(!s.update(0, 0, 1, 2920));
  assert(!s.update(0, 0, 2, 3700)); // Old first peak expired.
  assert(!s.update(0, 0, 1, 3720));
  assert(s.update(0, 0, 2, 3800));
  assert(!s.update(std::numeric_limits<float>::quiet_NaN(), 0, 0, 6000));
  // Unsigned elapsed time works across millis() rollover.
  ShakeDetector wrap;
  assert(!wrap.update(0, 0, 1, UINT32_MAX - 200));
  assert(!wrap.update(0, 0, 2, UINT32_MAX - 150));
  assert(!wrap.update(0, 0, 1, UINT32_MAX - 100));
  assert(wrap.update(0, 0, 2, 20));
}
