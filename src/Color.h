#pragma once

#include "common.h"

template<typename T>
struct ColorT {
  using Channel = T;

  Channel r, g, b, a;

  template<typename T>
  static constexpr Channel to_channel(T value) { return static_cast<Channel>(value); }

  constexpr Channel& channel(int index) { return (&r)[index]; }
  constexpr const Channel& channel(int index) const { return (&r)[index]; }

  friend bool operator==(const ColorT& lhs, const ColorT& rhs) {
    return std::tie(lhs.r, lhs.g, lhs.b, lhs.a) == 
           std::tie(rhs.r, rhs.g, rhs.b, rhs.a);
  }
  friend bool operator!=(const ColorT& lhs, const ColorT& rhs) {
    return !(lhs == rhs);
  }
  friend bool operator<(const ColorT& lhs, const ColorT& rhs) {
    return std::tie(lhs.r, lhs.g, lhs.b, lhs.a) < 
           std::tie(rhs.r, rhs.g, rhs.b, rhs.a);
  }
};

using RGBA = ColorT<uint8_t>;
using RGBAF = ColorT<float>;

constexpr RGBA uint32_to_rgba(uint32_t value) {
  return RGBA{
    RGBA::to_channel((value >> 0)  & 0xFF),
    RGBA::to_channel((value >> 8)  & 0xFF),
    RGBA::to_channel((value >> 16) & 0xFF),
    RGBA::to_channel((value >> 24) & 0xFF)
  };
}

constexpr RGBA::Channel to_gray(const RGBA& rgba) {
  return RGBA::to_channel((rgba.r * 77 + rgba.g * 151 + rgba.b * 28) / 256);
}

inline float uint8_to_unorm(uint8_t v) {
  return static_cast<float>(v) / 255.0f;
}

inline uint8_t unorm_to_uint8(float v) {
  return static_cast<uint8_t>(255.0 * std::clamp(v, 0.0f, 1.0f) + 0.5f);
}

inline float srgb_to_linear(uint8_t c) {
  const auto x = uint8_to_unorm(c);
  return (x < 0.04045f ? 
    x / 12.92f :
    std::pow((x + 0.055f) / 1.055f, 2.4f));
}

inline uint8_t linear_to_srgb(float x) {
  const auto c = (x <= 0.0031308f ?
    12.92f * x :
    1.055f * std::pow(x, 1 / 2.4f) - 0.055f);
  return unorm_to_uint8(c);
}

inline RGBAF srgb_to_linear(RGBA c) {
  return {
    srgb_to_linear(c.r),
    srgb_to_linear(c.g),
    srgb_to_linear(c.b),
    uint8_to_unorm(c.a)
  };
}

inline RGBA linear_to_srgb(const RGBAF& c) {
  return {
    linear_to_srgb(c.r),
    linear_to_srgb(c.g),
    linear_to_srgb(c.b),
    unorm_to_uint8(c.a)
  };
}
