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
    float third = 0, transient = 1, transientDecay = 1, fm = 0;
    unsigned family = 0;
    float modalA[4] = {}, modalB[4] = {}, modalY1[4] = {}, modalY2[4] = {};
    float filter1 = 0, filter2 = 0, filter3 = 0, filter4 = 0;
    float cutoff = 0, gate = 1, gateDecay = 1;
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
  uint64_t clock = 0, nextTick = 0;
  unsigned phraseStep = 0, phraseCount = 0, phraseLength = 16;
  std::array<int8_t, 16> melody{};
  std::array<float, 16> accents{}, articulation{};
  std::array<uint8_t, 16> rhythm{};
  unsigned family = 0, tonic = 2, mode = 0;
  int initialFamily = -1;
  float brightness = 0.48f, strike = 0.008f, overtoneLife = 0.4f;
  float voiceGain = 1, fmDepth = 0;
  uint32_t transition = 0;
  static constexpr uint32_t fadeFrames = rate / 3;
  unsigned lowestMidi = 127, highestMidi = 0;
  bool tonalViolation = false;
  unsigned tempo = 72, delayMode = 0, generation = 0, harmonyOffset = 0;
  uint32_t tickSamples = rate * 30 / 72;
  // One 1.5-second mono delay. Fixed allocation keeps the audio task predictable.
  std::array<int16_t, 48000> echo{};
  unsigned echoWrite = 0, delaySamples = 20000, oldDelaySamples = 20000;
  float delayBlend = 1, feedback = 0.35f, delayLevel = 0.42f, echoLowpass = 0;
  float previousFeedback = 0.35f, previousDelayLevel = 0.42f;
  bool changeRequested = false;
  float decaySeconds = 1.6f;
  float dcIn = 0, dcOut = 0, polish = 0, level = 0, target = 1;
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
      v.gain = gain * voiceGain; v.color = color;
      v.family = family; v.third = color * 0.28f;
      v.transientDecay = std::exp(-1.0f / (rate * overtoneLife));
      v.fm = fmDepth;
      lowestMidi = std::min(lowestMidi, unsigned(midi));
      highestMidi = std::max(highestMidi, unsigned(midi));
      if (!inKey(midi)) tonalViolation = true;
      float hz = 440.0f * std::pow(2.0f, (midi - 69) / 12.0f);
      v.step[0] = hz * 2048.0f / rate;
      // Barely unequal overtones breathe slowly without detuning the melody.
      v.step[1] = v.step[0] * 2.0f + (unit() - 0.5f) * 0.12f * 2048.0f / rate;
      v.step[2] = v.step[0] * 4.0f;
      if (family == 1) { v.step[2] = v.step[0] * 3; v.third *= 0.5f; }
      if (family == 2) { v.step[1] = v.step[0] * 3; v.step[2] = v.step[0] * 7; }
      if (family == 3) { v.step[1] = v.step[0] * 2; v.step[2] = v.step[0] * 3; v.third *= 0.4f; }
      if (family == 4) { v.step[1] = v.step[0] * 2; v.step[2] = v.step[0] * 3; v.third *= 1.5f; }
      if (family == 5 || family == 6) { v.step[1] = v.step[0] * 2; v.step[2] = v.step[0] * 3; }
      if (family == 0 || family == 6) {
        // Per-note darkening: amplitude and spectral decay move together.
        v.gateDecay = std::exp(-1.0f / (rate * overtoneLife));
        v.cutoff = std::min(0.85f, 1.0f - std::exp(-2*pi*hz*(1.2f+color*3.0f)/rate));
      }
      if (family == 1 || family == 3) {
        // A struck body represented by independently damped vibration modes.
        static const float ratios[2][4] = {{1,4.0f,9.7f,16.0f},{1,2.01f,2.76f,4.07f}};
        static const float weights[2][4] = {{1,0.46f,0.15f,0.045f},{1,0.27f,0.17f,0.085f}};
        static const float dampingTime[2][4] = {{0.65f,0.20f,0.075f,0.035f},{0.95f,0.65f,0.40f,0.23f}};
        unsigned model = family == 1 ? 0 : 1;
        for (unsigned m=0;m<4;++m) {
          float freq=hz*ratios[model][m];
          if(freq > rate*0.40f) continue;
          float radius=std::exp(-1.0f/(rate*seconds*dampingTime[model][m]));
          float omega=2*pi*freq/rate;
          float amplitude=weights[model][m]*(m ? color : 1.0f);
          v.modalA[m]=2*radius*std::cos(omega); v.modalB[m]=radius*radius;
          v.modalY2[m]=-amplitude*std::sin(omega);
        }
      }
      return;
    } // A full ensemble rests rather than cutting off an existing voice.
  }
  void generate() {
    // No adjacent repeats of either the instrument family or tonic.
    family = generation ? (family + 1 + random() % 6) % 7 : random() % 7;
    if (initialFamily >= 0 && initialFamily < 7) { family = unsigned(initialFamily); initialFamily = -1; }
    tonic = generation ? (tonic + 1 + random() % 11) % 12 : random() % 12;
    mode = random() % 3;
    ++generation;
    tempo = 62 + random() % 35;
    tickSamples = 2 * uint32_t(std::lround(float(rate) * 15 / tempo));
    phraseLength = (random() & 1) ? 16 : 12;
    harmonyOffset = random() % 4;
    phraseStep = phraseCount = 0;
    nextTick = clock;
    // Each family has its own authored envelope and spectral bounds.
    static const float durations[][2] = {{0.65f,1.15f},{1.0f,1.8f},{0.7f,1.2f},{1.6f,2.6f},{0.9f,1.6f},{1.5f,2.3f},{0.9f,1.6f}};
    static const float attacks[][2] = {{0.004f,0.009f},{0.004f,0.010f},{0.004f,0.010f},{0.009f,0.018f},{0.006f,0.016f},{0.018f,0.042f},{0.009f,0.020f}};
    static const float colors[][2] = {{0.22f,0.40f},{0.38f,0.68f},{0.27f,0.44f},{0.30f,0.52f},{0.35f,0.52f},{0.16f,0.26f},{0.25f,0.48f}};
    static const float life[] = {0.13f,0.20f,0.085f,0.8f,0.24f,0.7f,0.25f};
    static const float gains[] = {1.35f,1.45f,1.15f,1.28f,0.96f,1.0f,1.12f};
    decaySeconds = durations[family][0] + unit() * (durations[family][1] - durations[family][0]);
    strike = attacks[family][0] + unit() * (attacks[family][1] - attacks[family][0]);
    brightness = colors[family][0] + unit() * (colors[family][1] - colors[family][0]);
    overtoneLife = life[family] * (0.8f + unit() * 0.4f);
    voiceGain = gains[family];
    fmDepth = family == 5 ? 0.35f + unit() * 0.45f : 0;
    oldDelaySamples = delaySamples;
    previousFeedback = feedback;
    previousDelayLevel = delayLevel;
    delayMode = random() % 3;
    // Dotted eighth, quarter, or dotted quarter: exact multiples of the tick.
    const float ticks[] = {1.5f, 2.0f, 3.0f};
    delaySamples = unsigned(std::lround(tickSamples * ticks[delayMode]));
    feedback = 0.25f + unit() * 0.21f;
    delayLevel = 0.35f + unit() * 0.20f;
    delayBlend = 1.0f;
    // A generated four-note cell is answered and varied across the phrase.
    int cell[4] = {int(random() % 3), 0, 0, 0};
    for (unsigned i = 1; i < 4; ++i)
      cell[i] = (cell[i-1] + 1 + random() % 3) % 5;
    for (unsigned i = 0; i < phraseLength; ++i) {
      int degree = cell[i % 4];
      if (i >= 4 && i % 4 == 2) degree = (degree + 1 + random() % 2) % 5;
      bool rest = i != 0 && (random() % 100 < 23 || i == phraseLength - 1);
      melody[i] = rest ? -1 : degree;
      accents[i] = i % 4 == 0 ? 1.0f : 0.70f + unit() * 0.22f;
      articulation[i] = 0.90f + unit() * 0.17f;
    }
    // Each four-event cell spans exactly two beats. One gentle flourish is
    // guaranteed; the rest predominantly keep the unhurried eighth-note pulse.
    unsigned flourish = random() % (phraseLength / 4);
    for (unsigned cell = 0; cell < phraseLength / 4; ++cell) {
      unsigned shape = cell == flourish || random() % 100 < 25 ? 1 + random() % 4 : 0;
      rhythmCell(cell, shape);
      // Make the guaranteed gesture audible rather than hidden by random rests.
      if (cell == flourish) {
        for (unsigned j = 0; j < 4; ++j)
          if (melody[cell * 4 + j] < 0) melody[cell * 4 + j] = j % 5;
      }
    }
  }
  void rhythmCell(unsigned cell, unsigned shape) {
    static const uint8_t cells[5][4] = {
      {2,2,2,2}, {1,1,2,4}, {3,1,2,2}, {2,1,1,4}, {2,2,3,1}
    };
    for (unsigned i = 0; i < 4; ++i) rhythm[cell * 4 + i] = cells[shape][i];
  }
  void score() {
    if (clock < nextTick) return;
    static const unsigned paths[3][4] = {{0,5,3,4},{0,5,2,6},{0,3,1,4}};
    unsigned chapter = (harmonyOffset + phraseCount / 4) % 4;
    unsigned root = paths[mode][chapter];
    if (phraseStep == 0 && phraseCount % 2 == 0)
      note(foldPitch(scaleNote(root, 0),55,72), 2.8f, 0.025f, 2.775f, 0.075f, brightness * 0.65f);
    static const unsigned voicing[] = {0,2,4,7,8};
    int degree = melody[phraseStep];
    if (degree >= 0) {
      int pitch = foldPitch(scaleNote(root + voicing[degree], 1),67,88);
      float duration = decaySeconds * (0.32f + 0.23f * rhythm[phraseStep]) * articulation[phraseStep];
      float touch = rhythm[phraseStep] == 1 ? 0.88f : 1.0f;
      note(pitch, duration, strike,
           duration - strike, 0.15f * accents[phraseStep] * touch, brightness);
    }
    if (phraseStep == phraseLength / 2 && phraseCount % 3 == 2)
      note(foldPitch(scaleNote(root + 2, 2),76,91), 1.3f, strike, 1.3f - strike, 0.032f, brightness * 0.6f);
    nextTick += (tickSamples / 2) * rhythm[phraseStep];
    if (++phraseStep == phraseLength) {
      phraseStep = 0;
      ++phraseCount;
      // Preserve identity while allowing a small change every four phrases.
      if (phraseCount % 4 == 0) {
        unsigned index = 1 + random() % (phraseLength - 2);
        melody[index] = random() % 5;
        rhythmCell(random() % (phraseLength / 4), 1 + random() % 4);
      }
    }
  }
  static int foldPitch(int midi, int low, int high) {
    while (midi > high) midi -= 12;
    while (midi < low) midi += 12;
    return midi;
  }
  int scaleNote(unsigned degree, unsigned octave) const {
    static const int scales[3][7] = {{0,2,4,5,7,9,11},{0,2,3,5,7,8,10},{0,2,3,5,7,9,10}};
    return 60 + int(tonic) + scales[mode][degree % 7] + 12 * int(degree / 7 + octave);
  }
  void clearSound() {
    for (auto& v : voices) v = Voice{};
    for (auto& line : comb) line.fill(0);
    for (auto& value : damping) value = 0;
    ap1.fill(0); ap2.fill(0); echo.fill(0);
    echoLowpass = dcIn = dcOut = polish = 0;
  }

 public:
  explicit Engine(uint32_t seed = 0x6c696665, int firstFamily = -1) : rng(seed ? seed : 1), initialFamily(firstFamily) {
    for (unsigned i = 0; i <= 2048; ++i) sine[i] = std::sin(2 * pi * i / 2048);
    generate();
  }
  // Called before audio starts, or from the audio producer itself.
  void seed(uint32_t value) { rng = value ? value : 1; generation = 0; generate(); }
  void newVariation() { changeRequested = true; }
  bool inKey(int midi) const {
    int pc = (midi % 12 + 12) % 12;
    for (unsigned i = 0; i < 7; ++i) if (scaleNote(i, 0) % 12 == pc) return true;
    return false;
  }
  bool notesStayedInKey() const { return !tonalViolation; }
  unsigned lowestNote() const { return lowestMidi; }
  unsigned highestNote() const { return highestMidi; }
  unsigned toneFamily() const { return family; }
  unsigned keyRoot() const { return tonic; }
  unsigned keyMode() const { return mode; }
  uint32_t displayInfo() const {
    return (generation << 18) | (delayMode << 16) | (family << 13) | (tonic << 9) | (mode << 7) | tempo;
  }
  unsigned variation() const { return generation; }
  unsigned bpm() const { return tempo; }
  unsigned delayType() const { return delayMode; }
  unsigned delayFrames() const { return delaySamples; }
  unsigned tickFrames() const { return tickSamples; }
  bool rhythmIsBalanced() const {
    bool varied = false;
    for (unsigned cell = 0; cell < phraseLength / 4; ++cell) {
      unsigned sum = 0;
      for (unsigned i = 0; i < 4; ++i) {
        unsigned value = rhythm[cell * 4 + i];
        if (value < 1 || value > 4) return false;
        sum += value; varied |= value != 2;
      }
      if (sum != 8) return false;
    }
    return varied && tickSamples % 2 == 0;
  }
  uint32_t patternHash() const {
    uint32_t hash = 2166136261u;
    for (unsigned i = 0; i < phraseLength; ++i) hash = (hash ^ uint8_t(melody[i])) * 16777619u;
    return hash;
  }
  void setPlaying(bool playing) { target = playing ? 1.0f : 0.0f; }
  uint64_t frames() const { return clock; }
  unsigned activeVoices() const { unsigned n = 0; for (auto& v : voices) n += v.duration != 0; return n; }
  float sample() {
    if (changeRequested && transition == 0) { transition = 2 * fadeFrames; changeRequested = false; }
    float sceneGain = 1;
    if (transition) {
      if (transition == fadeFrames) { clearSound(); generate(); }
      float t = transition > fadeFrames ? float(transition - fadeFrames) / fadeFrames
                                      : float(fadeFrames - transition) / fadeFrames;
      sceneGain = t * t * (3 - 2 * t);
      --transition;
    }
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
      v.transient *= v.transientDecay;
      float transient = 0.12f + 0.88f * v.transient;
      float tone;
      if (v.family == 0) {
        // Buchla-inspired low-pass gate approximation, not a circuit clone.
        v.gate *= v.gateDecay;
        float source=wave(v.phase[0])+0.32f*wave(v.phase[1])+0.12f*wave(v.phase[2]);
        float cutoff=0.045f+v.cutoff*v.gate;
        v.filter1+=cutoff*(source-v.filter1);
        v.filter2+=cutoff*(v.filter1-v.filter2);
        tone=v.filter2*(0.12f+0.88f*v.gate);
      } else if (v.family == 1 || v.family == 3) {
        tone=0;
        for(unsigned m=0;m<4;++m) {
          float y=v.modalA[m]*v.modalY1[m]-v.modalB[m]*v.modalY2[m];
          v.modalY2[m]=v.modalY1[m]; v.modalY1[m]=y; tone+=y;
        }
      } else if (v.family == 5) {
        float phase = v.phase[0] + wave(v.phase[1]) * v.fm * transient * (2048.0f / (2 * pi));
        if (phase < 0) phase += 2048;
        if (phase >= 2048) phase -= 2048;
        tone = wave(phase) + v.third * wave(v.phase[2]) * transient;
      } else if (v.family == 6) {
        // Finite harmonic pulse/saw blend avoids naive discontinuous oscillators.
        v.gate *= v.gateDecay;
        float source=wave(v.phase[0])+v.color*wave(v.phase[1])+0.30f*wave(v.phase[2]);
        float cutoff=0.08f+v.cutoff*(0.25f+0.75f*v.gate);
        float drive=source-v.filter4*0.45f;
        v.filter1+=cutoff*(drive-v.filter1);
        v.filter2+=cutoff*(v.filter1-v.filter2);
        v.filter3+=cutoff*(v.filter2-v.filter3);
        v.filter4+=cutoff*(v.filter3-v.filter4);
        tone=v.filter4*1.25f;
      } else {
        // Original Wood and Wire oscillator, envelope and gain paths unchanged.
        float brightnessEnvelope = transient;
        tone = wave(v.phase[0]) + brightnessEnvelope * (v.color * wave(v.phase[1]) + v.third * wave(v.phase[2]));
      }
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
    unsigned oldRead = (echoWrite + echo.size() - oldDelaySamples) % echo.size();
    unsigned newRead = (echoWrite + echo.size() - delaySamples) % echo.size();
    float delayed = (echo[oldRead] * (1 - delayBlend) + echo[newRead] * delayBlend) / 32768.0f;
    float feedbackNow = previousFeedback + (feedback - previousFeedback) * delayBlend;
    float delayLevelNow = previousDelayLevel + (delayLevel - previousDelayLevel) * delayBlend;
    echoLowpass += 0.6f * (delayed - echoLowpass);
    float echoInput = dry + echoLowpass * feedbackNow;
    echo[echoWrite] = int16_t(std::max(-0.98f, std::min(0.98f, echoInput)) * 32767);
    if (++echoWrite == echo.size()) echoWrite = 0;
    delayBlend = std::min(1.0f, delayBlend + 1.0f / (rate * 0.3f));
    float mix = dry * 0.95f + wet * 0.28f + delayed * delayLevelNow;
    float clean = mix - dcIn + 0.999f * dcOut;
    dcIn = mix; dcOut = clean;
    // Soften the combined upper partials when dry notes and echoes coincide.
    polish += 0.40f * (clean - polish);
    level += (target - level) / (rate * 0.7f);
    // Smooth bounded saturation, leaving substantial headroom in normal use.
    float x = polish * level * sceneGain * 2.4f;
    return x / (1 + std::abs(x));
  }
  void render(int16_t* output, unsigned count) {
    for (unsigned i = 0; i < count; ++i) output[i] = int16_t(sample() * 32767);
  }
};
}
