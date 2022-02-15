// Copyright (c) 2013, Mema Hacking, All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <array>
#include <cassert>
#include <drc/internal/video-converter.h>
#include <drc/pixel-format.h>
#include <drc/screen.h>
#include <future>
#include <utility>
#include <vector>

extern "C" {
#include <libswscale/swscale.h>
}  // extern "C"

#undef PixelFormat  // fuck ffmpeg

namespace drc {

namespace {

AVPixelFormat ConvertPixFmt(PixelFormat pixfmt) {
  switch (pixfmt) {
    case PixelFormat::kRGB: return AV_PIX_FMT_RGB24;
    case PixelFormat::kRGBA: return AV_PIX_FMT_RGBA;
    case PixelFormat::kBGR: return AV_PIX_FMT_BGR24;
    case PixelFormat::kBGRA: return AV_PIX_FMT_BGRA;
    case PixelFormat::kRGB565: return AV_PIX_FMT_RGB565;
    default: assert(false && "Invalid PixelFormat value");
  }
}

std::array<const byte*, 4> GetPixFmtPlanes(PixelFormat pixfmt,
                                           const byte* data) {
  switch (pixfmt) {
    case PixelFormat::kRGB:
    case PixelFormat::kRGBA:
    case PixelFormat::kBGR:
    case PixelFormat::kBGRA:
    case PixelFormat::kRGB565:
      return {data, NULL, NULL, NULL};
    default:
      assert(false && "Invalid PixelFormat value");
  }
}
std::array<int, 4> GetPixFmtStrides(PixelFormat pixfmt, u16 width,
                                    u16 height) {
  switch (pixfmt) {
    case PixelFormat::kRGB565:
      return {width * 2, 0, 0, 0};
    case PixelFormat::kRGB:
    case PixelFormat::kBGR:
      return {width * 3, 0, 0, 0};
    case PixelFormat::kRGBA:
    case PixelFormat::kBGRA:
      return {width * 4, 0, 0, 0};
    default:
      assert(false && "Invalid PixelFormat value");
  }
}

std::array<int, 4> GetPixFmtHeights(PixelFormat pixfmt, u16 height) {
  switch (pixfmt) {
    case PixelFormat::kRGB565:
    case PixelFormat::kRGB:
    case PixelFormat::kBGR:
    case PixelFormat::kRGBA:
    case PixelFormat::kBGRA:
      return {height, 0, 0, 0};
    default:
      assert(false && "Invalid PixelFormat value");
  }
}

}  // namespace

VideoConverter::VideoConverter() {
}

VideoConverter::~VideoConverter() {
  Stop();

  // Free any allocated SwsContext.
  for (const auto& it : ctxs_) {
    sws_freeContext(it.second);
  }
}

bool VideoConverter::Start() {
  return true;
}

void VideoConverter::Stop() {
  if (!current_conv_.valid()) {
    return;
  }

  current_conv_.wait();
}

void VideoConverter::PushFrame(std::vector<byte>* frame,
                               const VideoConverterParams& params) {
  if (current_conv_.valid()) {
    current_conv_.wait();
  }

  // Capture the current frame before spawning the conversion thread: we do not
  // know when the thread will be scheduled, and the user might have destroyed
  // the vector containing the frame before that.
  current_frame_ = std::move(*frame);
  current_params_ = params;

  current_conv_ = std::async(std::launch::async,
      [=]() { DoConversion(); });
}

SwsContext* VideoConverter::GetContextForParams(
    const VideoConverterParams& params) {
  u16 width, height;
  PixelFormat pixfmt;
  bool flipv;
  bool stretch;
  bool keep_ar;

  std::tie(width, height, pixfmt, flipv, stretch, keep_ar) = params;

  int target_w = width;
  int target_h = height;

  if (stretch) {
    target_w = kScreenWidth;
    target_h = kScreenHeight;

    if (keep_ar) {
      if ((854 * height) > (480 * width)) {
        target_w = width * 480 / height;
      } else {
        target_h = height * 854 / width;
      }
    }
  }


  SwsContext* ctx = sws_getCachedContext(ctxs_[params],
      width, height, ConvertPixFmt(pixfmt),
      target_w, target_h,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51, 74, 100)
      AV_PIX_FMT_YUV420P,
#else
      PIX_FMT_YUV420P,
#endif
      SWS_FAST_BILINEAR, NULL, NULL, NULL);

  ctxs_[params] = ctx;
  return ctx;
}

void VideoConverter::DoConversion() {
  int y_size = kScreenWidth * kScreenHeight;
  int u_size = y_size / 4;
  int v_size = y_size / 4;

  // init vector to (0,128,128) (black in yuv)
  std::vector<byte> converted(y_size + u_size + v_size, 0);
  std::fill_n(converted.data()+y_size, u_size+v_size, 128);

  SwsContext* ctx = GetContextForParams(current_params_);

  u16 width, height;
  PixelFormat pixfmt;
  bool flipv;
  bool stretch;
  bool keep_ar;
  std::tie(width, height, pixfmt, flipv, stretch, keep_ar) = current_params_;

  int target_w = width;
  int target_h = height;

  if (stretch) {
    target_w = kScreenWidth;
    target_h = kScreenHeight;

    if (keep_ar) {
      if ((854 * height) > (480 * width)) {
        target_w = width * 480 / height;
      } else {
        target_h = height * 854 / width;
      }
    }
  }

  byte* converted_planes[4] = {
      converted.data(),
      converted.data() + y_size,
      converted.data() + y_size + u_size,
      NULL
  };
  int converted_strides[4] = {
      kScreenWidth,
      kScreenWidth / 2,
      kScreenWidth / 2,
      0
  };
  std::array<const byte*, 4> planes =
      GetPixFmtPlanes(pixfmt, current_frame_.data());
  std::array<int, 4> strides = GetPixFmtStrides(pixfmt, width, height);

  if (flipv) {
    std::array<int, 4> heights = GetPixFmtHeights(pixfmt, height);
    for (int i = 0; i < 4; ++i) {
      int plane_size = strides[i] * heights[i];
      planes[i] += plane_size - strides[i];
      strides[i] = -strides[i];
    }
  }

  if (!stretch || keep_ar) {
    int h_margin = (kScreenWidth - target_w)/2;
    int v_margin = (kScreenHeight - target_h)/2;

    converted_planes[0] += v_margin * converted_strides[0] + h_margin;
    converted_planes[1] += (v_margin / 2) * converted_strides[1]
                          + (h_margin / 2);
    converted_planes[2] += (v_margin / 2) * converted_strides[1]
                          + (h_margin / 2);
  }

  int res = sws_scale(ctx, planes.data(), strides.data(), 0, height,
                      converted_planes, converted_strides);

  done_cb_(&converted);
}

}  // namespace drc
