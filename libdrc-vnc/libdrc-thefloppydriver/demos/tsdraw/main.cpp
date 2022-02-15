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

#include "../framework/framework.h"

#include <cmath>
#include <cstdlib>
#include <drc/input.h>
#include <drc/screen.h>
#include <drc/streamer.h>
#include <SDL.h>
#include <vector>

namespace {

const drc::u32 kColors[] = {
  0xFFFFFFFF,  // white
  0x000000FF,  // black
  0xFF0000FF,  // red
  0x00FF00FF,  // green
  0x0000FFFF,  // blue
  0xFFFF00FF,  // yellow
  0xFF00FFFF,  // purple
  0x00FFFFFF,  // aqua
};
const int kNumColors =
    static_cast<int>(sizeof (kColors) / sizeof (kColors[0]));

void PutPixel(drc::u8* pixels, int x, int y, drc::u32 col) {
  drc::u8* ppix = pixels + 4 * (y * demo::GetWidth() + x);
  ppix[0] = col >> 8;
  ppix[1] = col >> 16;
  ppix[2] = col >> 24;
  ppix[3] = col;
}

void DrawLine(drc::u8* pixels, int x0, int y0, int x1, int y1, drc::u32 col) {
  int dx = abs(x1 - x0), dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    PutPixel(pixels, x0, y0, col);
    if (x0 == x1 && y0 == y1) {
      break;
    }

    int err2 = 2 * err;
    if (err2 > -dy) {
      err -= dy;
      x0 += sx;
    }

    if (x0 == x1 && y0 == y1) {
      PutPixel(pixels, x0, y0, col);
      break;
    }

    if (err2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void ShowPalette(drc::u8* pixels) {
  int base_x = 0;
  for (auto col : kColors) {
    for (int x = base_x; x < base_x + 16; ++x) {
      for (int y = 0; y < 16; ++y) {
        PutPixel(pixels, x, y, col);
      }
    }
    base_x += 16;
  }
}

void ShowSelectedColor(drc::u8* pixels, int idx) {
  static int frame_num = 0;

  int base_x = idx * 16;
  drc::u32 inverted_col = ~kColors[idx] | 0xFF;

  if (frame_num++ % 10 == 0) {
    for (int y = 0; y < 16; ++y) {
      if (y == 0 || y == 15) {
        for (int x = base_x; x < base_x + 16; ++x) {
          PutPixel(pixels, x, y, inverted_col);
        }
      } else {
        PutPixel(pixels, base_x, y, inverted_col);
        PutPixel(pixels, base_x + 15, y, inverted_col);
      }
    }
  }
}

void RenderFrame(SDL_Surface* surface, const drc::InputData& input_data) {
  static bool has_previous_point = false;
  static int previous_point_x, previous_point_y;
  static int selected_col = 0;
  static bool dir_btn_pressed = false;

  drc::u8* pixels = static_cast<drc::u8*>(surface->pixels);

  if (!input_data.valid) {
    return;
  }

  if (input_data.buttons & drc::InputData::kBtnLeft && selected_col > 0) {
    if (!dir_btn_pressed) {
      selected_col--;
    }
    dir_btn_pressed = true;
  } else if (input_data.buttons & drc::InputData::kBtnRight &&
             selected_col < kNumColors) {
    if (!dir_btn_pressed) {
      selected_col++;
    }
    dir_btn_pressed = true;
  } else {
    dir_btn_pressed = false;
  }

  if (input_data.buttons & drc::InputData::kBtnA) {
    memset(pixels, 0, 4 * demo::GetWidth() * demo::GetHeight());
  } else if (input_data.ts_pressed) {
    int x = static_cast<int>(input_data.ts_x * (demo::GetWidth() - 1));
    int y = static_cast<int>(input_data.ts_y * (demo::GetHeight() - 1));

    if (has_previous_point) {
      DrawLine(pixels, x, y, previous_point_x, previous_point_y,
               kColors[selected_col]);
    } else {
      PutPixel(pixels, x, y, kColors[selected_col]);
    }

    has_previous_point = true;
    previous_point_x = x;
    previous_point_y = y;

  } else {
    has_previous_point = false;
  }

  // Color selection stuff
  ShowPalette(pixels);
  ShowSelectedColor(pixels, selected_col);
}

}  // namespace

int main(int argc, char** argv) {
  int width = drc::kScreenWidth;
  int height = drc::kScreenHeight;

  // if the width/height is outside range, it's ok because
  // framework checks and sets to native w/h if invalid
  if (argc == 3) {
    width = atoi(argv[1]);
    height = atoi(argv[2]);
  }

  demo::Init("tsdraw", demo::kStreamerSDLDemo, width, height);

  demo::GetStreamer()->SetTSArea(demo::GetWidth(), demo::GetHeight());

  drc::InputData input_data;
  SDL_Surface* surface = SDL_GetVideoSurface();
  while (demo::HandleEvents()) {
    demo::GetStreamer()->PollInput(&input_data);

    SDL_LockSurface(surface);
    std::vector<drc::u8> pixels(
        (drc::u8*)surface->pixels,
        (drc::u8*)surface->pixels + demo::GetWidth() * demo::GetHeight() * 4);
    demo::GetStreamer()->PushVidFrame(&pixels, demo::GetWidth(),
                                      demo::GetHeight(),
                                      drc::PixelFormat::kBGRA);
    RenderFrame(surface, input_data);
    SDL_UnlockSurface(surface);

    demo::SwapBuffers();
  }

  demo::Quit();
  return 0;
}
