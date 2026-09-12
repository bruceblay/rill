#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace light {
// Fine, continuously folding ribbons. Independent from musical random state.
class Painting {
 public:
  static constexpr unsigned width=240, height=135;
 private:
  std::array<uint16_t,width*height> frame{};
  uint32_t rng;
  unsigned count=0, palette=0;
  float phase=0, breath=0;
  std::array<float,6> shape{}, target{};
  float colors[2][3]={{90,170,180},{210,130,100}};
  unsigned random() { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
  float unit() { return float(random()>>8)/16777216.0f; }
  void dot(int x,int y,float amount,const float* c) {
    if(x<0 || x>=int(width) || y<0 || y>=int(height)) return;
    auto& pixel=frame[y*width+x];
    unsigned r=std::min(255,int(((pixel>>11)&31)*255/31+c[0]*amount));
    unsigned g=std::min(255,int(((pixel>>5)&63)*255/63+c[1]*amount));
    unsigned b=std::min(255,int((pixel&31)*255/31+c[2]*amount));
    pixel=uint16_t(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
  }
 public:
  explicit Painting(uint32_t value=17):rng(value?value:1) { regenerate(); shape=target; }
  void seed(uint32_t value) { rng=value?value:1; count=0; regenerate(); shape=target; }
  void regenerate() {
    palette=count?(palette+1+random()%3)%4:random()%4;
    ++count;
    for(auto& v:target) v=unit();
  }
  unsigned generation() const { return count; }
  const uint16_t* pixels() const { return frame.data(); }
  void render(float seconds,float audio) {
    seconds=std::max(0.0f,std::min(0.25f,seconds));
    phase+=seconds*0.13f; if(phase>628.3185f) phase-=628.3185f;
    breath+=(std::max(0.0f,std::min(1.0f,audio))-breath)*std::min(1.0f,seconds*2);
    for(unsigned i=0;i<6;++i) shape[i]+=(target[i]-shape[i])*seconds*0.5f;
    static const float palettes[4][2][3]={
      {{70,160,160},{210,140,100}},{{110,135,210},{190,135,165}},
      {{170,160,95},{85,155,145}},{{130,170,190},{185,115,95}}};
    for(unsigned k=0;k<2;++k) for(unsigned j=0;j<3;++j)
      colors[k][j]+=(palettes[palette][k][j]-colors[k][j])*seconds*0.6f;
    frame.fill(0x0842);
    // Two families of strands share a field but drift at different rates.
    // Trigonometry per horizontal sample; strand offsets reuse those values.
    for(unsigned k=0;k<2;++k) {
      for(unsigned x=0;x<width;++x) {
        float u=float(x)/(width-1), t=u*6.283185f;
        float bend=std::sin(t*(0.65f+shape[k]*0.65f)+phase*(k?-.7f:1)+shape[4]*6);
        float fold=std::sin(t*(1.1f+shape[2+k])+phase*.43f+k*2);
        float edge=std::sin(u*3.141593f);
        float center=67+(k?1:-1)*12+edge*(bend*23+fold*10);
        float spread=4+edge*(12+10*std::cos(t*.7f-phase*.5f+k));
        for(unsigned strand=0;strand<18;++strand) {
          float v=float(strand)/17, offset=(v-.5f)*2;
          float y=center+offset*spread+offset*offset*edge*12*(shape[5]-.5f);
          int iy=int(std::floor(y)); float fraction=y-iy;
          float intensity=(.28f+.18f*breath)*(.45f+.55f*edge)*(1-.3f*std::abs(offset));
          dot(x,iy,intensity*(1-fraction),colors[k]);
          dot(x,iy+1,intensity*fraction,colors[k]);
        }
      }
    }
  }
};
}
