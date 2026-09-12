// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/Garden.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <set>

int main() {
  // Compare note contours independently of timbre/key: parameter changes alone
  // must not masquerade as compositional variety.
  std::set<uint32_t> contours;
  std::set<unsigned> lengths;
  unsigned harmonies=0;
  for(unsigned seed=1;seed<=256;++seed) {
    auto e=std::make_unique<garden::Engine>(seed,2);
    contours.insert(e->patternHash()); lengths.insert(e->phraseSize());
    harmonies|=1u<<e->harmonicBehavior();
    assert(e->rhythmIsBalanced());
  }
  assert(contours.size()>230 && lengths.size()>=8 && harmonies==15);
  // Leave individual pieces running; frequent regeneration in the engine tests
  // does not exercise multi-minute development or independent answering parts.
  for(unsigned seed : {3u,17u,42u,99u}) {
    auto e=std::make_unique<garden::Engine>(seed);
    unsigned activity=0;
    std::set<uint32_t> descendants;
    float peak=0,previous=0,jump=0;
    for(unsigned second=0;second<240;++second) {
      for(unsigned frame=0;frame<garden::rate;++frame) {
        float sample=e->sample();
        assert(std::isfinite(sample));
        peak=std::max(peak,std::abs(sample));
        jump=std::max(jump,std::abs(sample-previous)); previous=sample;
      }
      activity|=1u<<e->activityState(); descendants.insert(e->patternHash());
      assert(e->rhythmIsBalanced() && e->notesStayedInKey());
      assert(e->lowestNote()>=55 && e->highestNote()<=91);
    }
    assert(e->variation()==1 && e->developmentCount()>=3);
    assert(descendants.size()>=3 && e->answerCount()>=5);
    assert((activity & (activity-1))!=0);
    if(e->harmonicBehavior()==1 || e->harmonicBehavior()==2)
      assert(e->harmonyChangeCount()>0);
    assert(peak<0.8f && jump<0.3f);
    std::cout << "seed " << seed << ": " << descendants.size() << " contours, "
      << e->developmentCount() << " developments, " << e->answerCount()
      << " answer notes, peak=" << peak << " jump=" << jump << '\n';
  }
  std::cout << contours.size() << "/256 unique initial contours; " << lengths.size() << " phrase lengths\n";
}
