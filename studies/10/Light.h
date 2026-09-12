#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

// Independent from the score: changing a painting cannot consume musical RNG.
namespace light {
class Painting {
 public:
  static constexpr unsigned width = 240, height = 135;
 private:
  struct Layer { float x, y, rx, ry, angle, r, g, b, contour; };
  std::array<Layer, 3> current{}, destination{};
  std::array<uint16_t, width * height> frame{};
  uint32_t rng;
  unsigned palette = 0, count = 0;
  float phase = 0, breath = 0;
  uint32_t random() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
  float unit() { return float(random() >> 8) / 16777216.0f; }
  static float follow(float a, float b, float t) { return a + (b - a) * t; }
 public:
  explicit Painting(uint32_t seed = 17) : rng(seed ? seed : 1) { regenerate(); current = destination; }
  void seed(uint32_t value) { rng=value ? value : 1; count=0; regenerate(); current=destination; }
  void regenerate() {
    static const uint8_t colors[5][3][3] = {
      {{224,111,74},{47,155,151},{171,128,195}},
      {{116,144,207},{218,130,160},{117,184,165}},
      {{217,172,85},{145,99,163},{71,152,135}},
      {{93,160,179},{193,165,117},{189,104,106}},
      {{172,128,182},{95,155,129},{211,165,123}}
    };
    palette = count ? (palette + 1 + random() % 4) % 5 : random() % 5;
    ++count;
    for (unsigned i = 0; i < 3; ++i) {
      destination[i] = {-0.8f + unit() * 1.6f, -0.45f + unit() * 0.9f,
        0.40f + unit() * 0.5f, 0.30f + unit() * 0.45f,
        unit() * 6.283185f, float(colors[palette][i][0]), float(colors[palette][i][1]),
        float(colors[palette][i][2]), unit()};
    }
  }
  unsigned generation() const { return count; }
  const uint16_t* pixels() const { return frame.data(); }
  void render(float seconds, float audioLevel) {
    seconds = std::max(0.0f, std::min(0.25f, seconds));
    phase += seconds * 0.10f;
    if (phase > 628.3185f) phase -= 628.3185f;
    breath = follow(breath, std::max(0.0f, std::min(1.0f, audioLevel)), std::min(1.0f, seconds * 2.0f));
    struct Prepared { float x,y,rx,ry,c,s,r,g,b,contour; } p[3];
    float blend = std::min(1.0f, seconds * 0.85f);
    for (unsigned i = 0; i < 3; ++i) {
      auto& a = current[i]; const auto& b = destination[i];
      a.x=follow(a.x,b.x,blend); a.y=follow(a.y,b.y,blend);
      a.rx=follow(a.rx,b.rx,blend); a.ry=follow(a.ry,b.ry,blend);
      a.angle=follow(a.angle,b.angle,blend);
      a.r=follow(a.r,b.r,blend); a.g=follow(a.g,b.g,blend); a.b=follow(a.b,b.b,blend);
      a.contour=follow(a.contour,b.contour,blend);
      float angle = a.angle + std::sin(phase * 0.6f + i) * 0.30f;
      float expansion = 1.0f + breath * 0.045f;
      p[i] = {a.x + std::sin(phase + i * 2) * 0.16f,
        a.y + std::cos(phase * 0.7f + i * 2) * 0.12f,
        1.0f/(a.rx*a.rx*expansion), 1.0f/(a.ry*a.ry*expansion),
        std::cos(angle),std::sin(angle),a.r,a.g,a.b,a.contour};
    }
    // Half-resolution light fields, doubled with a delicate scan-line texture.
    // Trigonometry is evaluated per layer, never per pixel.
    for (unsigned y = 0; y < height; y += 2) {
      float py = (float(y) - 67.0f) / 67.0f;
      for (unsigned x = 0; x < width; x += 2) {
        float px = (float(x) - 119.0f) / 67.0f;
        float r=8, g=10, b=19;
        for (auto& l : p) {
          float dx=px-l.x, dy=py-l.y;
          float u=dx*l.c+dy*l.s, v=dy*l.c-dx*l.s;
          float q=u*u*l.rx+v*v*l.ry;
          float glow=1.0f/(1.0f+5.0f*q*q);
          float edge=q-0.82f;
          float rim=1.0f/(1.0f+100.0f*edge*edge);
          float weight=(glow*(0.30f+0.18f*(1-l.contour))+rim*l.contour*0.30f)*(1.50f+breath*0.16f);
          r+=l.r*weight; g+=l.g*weight; b+=l.b*weight;
        }
        float vignette=std::max(0.55f,1.0f-0.085f*(px*px+py*py));
        for (unsigned dy=0;dy<2 && y+dy<height;++dy) {
          float shade=vignette*(dy ? 0.955f : 1.0f);
          unsigned rr=unsigned(std::min(255.0f,r*shade));
          unsigned gg=unsigned(std::min(255.0f,g*shade));
          unsigned bb=unsigned(std::min(255.0f,b*shade));
          uint16_t color=uint16_t(((rr>>3)<<11)|((gg>>2)<<5)|(bb>>3));
          frame[(y+dy)*width+x]=frame[(y+dy)*width+x+1]=color;
        }
      }
    }
  }
};
}
