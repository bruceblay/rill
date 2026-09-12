// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
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
  uint32_t evolutionRng=1;
  float evolutionAt=0;
  std::array<float,6> evolving{}, goals{};
  std::array<Point,28> dots{}, speeds{};
  float evolveUnit() { evolutionRng^=evolutionRng<<13; evolutionRng^=evolutionRng>>17; evolutionRng^=evolutionRng<<5; return float(evolutionRng>>8)/16777216.0f; }
  void evolve(float dt) {
    evolutionAt-=dt;
    if(evolutionAt<=0) {
      evolutionAt=5+evolveUnit()*7;
      for(auto& g:goals) g=evolveUnit();
    }
    for(unsigned i=0;i<6;++i) evolving[i]+=(goals[i]-evolving[i])*dt*.45f;
  }
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
    float low=135,high=0;
    for(const auto& p:points) { low=std::min(low,p.y); high=std::max(high,p.y); }
    for(int y=std::max(0,int(std::floor(low)));y<=std::min(134,int(std::ceil(high)));++y) {
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
      float cx=20+v*200;
      float cy=67+std::sin(angle)*(8+parameters[2]*10);
      float rx=20+parameters[3]*12+std::sin(v*3.141593f)*(14+parameters[4]*10);
      float ry=4+7*(.5f+.5f*std::cos(angle*.7f))+breath*2;
      Color c={ink[0].r*(1-v)+ink[1].r*v,ink[0].g*(1-v)+ink[1].g*v,ink[0].b*(1-v)+ink[1].b*v};
      ellipse(cx,cy,ry*1.3f,rx*.78f,c);
      ellipse(cx-1,cy+2,std::max(1.0f,ry*.62f),rx*.51f,{12,14,24});
    }
  }
  void circles(float dt) {
    unsigned count=12+unsigned(parameters[0]*16);
    // Persistent particles: two moving attractions and local repulsion make the
    // next arrangement depend on the previous one, rather than replay a loop.
    for(unsigned i=0;i<count;++i) {
      unsigned group=i%2;
      float cx=35+evolving[group*2]*170,cy=25+evolving[group*2+1]*85;
      float a=phase*(.2f+parameters[2]*.4f)+i*2.399963f;
      float orbit=12+evolving[4]*30;
      float tx=cx+std::cos(a)*orbit,ty=cy+std::sin(a)*orbit*.7f;
      float fx=(tx-dots[i].x)*1.1f,fy=(ty-dots[i].y)*1.1f;
      for(unsigned j=0;j<count;++j) if(i!=j) {
        float dx=dots[i].x-dots[j].x,dy=dots[i].y-dots[j].y;
        float d2=dx*dx+dy*dy;
        if(d2<400) { float f=220/(d2+12); fx+=dx*f; fy+=dy*f; }
      }
      speeds[i].x=(speeds[i].x+fx*dt)*std::exp(-2.2f*dt);
      speeds[i].y=(speeds[i].y+fy*dt)*std::exp(-2.2f*dt);
    }
    for(unsigned i=0;i<count;++i) {
      dots[i].x=std::max(9.0f,std::min(231.0f,dots[i].x+speeds[i].x*dt));
      dots[i].y=std::max(9.0f,std::min(126.0f,dots[i].y+speeds[i].y*dt));
      float r=3+(i%4)*1.3f+breath*2;
      ellipse(dots[i].x,dots[i].y,r,r,ink[i%3]);
    }
  }
  void tiles() {
    unsigned columns=4+unsigned(parameters[0]*4),rows=3+unsigned(parameters[1]*2);
    float turn=phase*.35f;
    for(unsigned row=0;row<rows;++row) for(unsigned col=0;col<columns;++col) {
      float gx=(col+.5f)*240/columns,gy=(row+.5f)*135/rows;
      float u=(float(col)/(columns-1)-.5f)*2,v=(float(row)/(rows-1)-.5f)*2;
      float a=std::atan2(v,u)+turn;
      float orbit=20+std::sqrt(u*u+v*v)*35;
      float blend=evolving[0];
      float x=gx+(120+std::cos(a)*orbit*1.65f-gx)*blend;
      float y=gy+(67+std::sin(a)*orbit*.8f-gy)*blend;
      float wave=std::sin(u*(2+evolving[1]*4)+v*3-phase);
      float angle=turn+wave*(.2f+evolving[2]*1.8f);
      float c=std::cos(angle),s=std::sin(angle);
      float r=7+evolving[3]*6+wave*2+breath;
      float fold=.18f+.82f*std::abs(std::cos(phase*.45f+col*.6f+row*.8f));
      std::array<Point,24> points;
      for(unsigned i=0;i<24;++i) {
        unsigned side=i/6; float t=float(i%6)/6;
        static const float vx[4]={-1,1,1,-1},vy[4]={-1,-1,1,1};
        float px=(vx[side]+(vx[(side+1)%4]-vx[side])*t)*fold;
        float py=vy[side]+(vy[(side+1)%4]-vy[side])*t;
        points[i]={x+r*(px*c-py*s),y+r*(px*s+py*c)};
      }
      Color face=ink[(row+col)%3];
      float shade=.55f+.45f*fold; face.r*=shade;face.g*=shade;face.b*=shade;
      polygon(points,face);
    }
  }
  void line(float x,float y,float ex,float ey,Color c) {
    int steps=int(std::max(std::abs(ex-x),std::abs(ey-y)))+1;
    for(int i=0;i<=steps;++i) {
      float t=float(i)/steps;
      int px=int(x+(ex-x)*t),py=int(y+(ey-y)*t);
      if(px>=0 && px<240 && py>=0 && py<135) frame[py*240+px]=color(c);
    }
  }
  void reflections() {
    // Four boundaries plus a moving interior mirror create new ray paths.
    struct Mirror { Point a,b; };
    float mx=65+evolving[2]*110,my=35+evolving[3]*65;
    float angle=evolving[4]*6.283185f,length=22+parameters[3]*20;
    Mirror mirrors[5]={{{5,5},{235,5}},{{235,5},{235,130}},{{235,130},{5,130}},{{5,130},{5,5}},
      {{mx-std::cos(angle)*length,my-std::sin(angle)*length},{mx+std::cos(angle)*length,my+std::sin(angle)*length}}};
    unsigned rays=9+unsigned(parameters[0]*18),bounces=2+unsigned(parameters[2]*3);
    for(unsigned i=0;i<rays;++i) {
      float x=15+evolving[0]*210,y=15+evolving[1]*105;
      float a=phase*.13f+evolving[5]*6+(float(i)/(rays-1)-.5f)*(.25f+parameters[1]*2.8f)+breath*.1f;
      float dx=std::cos(a),dy=std::sin(a);
      for(unsigned bounce=0;bounce<bounces;++bounce) {
        float nearest=10000; int hit=-1;
        for(unsigned m=0;m<5;++m) {
          float sx=mirrors[m].b.x-mirrors[m].a.x,sy=mirrors[m].b.y-mirrors[m].a.y;
          float qx=mirrors[m].a.x-x,qy=mirrors[m].a.y-y,den=dx*sy-dy*sx;
          if(std::abs(den)<.0001f) continue;
          float t=(qx*sy-qy*sx)/den,u=(qx*dy-qy*dx)/den;
          if(t>.02f && t<nearest && u>=0 && u<=1) { nearest=t;hit=int(m); }
        }
        if(hit<0) break;
        float ex=x+dx*nearest,ey=y+dy*nearest;
        Color c=ink[(i/3)%3]; float shade=1-bounce*.18f;
        c.r*=shade;c.g*=shade;c.b*=shade;line(x,y,ex,ey,c);
        float nx=mirrors[hit].b.y-mirrors[hit].a.y,ny=mirrors[hit].a.x-mirrors[hit].b.x;
        float dot=2*(dx*nx+dy*ny)/(nx*nx+ny*ny);
        dx-=dot*nx;dy-=dot*ny;x=ex;y=ey;
      }
    }
  }

 public:
  explicit Painting(uint32_t value=17):rng(value?value:1) { regenerate(); }
  void seed(uint32_t value) { rng=value?value:1; count=0; regenerate(); }
  void regenerate() {
    kind=count?(kind+1+random()%5)%6:random()%6;
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
    evolutionRng=(rng^0x85ebca6bu)|1u;
    evolutionAt=0;
    for(unsigned i=0;i<6;++i) evolving[i]=goals[i]=evolveUnit();
    for(unsigned i=0;i<28;++i) { dots[i]={15+evolveUnit()*210,15+evolveUnit()*105}; speeds[i]={0,0}; }
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
    if(kind>=3) evolve(seconds);
    if(kind==0) creatures(seconds);
    else if(kind==1) cutouts();
    else if(kind==2) sculpture();
    else if(kind==3) circles(seconds);
    else if(kind==4) tiles();
    else reflections();
  }
};
}
