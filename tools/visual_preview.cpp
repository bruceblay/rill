// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/Light.h"
#include <fstream>
#include <memory>
#include <cstdlib>
int main(int argc,char** argv) {
  if(argc<2) return 1;
  auto p=std::unique_ptr<light::Painting>(new light::Painting(argc>2 ? std::strtoul(argv[2],nullptr,10) : 17));
  if(argc>3) while(p->visualFamily()!=unsigned(std::strtoul(argv[3],nullptr,10))%6) p->regenerate();
  for(unsigned i=0;i<120;++i) p->render(1.0f/12,0.35f);
  std::ofstream out(argv[1],std::ios::binary);
  out<<"P6\n240 135\n255\n";
  for(unsigned i=0;i<240*135;++i) {
    uint16_t c=p->pixels()[i];
    out.put(char(((c>>11)&31)*255/31));
    out.put(char(((c>>5)&63)*255/63));
    out.put(char((c&31)*255/31));
  }
  return out ? 0 : 2;
}
