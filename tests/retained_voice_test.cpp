// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#define garden previous
#include "fixtures/voices-before.h"
#undef garden
// Historical check for Study 07: Study 08 deliberately changes the wet mix.
#include "fixtures/voices-after.h"
#include <memory>
#include <cassert>
#include <iostream>
int main() {
  for(unsigned family : {2u,4u,5u}) {
    for(uint32_t seed=1;;++seed) {
      auto old=std::unique_ptr<previous::Engine>(new previous::Engine(seed));
      if(old->toneFamily()!=family) continue;
      auto now=std::unique_ptr<garden::Engine>(new garden::Engine(seed,int(family)));
      for(unsigned i=0;i<garden::rate*60;++i) assert(old->sample()==now->sample());
      std::cout<<"Family "<<family<<": 60 seconds Study 07 sample-identical to Study 06\n";
      break;
    }
  }
}
