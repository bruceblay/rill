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
      float cx=120+std::sin(angle)*(15+parameters[2]*25);
      float cy=20+v*96;
      float rx=20+parameters[3]*12+std::sin(v*3.141593f)*(14+parameters[4]*10);
      float ry=4+7*(.5f+.5f*std::cos(angle*.7f))+breath*2;
      Color c={ink[0].r*(1-v)+ink[1].r*v,ink[0].g*(1-v)+ink[1].g*v,ink[0].b*(1-v)+ink[1].b*v};
      ellipse(cx,cy,rx,ry,c);
      ellipse(cx+2,cy-1,rx*.65f,std::max(1.0f,ry*.48f),{12,14,24});
    }
  }
  void circles() {
    float blend=.5f-.5f*std::cos(phase*.65f);
    for(unsigned i=0;i<20;++i) {
      float a=i*6.283185f/20+phase*.35f;
      float gx=40+(i%5)*40,gy=27+(i/5)*27;
      float ox=120+std::cos(a)*(42+parameters[0]*35),oy=67+std::sin(a)*43;
      float x=gx+(ox-gx)*blend,y=gy+(oy-gy)*blend;
      float r=5+3*std::sin(a*2+phase)+breath*2;
      ellipse(x,y,r,r,ink[i%3]);
    }
  }
  void tiles() {
    for(unsigned row=0;row<4;++row) for(unsigned col=0;col<7;++col) {
      float x=18+col*34,y=17+row*34;
      float a=phase*.7f+col*(.3f+parameters[0])+row*.55f;
      float r=10+3*std::sin(a+phase*.4f)+breath;
      std::array<Point,24> points;
      // Four straight edges, sampled for the common contour rasterizer.
      for(unsigned i=0;i<24;++i) {
        unsigned side=i/6; float t=float(i%6)/6;
        static const float vx[4]={-1,1,1,-1},vy[4]={-1,-1,1,1};
        float u=vx[side]+(vx[(side+1)%4]-vx[side])*t;
        float v=vy[side]+(vy[(side+1)%4]-vy[side])*t;
        points[i]={x+r*(u*std::cos(a)-v*std::sin(a)),y+r*(u*std::sin(a)+v*std::cos(a))};
      }
      polygon(points,ink[(row+col)%3]);
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
    float origin=120+std::sin(phase*.6f)*60;
    for(unsigned i=0;i<26;++i) {
      float x=origin,y=67;
      float a=phase*.25f+(float(i)/25-.5f)*(1.0f+parameters[1]*2)+breath*.08f;
      float dx=std::cos(a),dy=std::sin(a);
      for(unsigned bounce=0;bounce<4;++bounce) {
        float tx=std::abs(dx)<.0001f?100000:(dx>0?235-x:5-x)/dx;
        float ty=std::abs(dy)<.0001f?100000:(dy>0?130-y:5-y)/dy;
        float distance=std::min(tx,ty);
        float ex=x+dx*distance,ey=y+dy*distance;
        Color c=ink[i%3]; float shade=1-bounce*.18f;
        c.r*=shade; c.g*=shade; c.b*=shade;
        line(x,y,ex,ey,c);
        x=ex;y=ey;
        if(tx<=ty) dx=-dx; else dy=-dy;
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
    else if(kind==2) sculpture();
    else if(kind==3) circles();
    else if(kind==4) tiles();
    else reflections();
  }
};
}
