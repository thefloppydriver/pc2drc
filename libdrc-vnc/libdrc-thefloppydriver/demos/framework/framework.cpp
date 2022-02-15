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

#include "./framework.h"

#include <drc/screen.h>
#include <drc/streamer.h>
#include <GL/glew.h>
#include <stdio.h>
#include <string>
#include <SDL.h>
#include <time.h>
#include <vector>

namespace demo {

namespace {
drc::Streamer* g_streamer;
drc::u16 g_width;
drc::u16 g_height;
}  // namespace

void Init(const char* name, DemoMode mode, int width, int height) {
  std::string win_title = "libdrc demo - ";
  win_title += name;

  if (mode == kStreamerGLDemo || mode == kStreamerSDLDemo) {
    g_streamer = new drc::Streamer();
    if (!g_streamer->Start()) {
      puts("Unable to start streamer");
      exit(1);
    }
  } else {
    puts("Receiver mode demo not yet implemented");
    exit(1);
  }

  SDL_Init(SDL_INIT_VIDEO);
  int flags = SDL_HWSURFACE | SDL_DOUBLEBUF;
  if (mode == kStreamerGLDemo) {
    flags |= SDL_OPENGL;
  }

  if ((width > drc::kScreenWidth) || (width <= 0)) {
    g_width = drc::kScreenWidth;
  } else {
    g_width = width;
  }

  if ((height > drc::kScreenWidth) || (height <= 0)) {
    g_height = drc::kScreenHeight;
  } else {
    g_height = height;
  }

  SDL_SetVideoMode(g_width, g_height, 32, flags);
  SDL_WM_SetCaption(win_title.c_str(), NULL);

  if (mode == kStreamerGLDemo) {
    glewInit();

    if (!GLEW_ARB_pixel_buffer_object) {
      puts("Missing OpenGL extension: ARB_pixel_buffer_object");
      exit(1);
    }
  }
}

void Quit() {
  SDL_Quit();

  if (g_streamer) {
    g_streamer->Stop();
  }

  delete g_streamer;
}

drc::Streamer* GetStreamer() {
  return g_streamer;
}

drc::u16 GetWidth() {
  return g_width;
}

drc::u16 GetHeight() {
  return g_height;
}

bool HandleEvents() {
  SDL_Event evt;

  while (SDL_PollEvent(&evt)) {
    if (evt.type == SDL_QUIT) {
      return false;
    }
  }

  return true;
}

std::vector<drc::u8> TryReadbackFromGL() {
  std::vector<drc::u8> ret;

  // TODO(delroth): PBO

  // Very stupid implementation - ideally the GPU should flip the image
  // vertically and encode to YUV420.
  ret.resize(g_width * g_height * 4);
  glReadPixels(0, 0, g_width, g_height, GL_BGRA,
               GL_UNSIGNED_BYTE, ret.data());

  return ret;
}

void TryPushingGLFrame() {
  if (!g_streamer) {
    puts("Streamer not initialized, can't push a frame");
    exit(1);
  }

  std::vector<drc::u8> frame = demo::TryReadbackFromGL();
  if (frame.size()) {
    g_streamer->PushVidFrame(&frame, g_width, g_height,
                             drc::PixelFormat::kBGRA,
                             drc::Streamer::FlipVertically);
  }
}

void SwapBuffers(int fps_limit) {
  SDL_Surface* screen = SDL_GetVideoSurface();

  SDL_Flip(screen);
  if (screen->flags & SDL_OPENGL) {
    SDL_GL_SwapBuffers();
  }

  if (fps_limit == 0)
    return;

  static timespec prev_ts;
  timespec ts, delta;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  // Compute the time delta since last frame end.
  delta.tv_sec = ts.tv_sec - prev_ts.tv_sec - (ts.tv_nsec < prev_ts.tv_nsec);
  if (ts.tv_nsec < prev_ts.tv_nsec) {
    delta.tv_nsec = 1000000000L + ts.tv_nsec - prev_ts.tv_nsec;
  } else {
    delta.tv_nsec = ts.tv_nsec - prev_ts.tv_nsec;
  }

  // If more than 1s was spent computing the last frame, don't try to slow down
  // rendering, we're already too slow.
  long nsec_planned = 1000000000L / fps_limit;
  if (delta.tv_sec == 0 && nsec_planned > delta.tv_nsec) {
    delta.tv_nsec = nsec_planned - delta.tv_nsec;
    nanosleep(&delta, NULL);

    // Readjust current timestamp to start after the nanosleep.
    ts.tv_nsec += delta.tv_nsec;
    if (ts.tv_nsec >= 1000000000L) {
      ts.tv_nsec -= 1000000000L;
      ts.tv_sec += 1;
    }
  }

  prev_ts = ts;
}

}  // namespace demo
