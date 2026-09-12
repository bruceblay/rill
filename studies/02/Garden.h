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
  uint64_t clock = 0, nextLow = 0, nextMiddle = rate / 5, nextHigh = rate * 3;
  unsigned phraseStep = 0, patternIndex = 0;
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
      v.release = std::min(uint32_t(release * rate), v.duration - v.attack);
      v.gain = gain; v.color = color;
      float hz = 440.0f * std::pow(2.0f, (midi - 69) / 12.0f);
      v.step[0] = hz * 2048.0f / rate;
      // Barely unequal overtones breathe slowly without detuning the melody.
      v.step[1] = v.step[0] * 2.0f + (unit() - 0.5f) * 0.12f * 2048.0f / rate;
      v.step[2] = v.step[0] * 4.0f;
      return;
    } // A full ensemble rests rather than cutting off an existing voice.
  }
  void score() {
    // Related open voicings share notes while the arpeggio gently changes shape.
    static const int roots[] = {62, 59, 67, 64};
    static const int chords[4][4] = {
      {74, 78, 81, 86}, {71, 74, 78, 83},
      {71, 74, 81, 86}, {74, 76, 81, 83}
    };
    static const unsigned patterns[3][8] = {
      {0, 1, 2, 1, 3, 2, 1, 2},
      {0, 2, 1, 3, 2, 1, 2, 0},
      {2, 1, 0, 1, 2, 3, 2, 1}
    };
    unsigned chapter = unsigned(clock / (uint64_t(rate) * 48)) % 4;
    if (clock >= nextLow) {
      note(roots[chapter], 3.8f, 0.025f, 3.775f, 0.11f, 0.36f);
      nextLow = clock + uint64_t((5.5f + unit() * 2) * rate);
    }
    if (clock >= nextMiddle) {
      unsigned degree = patterns[patternIndex][phraseStep];
      float accent = phraseStep == 0 ? 1.0f : 0.80f + unit() * 0.16f;
      note(chords[chapter][degree], 2.1f + unit() * 0.45f,
           0.008f, 2.6f, 0.16f * accent, 0.48f);
      // Eight-note phrases, with a little breath before the next phrase.
      float interval = 0.47f + unit() * 0.08f;
      if (++phraseStep == 8) {
        phraseStep = 0;
        patternIndex = random() % 3;
        interval += 0.65f + unit() * 0.7f;
      }
      nextMiddle = clock + uint64_t(interval * rate);
    }
    if (clock >= nextHigh) {
      note(chords[chapter][1] + 12, 1.5f, 0.006f, 1.494f, 0.045f, 0.20f);
      nextHigh = clock + uint64_t((6 + unit() * 6) * rate);
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
        envelope = t * t * t;
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
      comb[j][ci[j]] = dry * 0.19f + damping[j] * 0.78f;
      wet += delayed * 0.25f;
      if (++ci[j] == lengths[j]) ci[j] = 0;
    }
    float a = ap1[ai]; ap1[ai] = wet + a * 0.5f; wet = a - ap1[ai] * 0.5f;
    if (++ai == ap1.size()) ai = 0;
    float b = ap2[bi]; ap2[bi] = wet + b * 0.5f; wet = b - ap2[bi] * 0.5f;
    if (++bi == ap2.size()) bi = 0;
    float mix = dry * 0.95f + wet * 0.35f;
    float clean = mix - dcIn + 0.999f * dcOut;
    dcIn = mix; dcOut = clean;
    level += (target - level) / (rate * 0.7f);
    // Smooth bounded saturation, leaving substantial headroom in normal use.
    float x = clean * level * 2.4f;
    return x / (1 + std::abs(x));
  }
  void render(int16_t* output, unsigned count) {
    for (unsigned i = 0; i < count; ++i) output[i] = int16_t(sample() * 32767);
  }
};
}
