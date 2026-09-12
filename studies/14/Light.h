#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace light {
class Painting {
 public:
  static constexpr unsigned width=240,height=135;
 private:
  struct Point { float x,y; };
  struct Color { float r,g,b; };
  std::array<uint16_t,width*height> frame{};
  std::array<float,48> radius{},velocity{};
  uint32_t rng;
  unsigned count=0,kind=0,palette=0;
  float phase=0,breath=0;
  std::array<float,6> parameters{};
  Color ink[3]{};
  uint16_t background=0;
  unsigned random() { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
  float unit() { return float(random()>>8)/16777216.0f; }
  static uint16_t color(Color c,float shade=1) {
    unsigned r=unsigned(std::max(0.0f,std::min(255.0f,c.r*shade)));
    unsigned g=unsigned(std::max(0.0f,std::min(255.0f,c.g*shade)));
    unsigned b=unsigned(std::max(0.0f,std::min(255.0f,c.b*shade)));
    return uint16_t(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
  }
  void span(int y,int left,int right,uint16_t c) {
    if(y<0 || y>=int(height)) return;
    left=std::max(0,left); right=std::min(int(width)-1,right);
    for(int x=left;x<=right;++x) frame[y*width+x]=c;
  }
  void ellipse(float cx,float cy,float rx,float ry,Color c,float shade=1) {
    if(rx<1 || ry<1) return;
    int top=std::max(0,int(std::floor(cy-ry))),bottom=std::min(134,int(std::ceil(cy+ry)));
    for(int y=top;y<=bottom;++y) {
      float v=(y+.5f-cy)/ry;
      if(std::abs(v)>1) continue;
      float extent=rx*std::sqrt(1-v*v);
      span(y,int(std::ceil(cx-extent)),int(std::floor(cx+extent)),color(c,shade*(.75f+.25f*(1-v)*.5f)));
    }
  }
  void polygon(const std::array<Point,24>& points,Color c,bool window=false) {
    // Bounded scanline fill, including concave contours.
    float crossings[24];
    for(int y=0;y<135;++y) {
      unsigned n=0;
      for(unsigned i=0;i<24;++i) {
        const auto& a=points[i]; const auto& b=points[(i+1)%24];
        float scan=y+.5f;
        if((a.y<=scan && b.y>scan)||(b.y<=scan && a.y>scan))
          crossings[n++]=a.x+(scan-a.y)*(b.x-a.x)/(b.y-a.y);
      }
      std::sort(crossings,crossings+n);
      for(unsigned j=0;j+1<n;j+=2) {
        int left=std::max(0,int(std::ceil(crossings[j]))),right=std::min(239,int(std::floor(crossings[j+1])));
        if(!window) span(y,left,right,color(c,.82f+.18f*(1-float(y)/135)));
        else for(int x=left;x<=right;++x) {
          // A separate moving plane revealed through the opening.
          float stripe=x*(.5f+parameters[3])+y*(parameters[4]-.5f)+phase*19+breath*6;
          int band=int(std::floor(stripe/(7+parameters[5]*10)));
          unsigned index=unsigned((band%3+3)%3);
          frame[y*width+x]=color(ink[index]);
        }
      }
    }
  }
  void creatures(float dt) {
    for(unsigned body=0;body<2;++body) {
      std::array<Point,24> points;
      float orbit=phase*.7f+body*3.141593f;
      float cx=120+std::cos(orbit)*(43+parameters[0]*17),cy=67+std::sin(orbit)*22;
      float base=26+parameters[1+body]*12+breath*3;
      for(unsigned i=0;i<24;++i) {
        unsigned j=body*24+i;
        float angle=i*6.283185f/24;
        float target=base*(1+.16f*std::sin(angle*3+phase*1.8f+body)+.1f*std::cos(angle*2-phase));
        velocity[j]+=(target-radius[j])*28*dt;
        velocity[j]*=std::exp(-8*dt);
        radius[j]+=velocity[j]*dt;
        points[i]={cx+std::cos(angle)*radius[j]*(1+.12f*std::sin(phase)),cy+std::sin(angle)*radius[j]};
      }
      polygon(points,ink[body]);
      // An off-center opening gives each moving form an asymmetric silhouette.
      ellipse(cx+7,cy-4,7+parameters[4]*5,9,ink[2]);
    }
  }
  void cutouts() {
    // Bold surrounding arcs move independently of the inner stripe plane.
    for(int i=5;i>=0;--i)
      ellipse(120+std::sin(phase*.4f)*22,67,120-i*14,90-i*10,ink[i%2],.28f);
    std::array<Point,24> points;
    for(unsigned i=0;i<24;++i) {
      float a=i*6.283185f/24;
      float r=1+.18f*std::sin(a*(3+int(parameters[0]*3))+phase)+.08f*std::cos(a*2-phase);
      points[i]={120+std::cos(a+phase*.23f)*r*(64+parameters[1]*18),67+std::sin(a+phase*.23f)*r*43};
    }
    polygon(points,ink[0],true);
  }
  void sculpture() {
    unsigned layers=19+unsigned(parameters[0]*10);
    for(unsigned i=0;i<layers;++i) {
      float v=float(i)/(layers-1),angle=phase+v*(3+parameters[1]*7);
      float cx=120+std::sin(angle)*(15+parameters[2]*25);
      float cy=20+v*96;
      float rx=20+parameters[3]*12+std::sin(v*3.141593f)*(14+parameters[4]*10);
      float ry=4+7*(.5f+.5f*std::cos(angle*.7f))+breath*2;
      Color c={ink[0].r*(1-v)+ink[1].r*v,ink[0].g*(1-v)+ink[1].g*v,ink[0].b*(1-v)+ink[1].b*v};
      ellipse(cx,cy,rx,ry,c);
      ellipse(cx+2,cy-1,rx*.65f,std::max(1.0f,ry*.48f),{12,14,24});
    }
  }
 public:
  explicit Painting(uint32_t value=17):rng(value?value:1) { regenerate(); }
  void seed(uint32_t value) { rng=value?value:1; count=0; regenerate(); }
  void regenerate() {
    kind=count?(kind+1+random()%2)%3:random()%3;
    palette=count?(palette+1+random()%3)%4:random()%4;
    ++count; phase=unit()*6.283185f; breath=0;
    for(auto& p:parameters) p=unit();
    static const Color palettes[4][3]={
      {{245,111,91},{76,205,192},{251,217,145}},
      {{154,142,236},{247,187,83},{243,220,202}},
      {{229,116,171},{124,203,232},{248,224,161}},
      {{177,219,142},{252,164,98},{128,166,231}}};
    for(unsigned i=0;i<3;++i) ink[i]=palettes[palette][i];
    background=color({12,14,24});
    radius.fill(30); velocity.fill(0);
    frame.fill(background);
  }
  unsigned generation() const { return count; }
  unsigned visualFamily() const { return kind; }
  const uint16_t* pixels() const { return frame.data(); }
  void render(float seconds,float audio) {
    seconds=std::max(0.0f,std::min(.1f,seconds));
    phase+=seconds*(.65f+parameters[5]*.45f); if(phase>628.3185f) phase-=628.3185f;
    breath+=(std::max(0.0f,std::min(1.0f,audio))-breath)*std::min(1.0f,seconds*5);
    frame.fill(background);
    if(kind==0) creatures(seconds);
    else if(kind==1) cutouts();
    else sculpture();
  }
};
}
