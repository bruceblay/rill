// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
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
  uint32_t performanceRng = 1;
  float phraseLevel = 1;
  uint32_t scoreRng = 1;
  uint32_t scoreRandom() { scoreRng ^= scoreRng << 13; scoreRng ^= scoreRng >> 17; scoreRng ^= scoreRng << 5; return scoreRng; }
  float scoreUnit() { return float(scoreRandom() >> 8) / 16777216.0f; }
  std::array<int8_t,16> original{}, answer{};
  unsigned phraseTicks = 32, developAt = 2, activityAt = 4, harmonyAt = 3;
  unsigned activity = 1, harmonyStyle = 0, harmonicRoot = 0, homeRoot = 0;
  unsigned answerLength = 3, answerStep = 0, answerPeriod = 20;
  unsigned developments = 0, answersPlayed = 0, harmonyChanges = 0;
  int previousSupport = 60, lastLead = 72;
  uint64_t nextAnswer = 0, lastLeadAt = 0;
  float activityLevel = 1, activityTarget = 1;
  unsigned phraseBeats = 8, preferredLeap = 1, landing = 0;
  bool upward = true;
  float performanceUnit() {
    performanceRng ^= performanceRng << 13; performanceRng ^= performanceRng >> 17; performanceRng ^= performanceRng << 5;
    return float(performanceRng >> 8) / 16777216.0f;
  }
  float touchVariation() { return 0.95f + 0.10f * performanceUnit(); }
  uint64_t clock = 0, nextTick = 0;
  unsigned phraseStep = 0, phraseCount = 0, phraseLength = 16;
  std::array<int8_t, 16> melody{};
  std::array<float, 16> accents{}, articulation{};
  std::array<uint8_t, 16> rhythm{};
  unsigned character = 0, intervalStyle = 0;
  unsigned family = 0, tonic = 2, mode = 0;
  int initialFamily = -1;
  float brightness = 0.48f, strike = 0.008f, overtoneLife = 0.4f;
  float voiceGain = 1, fmDepth = 0;
  uint32_t transition = 0;
  static constexpr uint32_t fadeFrames = rate / 3;
  unsigned lowestMidi = 127, highestMidi = 0;
  bool tonalViolation = false;
  unsigned tempo = 72, delayMode = 0, generation = 0;
  uint32_t tickSamples = rate * 30 / 72;
  // One 1.5-second mono delay. Fixed allocation keeps the audio task predictable.
  std::array<int16_t, 48000> echo{};
  unsigned echoWrite = 0, delaySamples = 20000, secondDelaySamples = 30000;
  float feedback = 0.35f, delayLevel = 0.42f, echoLowpass = 0;
  float lfo1 = 0, lfo2 = 0, lfoStep1 = 0, lfoStep2 = 0;
  float smearPhase = 0, smearStep = 0, smearDepth = 0, smearNow = 0, smearTarget = 0;
  bool steppedFeedback = false;
  uint64_t feedbackBeat = UINT64_MAX;
  uint32_t feedbackRng = 1;
  float heldFeedback = 0.3f;
  float feedbackDepth = 0.07f, mixDepth = 0.10f;
  float feedbackNow = 0.3f, mixNow = 0.4f, toneNow = 0.4f, balanceNow = 0.5f, sendNow = 1;
  float feedbackTarget = 0.3f, mixTarget = 0.4f, toneTarget = 0.4f, balanceTarget = 0.5f, sendTarget = 1;
  uint64_t effectStart = 0;
  uint8_t sendMask = 0xb6;
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
    character = generation ? (character + 1 + random() % 5) % 6 : random() % 6;
    intervalStyle = random() % 3;
    ++generation;
    tempo = 62 + random() % 35;
    tickSamples = 2 * uint32_t(std::lround(float(rate) * 15 / tempo));
    phraseLength = 7 + random() % 10;
    phraseStep = phraseCount = 0;
    // Performance has its own stream, preserving the generated score and effects.
    performanceRng = (rng ^ 0xa341316cu) | 1u;
    phraseLevel = 1;
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
    delayMode = random() % 4;
    static const float taps[4][2] = {{1.5f,2.5f},{4.0f/3,8.0f/3},{1,3},{0.75f,2.25f}};
    delaySamples = unsigned(std::lround(tickSamples * taps[delayMode][0]));
    secondDelaySamples = unsigned(std::lround(tickSamples * taps[delayMode][1]));
    feedback = 0.25f + unit() * 0.14f;
    delayLevel = 0.34f + unit() * 0.14f;
    // Independent effect randomization does not consume any additional score RNG.
    uint32_t fx = rng ^ 0x9e3779b9u;
    auto fxUnit = [&fx]() { fx ^= fx<<13; fx ^= fx>>17; fx ^= fx<<5; return float(fx>>8)/16777216.0f; };
    lfo1=fxUnit()*2048; lfo2=fxUnit()*2048;
    lfoStep1=2048.0f*128/(rate*(14+fxUnit()*24));
    lfoStep2=2048.0f*128/(rate*(29+fxUnit()*42));
    feedbackDepth=0.045f+fxUnit()*0.05f;
    mixDepth=0.06f+fxUnit()*0.07f;
    static const uint8_t masks[] = {0xb6,0xdb,0xad,0xeb};
    sendMask=masks[unsigned(fxUnit()*4)];
    effectStart=clock;
    smearPhase=fxUnit()*2048;
    smearStep=2048.0f*128/(rate*(3.0f+fxUnit()*4.0f));
    smearDepth=fxUnit()<0.45f ? rate*(0.008f+fxUnit()*0.020f) : 0;
    smearNow=smearTarget=0;
    steppedFeedback=fxUnit()<0.5f;
    feedbackRng=fx|1u; feedbackBeat=UINT64_MAX; heldFeedback=feedback;
    scoreRng = (rng ^ 0x51ed270bu) | 1u;
    preferredLeap = 1 + intervalStyle;
    landing = (scoreRandom()%3)*2;
    upward = scoreRandom() & 1;
    phraseBeats = 4 + scoreRandom()%7;
    harmonicRoot = 0;
    homeRoot = scoreRandom()%3;
    harmonyStyle = scoreRandom()%4;
    previousSupport = foldPitch(scaleNote(0,0),55,72);
    lastLead = melodyPitch(0,landing);
    lastLeadAt = clock;
    activity = scoreRandom()%3;
    activityTarget = activityLevel = activity == 0 ? 0.72f : 1.0f;
    developAt = 2 + scoreRandom()%4;
    activityAt = 3 + scoreRandom()%5;
    harmonyAt = 2 + scoreRandom()%5;
    developments = answersPlayed = harmonyChanges = 0;
    answerPeriod = 4 * (5 + 2*(scoreRandom()%3)); // Five, seven or nine beats.
    nextAnswer = clock + uint64_t(tickSamples/2)*(answerPeriod+3);
    answerStep = 0;
    composePhrase();
    original = melody;
    makeAnswer();
  }
  void composePhrase() {
    int degree = upward ? 0 : 6;
    for (unsigned i=0;i<phraseLength;++i) {
      // A generated contour, with local steps and occasional characteristic leaps.
      int direction = upward ? 1 : -1;
      if (character==1) direction=-1;
      if (character==2) direction=degree>int(landing) ? -1 : 1;
      if (character==3 && i>=phraseLength/2) direction=-direction;
      if (character==5 && i>phraseLength/3) direction=-direction;
      if (scoreUnit()<0.24f) direction=-direction;
      unsigned leap = scoreUnit()<0.72f ? 1 : preferredLeap+1;
      degree=std::max(0,std::min(9,degree+direction*int(leap)));
      if (character==0 && scoreUnit()<0.4f) degree=int(landing);
      melody[i] = (i && i+1<phraseLength && scoreUnit()<0.18f) ? -1 : degree;
    }
    melody[phraseLength-1]=int8_t(landing);
    composeRhythm();
  }
  void composeRhythm() {
    // Distribute a chosen span among events; no repeated four-step rhythm cells.
    phraseTicks=std::max(phraseLength+2,phraseBeats*4);
    rhythm.fill(1);
    for(unsigned remaining=phraseTicks-phraseLength;remaining;--remaining) {
      unsigned i=scoreRandom()%phraseLength;
      if(scoreUnit()<0.3f) i=phraseLength-1; // Give the ending room to breathe.
      ++rhythm[i];
    }
    for(unsigned i=0;i<phraseLength;++i) {
      accents[i]=0.73f+scoreUnit()*0.22f;
      if(i==0 || i+1==phraseLength) accents[i]=0.96f;
      articulation[i]=0.5f+scoreUnit()*0.85f;
      if(i+1==phraseLength) articulation[i]=1.3f+scoreUnit()*0.35f;
    }
  }
  void makeAnswer() {
    answerLength=2+scoreRandom()%3;
    for(unsigned i=0;i<answerLength;++i) {
      int d=melody[(phraseLength-1-i)%phraseLength];
      answer[i]=int8_t(d<0 ? landing : unsigned(d));
    }
  }
  void developPhrase() {
    // Change a whole musical idea, preserving some of its history.
    unsigned operation=scoreRandom()%6;
    if(operation==0) { // A changed ending, approaching the landing by step.
      melody[phraseLength-2]=int8_t(std::min(9u,landing+1+scoreRandom()%2));
      melody[phraseLength-1]=int8_t(landing);
    } else if(operation==1) {
      composeRhythm(); // Remember the pitches but change their delivery.
    } else if(operation==2) {
      int shift=scoreRandom()%2 ? 1 : -1;
      for(unsigned i=1;i+1<phraseLength;++i)
        if(melody[i]>=0) melody[i]=int8_t(std::max(0,std::min(9,int(melody[i])+shift)));
    } else if(operation==3) {
      // Promote the answer into the opening of the next phrase.
      for(unsigned i=0;i<answerLength;++i) melody[i]=answer[i];
      unsigned i=1+scoreRandom()%(phraseLength-2);
      melody[i]=melody[i]<0 ? int8_t(landing) : -1;
    } else if(operation==4) {
      for(unsigned i=0;i<phraseLength/2;++i) melody[i]=original[i];
      composeRhythm();
    } else {
      // An occasional new descendant, retaining the previous opening as a link.
      int8_t opening=melody[0];
      composePhrase(); melody[0]=opening;
      original=melody;
    }
    makeAnswer();
    ++developments;
  }
  int melodyPitch(unsigned root,unsigned degree) const {
    return foldPitch(scaleNote(root+degree,0),60,91);
  }
  int supportPitch(unsigned root) const {
    int best=previousSupport, distance=100;
    for(unsigned d : {0u,2u,4u}) for(int octave=-1;octave<=1;++octave) {
      int candidate=scaleNote(root+d,0)+12*octave;
      int delta=std::abs(candidate-previousSupport);
      if(candidate>=55 && candidate<=72 && delta<distance) {best=candidate;distance=delta;}
    }
    return best;
  }
  void beginPhrase() {
    if(phraseCount>=developAt) {
      developPhrase(); developAt=phraseCount+2+scoreRandom()%5;
    }
    if(phraseCount>=activityAt) {
      activity=(activity+1+scoreRandom()%3)%4;
      activityTarget=activity==0 ? 0.72f : activity==3 ? 0.55f : 1.0f;
      activityAt=phraseCount+(activity==3 ? 1 : 2+scoreRandom()%5);
    }
    if(phraseCount>=harmonyAt) {
      unsigned before=harmonicRoot;
      if(harmonyStyle==1) harmonicRoot=harmonicRoot==0 ? 3+(homeRoot%3) : 0;
      if(harmonyStyle==2) {
        static const unsigned moves[]={2,3,4,5};
        harmonicRoot=(harmonicRoot+moves[scoreRandom()%4])%7;
      }
      // Styles 0 and 3 retain a tonal center; 3 supports a recurring pedal.
      harmonyChanges+=before!=harmonicRoot;
      harmonyAt=phraseCount+2+scoreRandom()%6;
    }
    phraseLevel=0.5f*phraseLevel+0.5f*(0.94f+0.12f*performanceUnit());
    if(activity!=3 && scoreUnit()<0.65f) {
      previousSupport=harmonyStyle==3 ? foldPitch(scaleNote(0,0),55,67) : supportPitch(harmonicRoot);
      note(previousSupport,2.1f,0.025f,2.075f,0.055f*phraseLevel*touchVariation(),brightness*0.65f);
    }
  }
  void score() {
    // Answering parts have their own recurrence, with a gap after lead attacks.
    if(clock>=nextAnswer) {
      if(clock-lastLeadAt<uint64_t(tickSamples/2)) {
        nextAnswer=lastLeadAt+tickSamples/2;
      } else {
        if(activity==2 || (activity!=3 && scoreUnit()<0.48f)) {
          unsigned d=unsigned(answer[answerStep]);
          int pitch=foldPitch(melodyPitch(harmonicRoot,d)+12,72,91);
          // Avoid a close semitone against a recently sounding lead note.
          int interval=std::abs(pitch-lastLead)%12;
          if(interval!=1 && interval!=11) {
            note(pitch,decaySeconds*0.75f,strike,decaySeconds*0.75f-strike,
                 0.045f*phraseLevel*touchVariation(),brightness*0.8f);
            ++answersPlayed;
          }
        }
        if(++answerStep<answerLength) nextAnswer+=uint64_t(tickSamples)*(1+scoreRandom()%3);
        else {answerStep=0;nextAnswer+=uint64_t(tickSamples/2)*answerPeriod;}
      }
    }
    if(clock<nextTick) return;
    if(phraseStep==0) beginPhrase();
    activityLevel+=0.25f*(activityTarget-activityLevel);
    unsigned elapsed=0;
    for(unsigned i=0;i<phraseStep;++i) elapsed+=rhythm[i];
    float position=float(elapsed)/phraseTicks;
    float expression=phraseLevel*(0.94f+0.12f*(4*position*(1-position)))*activityLevel;
    int degree=melody[phraseStep];
    bool speak=degree>=0;
    if(activity==0 && phraseStep!=0 && phraseStep+1!=phraseLength && scoreUnit()<0.30f) speak=false;
    if(activity==3 && phraseStep!=0 && phraseStep+1!=phraseLength) speak=false;
    if(speak) {
      int pitch=melodyPitch(harmonicRoot,unsigned(degree));
      // Occasionally place the whole answering half in a different register.
      if(character==3 && phraseCount%3==1 && phraseStep>=phraseLength/2)
        pitch=foldPitch(pitch+12,72,91);
      float duration=decaySeconds*(0.32f+0.18f*rhythm[phraseStep])*articulation[phraseStep]
                     *(0.90f+0.20f*performanceUnit());
      if(activity==2) duration*=0.8f;
      duration=std::max(0.16f,std::min(3.8f,duration));
      note(pitch,duration,strike,duration-strike,0.15f*accents[phraseStep]*expression*touchVariation(),brightness);
      lastLead=pitch;lastLeadAt=clock;
    }
    nextTick+=uint64_t(tickSamples/2)*rhythm[phraseStep];
    if(++phraseStep==phraseLength) {phraseStep=0;++phraseCount;}
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
  unsigned phraseCharacter() const { return character; }
  unsigned intervalPreference() const { return intervalStyle; }
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
  unsigned secondDelayFrames() const { return secondDelaySamples; }
  float currentSmearFrames() const { return smearNow; }
  bool hasSmear() const { return smearDepth > 0; }
  float currentFeedback() const { return feedbackNow; }
  float currentDelayMix() const { return mixNow; }
  unsigned tickFrames() const { return tickSamples; }
  unsigned developmentCount() const { return developments; }
  unsigned answerCount() const { return answersPlayed; }
  unsigned harmonyChangeCount() const { return harmonyChanges; }
  unsigned harmonicBehavior() const { return harmonyStyle; }
  unsigned activityState() const { return activity; }
  unsigned phraseSize() const { return phraseLength; }
  bool rhythmIsBalanced() const {
    unsigned sum=0;
    for(unsigned i=0;i<phraseLength;++i) {
      if(rhythm[i]<1 || rhythm[i]>40) return false;
      sum+=rhythm[i];
    }
    return sum==phraseTicks && phraseLength>=7 && phraseLength<=16 && tickSamples%2==0;
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
    if ((clock & 127) == 0) {
      lfo1 += lfoStep1; if(lfo1>=2048) lfo1-=2048;
      lfo2 += lfoStep2; if(lfo2>=2048) lfo2-=2048;
      float slow=wave(lfo1), slower=wave(lfo2);
      smearPhase+=smearStep; if(smearPhase>=2048) smearPhase-=2048;
      // A brief, smoothly opening window in the slower LFO cycle.
      float window=std::max(0.0f,(slower-0.65f)/0.35f);
      window=window*window*(3-2*window);
      smearTarget=smearDepth*wave(smearPhase)*window;
      uint64_t beat=(clock-effectStart)/(uint64_t(tickSamples)*8);
      if (beat!=feedbackBeat) {
        feedbackBeat=beat;
        feedbackRng^=feedbackRng<<13; feedbackRng^=feedbackRng>>17; feedbackRng^=feedbackRng<<5;
        float choice=float(feedbackRng>>8)/16777216.0f;
        // Longer repeats occupy at most one consecutive four-beat window.
        heldFeedback=(heldFeedback<0.6f && choice>0.70f) ? 0.66f+0.12f*(choice-.7f)/.3f : 0.22f+0.24f*choice;
      }
      float crest=std::max(0.0f,(slow-.25f)/.75f);
      feedbackTarget=steppedFeedback ? heldFeedback : std::min(0.78f,feedback+feedbackDepth*slow+0.30f*crest*crest);
      mixTarget=(delayLevel+mixDepth*slower)*(activity==2 ? 0.85f : 1.0f);
      toneTarget=0.36f+0.16f*slower;
      balanceTarget=0.5f+0.22f*slow;
      unsigned step=unsigned((clock-effectStart)/tickSamples)%8;
      sendTarget=(sendMask & (1u<<step)) ? (activity==2 ? 0.72f : 1.0f) : 0.0f;
    }
    // Smooth control motion, including the rhythmic send windows.
    feedbackNow+=(feedbackTarget-feedbackNow)*0.001f;
    mixNow+=(mixTarget-mixNow)*0.001f;
    toneNow+=(toneTarget-toneNow)*0.001f;
    balanceNow+=(balanceTarget-balanceNow)*0.001f;
    sendNow+=(sendTarget-sendNow)*0.002f;
    smearNow+=(smearTarget-smearNow)*0.001f;
    auto readEcho = [this](float delay) {
      delay=std::max(1.0f,std::min(float(echo.size()-2),delay));
      unsigned whole=unsigned(delay);
      float fraction=delay-whole;
      unsigned index=(echoWrite+echo.size()-whole)%echo.size();
      unsigned older=(index+echo.size()-1)%echo.size();
      return echo[index]+(echo[older]-echo[index])*fraction;
    };
    float delayed=(readEcho(delaySamples+smearNow)*(1-balanceNow)
                  +readEcho(secondDelaySamples-smearNow*0.7f)*balanceNow)/32768.0f;
    echoLowpass+=toneNow*(delayed-echoLowpass);
    float echoInput=dry*sendNow+echoLowpass*feedbackNow;
    echo[echoWrite]=int16_t(std::max(-0.98f,std::min(0.98f,echoInput))*32767);
    if(++echoWrite==echo.size()) echoWrite=0;
    float mix = dry * 0.95f + wet * 0.28f + delayed * mixNow;
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
