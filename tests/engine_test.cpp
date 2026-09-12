// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/Garden.h"
#include <cassert>
#include <iostream>
#include <memory>

int main() {
  unsigned charactersSeen=0, intervalsSeen=0;
  unsigned familiesSeen = 0, keysSeen = 0, modesSeen = 0;
  bool smearSeen=false, cleanSeen=false, intenseSeen=false, quietSeen=false;
  for (uint32_t seed : {1u, 0x6c696665u, 0xffffffffu}) {
    auto engine = std::unique_ptr<garden::Engine>(new garden::Engine(seed));
    float previous = 0, peak = 0, jump = 0;
    double energy = 0;
    for (unsigned i = 0; i < garden::rate * 600; ++i) {
      // Exercise live pattern and delay changes, including quick repeated taps.
      if (i && i % (garden::rate * 13) == 0) engine->newVariation();
      if (i % (garden::rate * 13) == garden::rate / 10) engine->newVariation();
      float s = engine->sample();
      assert(std::isfinite(s) && std::abs(s) < 0.8f);
      smearSeen |= std::abs(engine->currentSmearFrames())>1;
      cleanSeen |= !engine->hasSmear();
      intenseSeen |= engine->currentFeedback()>0.65f;
      quietSeen |= engine->currentFeedback()<0.3f;
      assert(std::abs(engine->currentSmearFrames())<900);
      assert(engine->delayFrames() < 48000 && engine->secondDelayFrames() < 48000);
      assert(engine->currentFeedback() > 0 && engine->currentFeedback() < 0.79f);
      assert(engine->currentDelayMix() > 0 && engine->currentDelayMix() < 0.65f);
      assert(engine->notesStayedInKey());
      if (i % 512 == 0) assert(engine->rhythmIsBalanced());
      assert(engine->lowestNote() >= 55 && engine->highestNote() <= 91);
      charactersSeen |= 1u << engine->phraseCharacter();
      intervalsSeen |= 1u << engine->intervalPreference();
      familiesSeen |= 1u << engine->toneFamily();
      keysSeen |= 1u << engine->keyRoot();
      modesSeen |= 1u << engine->keyMode();
      peak = std::max(peak, std::abs(s));
      jump = std::max(jump, std::abs(s - previous));
      previous = s; energy += s * s;
    }
    // Bright upper partials legitimately have steeper sample slopes than Study 01.
    // Keep a discontinuity bound alongside the independent headroom check.
    assert(peak > 0.05f && jump < 0.30f);
    assert(std::sqrt(energy / (garden::rate * 600)) > 0.01);
    engine->setPlaying(false);
    for (unsigned i = 0; i < garden::rate * 12; ++i) previous = engine->sample();
    assert(std::abs(previous) < 0.00001f);
    engine->setPlaying(true);
    energy = 0;
    for (unsigned i = 0; i < garden::rate * 12; ++i) { float s = engine->sample(); energy += s*s; }
    assert(energy > 1);
    std::cout << "seed " << seed << ": ten-minute stability, headroom, fade/resume passed; peak=" << peak << " jump=" << jump << '\n';
  }
  auto a = std::unique_ptr<garden::Engine>(new garden::Engine(42));
  auto b = std::unique_ptr<garden::Engine>(new garden::Engine(42));
  int16_t block[512];
  for (unsigned i = 0; i < 300; ++i) {
    a->render(block,512);
    for (auto s : block) assert(s == int16_t(b->sample() * 32767));
  }
  std::cout << "Seed reproducibility and block rendering passed\n";
  assert(smearSeen && cleanSeen && intenseSeen && quietSeen);
  assert(charactersSeen==63 && intervalsSeen==7);
  assert(familiesSeen == 127 && keysSeen == 4095 && modesSeen == 7);
  auto c = std::unique_ptr<garden::Engine>(new garden::Engine(42));
  auto firstPattern = c->patternHash();
  auto firstFamily = c->toneFamily(), firstKey = c->keyRoot();
  c->newVariation();
  for (unsigned i = 0; i < garden::rate; ++i) c->sample();
  assert(c->variation() == 2 && c->patternHash() != firstPattern);
  assert(c->toneFamily() != firstFamily && c->keyRoot() != firstKey);
  const float ratios[] = {1.5f, 4.0f/3, 1.0f, 0.75f};
  assert(std::abs(float(c->delayFrames()) - c->tickFrames() * ratios[c->delayType()]) <= 0.5f);
  float firstMix=c->currentDelayMix(), firstFeedback=c->currentFeedback();
  for(unsigned i=0;i<garden::rate*10;++i) c->sample();
  assert(std::abs(c->currentDelayMix()-firstMix)>0.001f);
  assert(std::abs(c->currentFeedback()-firstFeedback)>0.001f);
  std::cout << "New variation changes motif, voice and tonic; all seven voices, twelve tonics and three modes covered; delay matches musical tick\n";
}
