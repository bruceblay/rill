#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// A bounded, deterministic score and renderer shared by firmware and audition.
// No allocation, locks, or transcendental functions in the per-sample path.
namespace garden {
constexpr uint32_t rate = 32000;
constexpr float pi = 3.14159265358979323846f;
class Engine {
  struct Voice {
    uint32_t age = 0, duration = 0, attack = 0, release = 0;
    float phase[3] = {}, step[3] = {}, gain = 0, color = 0;
  };
  std::array<Voice, 9> voices{};
  std::array<float, 2049> sine{};
  // Parallel damped combs followed by two diffusers; under 40 KB total.
  std::array<std::array<float, 2003>, 4> comb{};
  const unsigned lengths[4] = {1499, 1601, 1867, 2003};
  unsigned ci[4] = {};
  float damping[4] = {};
  std::array<float, 353> ap1{};
  std::array<float, 127> ap2{};
  unsigned ai = 0, bi = 0;
  uint32_t rng;
  uint64_t clock = 0, nextLow = 0, nextMiddle = rate * 3, nextHigh = rate * 11;
  unsigned lastDegree = 2;
  float dcIn = 0, dcOut = 0, level = 0, target = 1;
  uint32_t random() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
  float unit() { return float(random() >> 8) / 16777216.0f; }
  float wave(float phase) const {
    unsigned i = unsigned(phase);
    return sine[i] + (sine[i + 1] - sine[i]) * (phase - i);
  }
  void note(int midi, float seconds, float attack, float release, float gain, float color) {
    for (auto& v : voices) if (v.duration == 0) {
      v = Voice{};
      v.duration = uint32_t(seconds * rate);
      v.attack = uint32_t(attack * rate);
      v.release = uint32_t(release * rate);
      v.gain = gain; v.color = color;
      float hz = 440.0f * std::pow(2.0f, (midi - 69) / 12.0f);
      v.step[0] = hz * 2048.0f / rate;
      // Barely unequal overtones breathe slowly without detuning the melody.
      v.step[1] = v.step[0] * 2.0f + (unit() - 0.5f) * 0.12f * 2048.0f / rate;
      v.step[2] = v.step[0] * 3.0f;
      return;
    } // A full ensemble rests rather than cutting off an existing voice.
  }
  void score() {
    // D major pentatonic upper voices remain compatible across these roots.
    static const int roots[] = {38, 35, 43, 33, 38, 43, 35, 33};
    static const int degrees[] = {62, 64, 66, 69, 71};
    unsigned chapter = unsigned(clock / (uint64_t(rate) * 96)) % 8;
    if (clock >= nextLow) {
      note(roots[chapter], 31 + unit() * 8, 7, 13, 0.095f, 0.19f);
      nextLow = clock + uint64_t((27 + unit() * 7) * rate);
    }
    if (clock >= nextMiddle) {
      int direction = int(random() % 3) - 1;
      lastDegree = unsigned(std::max(0, std::min(4, int(lastDegree) + direction)));
      note(degrees[lastDegree], 19 + unit() * 9, 4 + unit() * 3, 10, 0.072f, 0.12f);
      nextMiddle = clock + uint64_t((9 + unit() * 8) * rate);
    }
    if (clock >= nextHigh) {
      // Phrases breathe: occasional unanswered spaces instead of constant fills.
      if (random() % 5 != 0) {
        unsigned d = (lastDegree + 2 + random() % 2) % 5;
        note(degrees[d] + 12, 10 + unit() * 7, 1.2f, 7, 0.034f, 0.055f);
      }
      nextHigh = clock + uint64_t((15 + unit() * 16) * rate);
    }
  }
 public:
  explicit Engine(uint32_t seed = 0x6c696665) : rng(seed ? seed : 1) {
    for (unsigned i = 0; i <= 2048; ++i) sine[i] = std::sin(2 * pi * i / 2048);
  }
  void setPlaying(bool playing) { target = playing ? 1.0f : 0.0f; }
  uint64_t frames() const { return clock; }
  unsigned activeVoices() const { unsigned n = 0; for (auto& v : voices) n += v.duration != 0; return n; }
  float sample() {
    score(); ++clock;
    float dry = 0;
    for (auto& v : voices) if (v.duration) {
      float envelope = 1;
      if (v.age < v.attack) {
        float t = float(v.age) / v.attack;
        envelope = t * t * (3 - 2 * t);
      } else if (v.age > v.duration - v.release) {
        float t = float(v.duration - v.age) / v.release;
        envelope = t * t * (3 - 2 * t);
      }
      float tone = wave(v.phase[0]) + v.color * wave(v.phase[1]) + v.color * 0.28f * wave(v.phase[2]);
      dry += tone * envelope * v.gain;
      for (unsigned p = 0; p < 3; ++p) {
        v.phase[p] += v.step[p];
        if (v.phase[p] >= 2048) v.phase[p] -= 2048;
      }
      if (++v.age >= v.duration) v.duration = 0;
    }
    float wet = 0;
    for (unsigned j = 0; j < 4; ++j) {
      float delayed = comb[j][ci[j]];
      damping[j] += 0.24f * (delayed - damping[j]);
      comb[j][ci[j]] = dry * 0.19f + damping[j] * 0.86f;
      wet += delayed * 0.25f;
      if (++ci[j] == lengths[j]) ci[j] = 0;
    }
    float a = ap1[ai]; ap1[ai] = wet + a * 0.5f; wet = a - ap1[ai] * 0.5f;
    if (++ai == ap1.size()) ai = 0;
    float b = ap2[bi]; ap2[bi] = wet + b * 0.5f; wet = b - ap2[bi] * 0.5f;
    if (++bi == ap2.size()) bi = 0;
    float mix = dry * 0.82f + wet * 0.75f;
    float clean = mix - dcIn + 0.999f * dcOut;
    dcIn = mix; dcOut = clean;
    level += (target - level) / (rate * 0.7f);
    // Smooth bounded saturation, leaving substantial headroom in normal use.
    float x = clean * level * 1.8f;
    return x / (1 + std::abs(x));
  }
  void render(int16_t* output, unsigned count) {
    for (unsigned i = 0; i < count; ++i) output[i] = int16_t(sample() * 32767);
  }
};
}
