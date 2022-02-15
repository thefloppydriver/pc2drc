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

#include <cstdio>
#include <cstring>
#include <drc/input.h>
#include <drc/internal/uinput-feeder.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <unistd.h>

namespace drc {

namespace {

// Virtual stick radius used by uinput.
const int kStickRadius = 128;

// TODO(delroth): Implicit initialization really sucks in my opinion. System
// input feeders should be refactored to be objects that expose a Start/Stop
// methods pair.
int InitUinput() {
  // Try to open the uinput file descriptor.
  const char* uinput_filename[] = {
    "/dev/uinput",
    "/dev/input/uinput",
    "/dev/misc/uinput"
  };

  int uinput_fd;
  for (auto fn : uinput_filename) {
    if ((uinput_fd = open(fn, O_RDWR)) >= 0) {
      break;
    }
  }

  if (uinput_fd < 0) {
    return -1;
  }

  // Create the virtual device.
  struct uinput_user_dev dev;
  memset(&dev, 0, sizeof (dev));
  strncpy(dev.name, "Wii U GamePad (libdrc)", sizeof (dev.name));
  dev.id.bustype = BUS_VIRTUAL;

  // Enable joysticks: 4 axis (2 for each stick).
  for (int i = 0; i < 4; ++i) {
    dev.absmin[i] = -kStickRadius;
    dev.absmax[i] = kStickRadius;
    ioctl(uinput_fd, UI_SET_ABSBIT, i);
  }

  // Enable 32 buttons (we only use 19 of these, but it makes things easier).
  for (int i = 0; i < 32; ++i) {
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_JOYSTICK + i);
  }

  // Set the absolute and key bits.
  ioctl(uinput_fd, UI_SET_EVBIT, EV_ABS);
  ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);

  // Create the device.
  if (write(uinput_fd, &dev, sizeof (dev)) != sizeof (dev)) {
    perror("write failed - creating the device");
  }
  ioctl(uinput_fd, UI_DEV_CREATE);

  return uinput_fd;
}

}  // namespace

void FeedDrcInputToUinput(const InputData& inp) {
  static bool initialized = false;
  static int uinput_fd = -1;
  if (!initialized) {
    initialized = true;
    uinput_fd = InitUinput();
    if (uinput_fd == -1) {
      perror("uinput feeder");
    }
  }

  if (uinput_fd == -1) {
    return;
  }

  // Get the current timestamp
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // 32 button events + 4 axis events + 1 report event.
  struct input_event evt[32 + 4 + 1];
  memset(&evt, 0, sizeof (evt));

  // Read the 32 buttons and initialize their event object.
  for (int i = 0; i < 32; ++i) {
    evt[i].type = EV_KEY;
    evt[i].code = BTN_JOYSTICK + i;
    evt[i].value = !!(inp.buttons & (1 << i));
    memcpy(&evt[i].time, &tv, sizeof (tv));
  }

  // Read the 4 axis values.
  float sticks[] = {
    inp.left_stick_x, inp.left_stick_y,
    inp.right_stick_x, inp.right_stick_y
  };
  for (int i = 32; i < 32 + 4; ++i) {
    evt[i].type = EV_ABS;
    evt[i].code = i - 32;
    evt[i].value = static_cast<int>(sticks[i - 32] * kStickRadius);
    memcpy(&evt[i].time, &tv, sizeof (tv));
  }

  // Report event.
  evt[36].type = EV_SYN;
  evt[36].code = SYN_REPORT;
  memcpy(&evt[36].time, &tv, sizeof (tv));

  // Send the events to uinput.
  for (auto& e : evt) {
    if (write(uinput_fd, &e, sizeof (e)) != sizeof (e)) {
      perror("write failed - sending an event to uinput");
    }
  }
}

}  // namespace drc
