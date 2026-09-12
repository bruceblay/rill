// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/Garden.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <memory>

static void le(std::ostream& out, uint32_t value, unsigned bytes) {
  while (bytes--) { out.put(char(value & 255)); value >>= 8; }
}
int main(int argc, char** argv) {
  if (argc < 2) { std::cerr << "render output.wav [seconds=180] [seed=1818846821] [first-family=0..6]\n"; return 1; }
  unsigned seconds = argc > 2 ? unsigned(std::strtoul(argv[2], nullptr, 10)) : 180;
  if (seconds < 1 || seconds > 3600) return 2;
  uint32_t seed = argc > 3 ? uint32_t(std::strtoul(argv[3], nullptr, 10)) : 0x6c696665;
  auto engine = std::unique_ptr<garden::Engine>(new garden::Engine(seed, argc > 4 ? std::atoi(argv[4]) : -1));
  std::ofstream out(argv[1], std::ios::binary);
  if (!out) return 3;
  uint32_t count = seconds * garden::rate, bytes = count * 2;
  out.write("RIFF",4); le(out,bytes+36,4); out.write("WAVEfmt ",8);
  le(out,16,4); le(out,1,2); le(out,1,2); le(out,garden::rate,4);
  le(out,garden::rate*2,4); le(out,2,2); le(out,16,2); out.write("data",4); le(out,bytes,4);
  double squares = 0, sum = 0; float peak = 0, delta = 0, previous = 0;
  for (uint32_t i = 0; i < count; ++i) {
    float sample = engine->sample();
    if (!std::isfinite(sample) || std::abs(sample) >= 1) return 4;
    peak = std::max(peak,std::abs(sample)); delta = std::max(delta,std::abs(sample-previous));
    previous = sample; squares += sample*sample; sum += sample;
    // Audition file fades only at its endpoint; the live score continues forever.
    if (count-i < garden::rate*6) sample *= float(count-i)/(garden::rate*6);
    le(out,uint16_t(int16_t(sample*32767)),2);
  }
  std::cout << "seconds=" << seconds << " peak=" << peak << " rms=" << std::sqrt(squares/count)
    << " dc=" << sum/count << " max_delta=" << delta << " engine_bytes=" << sizeof(garden::Engine) << "\n";
  return out ? 0 : 5;
}
