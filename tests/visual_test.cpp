// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/Light.h"
#include <cassert>
#include <memory>
#include <iostream>
uint32_t hash(const light::Painting& p) {
  uint32_t value=2166136261u;
  for(unsigned i=0;i<240*135;++i) value=(value^p.pixels()[i])*16777619u;
  return value;
}
int main() {
  auto a=std::unique_ptr<light::Painting>(new light::Painting(17));
  auto b=std::unique_ptr<light::Painting>(new light::Painting(17));
  a->render(0,0); b->render(0,0);
  assert(hash(*a)==hash(*b));
  auto initial=hash(*a);
  unsigned seen=0;
  for(unsigned i=0;i<3600;++i) {
    if(i%60==0) { auto old=a->visualFamily(); a->regenerate(); b->regenerate(); assert(old!=a->visualFamily()); }
    seen |= 1u << a->visualFamily();
    a->render(1.0f/12,0.4f); b->render(1.0f/12,0.4f);
    assert(hash(*a)==hash(*b));
  }
  assert(seen==63);
  assert(hash(*a)!=initial && a->generation()==61);
  a->render(0.1f,1); b->render(0.1f,0);
  assert(hash(*a)!=hash(*b));
  // Every family animates and responds; a shake is visibly different immediately.
  for(unsigned family=0;family<6;++family) {
    a->seed(17); b->seed(17);
    while(a->visualFamily()!=family) { a->regenerate(); b->regenerate(); }
    a->render(0,0); auto start=hash(*a);
    for(unsigned i=0;i<90;++i) { a->render(.083f,1); b->render(.083f,0); }
    assert(hash(*a)!=start && hash(*a)!=hash(*b));
    auto before=hash(*a); a->regenerate(); a->render(0,0);
    assert(hash(*a)!=before);
  }
  std::cout<<"Visual transitions, deterministic seeds, audio response and frame bounds passed\n";
}
