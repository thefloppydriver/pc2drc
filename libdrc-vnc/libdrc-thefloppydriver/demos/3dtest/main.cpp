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

#include <drc/input.h>
#include <drc/screen.h>
#include <GL/glu.h>

namespace {

void InitRendering() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glViewport(0, 0, drc::kScreenWidth, drc::kScreenHeight);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, drc::kScreenAspectRatio, 0.1, 100.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void RenderFrame(const drc::InputData& input_data) {
  static float rot[2], trans[2], bkgd;
  static int bklt_level = 4;

  if (input_data.valid) {
    trans[0] += input_data.left_stick_x / 40.0;
    trans[1] += input_data.left_stick_y / 40.0;

    rot[0] += input_data.right_stick_x * 2.0;
    rot[1] += input_data.right_stick_y * 2.0;

    if (input_data.buttons & drc::InputData::kBtnA) {
      bkgd = 1.0;
    }

    int new_bklt_level = bklt_level;
    if (input_data.buttons & drc::InputData::kBtnUp && new_bklt_level < 4) {
      new_bklt_level++;
    } else if (input_data.buttons & drc::InputData::kBtnDown &&
               new_bklt_level > 0) {
      new_bklt_level--;
    }

    if (new_bklt_level != bklt_level) {
      bklt_level = new_bklt_level;
      demo::GetStreamer()->SetLcdBacklight(bklt_level);
    }
  }

  glClearColor(bkgd, bkgd, bkgd, 0.0);
  glClearDepth(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  glTranslated(0.0, 0.0, -6.0);

  glTranslated(trans[0], trans[1], 0.0);
  glRotated(rot[0], 0.0, 1.0, 0.0);
  glRotated(rot[1], 1.0, 0.0, 0.0);

  // Draw a cube (from NeHe's tutorials...).
  glBegin(GL_QUADS);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);

    glColor3f(1.0f, 0.5f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);

    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);

    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);

    glColor3f(1.0f, 0.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
  glEnd();

  if (bkgd > 0.0) {
    bkgd -= 0.02;
  }
}

}  // namespace

int main() {
  demo::Init("3dtest", demo::kStreamerGLDemo);

  InitRendering();

  drc::InputData input_data;
  while (demo::HandleEvents()) {
    demo::GetStreamer()->PollInput(&input_data);
    RenderFrame(input_data);

    demo::TryPushingGLFrame();
    demo::SwapBuffers();
  }

  demo::Quit();
  return 0;
}
