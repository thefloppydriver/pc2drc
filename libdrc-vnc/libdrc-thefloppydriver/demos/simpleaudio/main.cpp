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
#include <drc/input.h>
#include <drc/streamer.h>
#include <vector>

int main() {
  std::vector<drc::s16> samples(48000 * 2);
  for (int i = 0; i < 48000; ++i) {
    float t = i / (48000.0 - 1);
    drc::s16 samp = (drc::s16)(32000 * sinf(2 * M_PI * t * 480));
    samples[2 * i] = samp;
    samples[2 * i + 1] = samp;
  }
  demo::Init("simpleaudio", demo::kStreamerGLDemo);

  drc::InputData input_data;
  bool was_pressed = false;
  while (demo::HandleEvents()) {
    demo::TryPushingGLFrame();

    demo::GetStreamer()->PollInput(&input_data);
    if (input_data.valid && input_data.buttons & drc::InputData::kBtnA) {
      if (!was_pressed) {
        demo::GetStreamer()->PushAudSamples(samples);
        was_pressed = true;
      }
    } else {
      was_pressed = false;
    }

    demo::SwapBuffers();
  }

  demo::Quit();
}
