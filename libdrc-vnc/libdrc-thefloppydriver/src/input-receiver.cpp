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

#include <algorithm>
#include <drc/input.h>
#include <drc/internal/input-receiver.h>
#include <drc/internal/udp.h>
#include <mutex>
#include <string>
#include <vector>

namespace drc {

namespace {

// Range of the directional sticks. This is kind of approximate but should
// still be accurate enough. Some sticks might report values lower than the
// minimum or higher than the maximum - just clamp in that case.
const s16 kDrcStickMin = 900;
const s16 kDrcStickMax = 3200;

// Dead zone: values that are < to the dead zone will be converted to 0 instead
// to avoid noise.
const float kStickDeadZone = 0.1;

}  // namespace


// Initial calibration values below are those that the device itself
// uses prior to loading the real ones from UIC config.
InputReceiver::InputReceiver(const std::string& hid_bind)
    : server_(new UdpServer(hid_bind)),
      ref_1_x_(20), ref_1_y_(20), ref_2_x_(834), ref_2_y_(460),
      raw_1_x_(195), raw_1_y_(3818), raw_2_x_(3877), raw_2_y_(373),
      margin_x_(0.0f), size_x_(1.0f), margin_y_(0.0f), size_y_(1.0f) {
  Recalibrate();
}

InputReceiver::~InputReceiver() {
  Stop();
}

bool InputReceiver::Start() {
  // Packets are usually received at 180Hz. If nothing was received after 1/60s
  // (3 packets missed), timeout.
  server_->SetTimeout(1000000L / 60);

  server_->SetReceiveCallback([=](const std::vector<byte>& msg) {
    ProcessInputMessage(msg);
  });
  server_->SetTimeoutCallback([=]() {
    ProcessInputTimeout();
  });

  if (!server_->Start()) {
    return false;
  }
  return true;
}

void InputReceiver::Stop() {
  server_->Stop();
}

void InputReceiver::Poll(InputData* data) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  *data = current_;
}


void InputReceiver::SetMargins(float margin_x, float size_x,
                               float margin_y, float size_y) {
  margin_x_ = margin_x;
  size_x_ = size_x;
  margin_y_ = margin_y;
  size_y_ = size_y;

  Recalibrate();
}

void InputReceiver::SetCalibrationPoints(
    s32 ref_1_x, s32 ref_1_y, s32 ref_2_x, s32 ref_2_y,
    s32 raw_1_x, s32 raw_1_y, s32 raw_2_x, s32 raw_2_y) {
  ref_1_x_ = ref_1_x;
  ref_1_y_ = ref_1_y;
  ref_2_x_ = ref_2_x;
  ref_2_y_ = ref_2_y;
  raw_1_x_ = raw_1_x;
  raw_1_y_ = raw_1_y;
  raw_2_x_ = raw_2_x;
  raw_2_y_ = raw_2_y;

  Recalibrate();
}


void InputReceiver::Recalibrate() {
  ts_ox_ = static_cast<float>(raw_2_x_ * ref_1_x_ - raw_1_x_ * ref_2_x_) /
                  (raw_2_x_ - raw_1_x_);
  ts_w_ = static_cast<float>(ref_1_x_ - ref_2_x_) / (raw_1_x_ - raw_2_x_);

  ts_oy_ = static_cast<float>(raw_2_y_ * ref_1_y_ - raw_1_y_ * ref_2_y_) /
                  (raw_2_y_ - raw_1_y_);
  ts_h_ = static_cast<float>(ref_1_y_ - ref_2_y_) / (raw_1_y_ - raw_2_y_);

  // post processing

  ts_ox_ = (ts_ox_ - margin_x_) / size_x_;
  ts_oy_ = (ts_oy_ - margin_y_) / size_y_;

  ts_w_ = ts_w_ / size_x_;
  ts_h_ = ts_h_ / size_y_;
}

void InputReceiver::SetCurrent(const InputData& new_current) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  current_ = new_current;
}

void InputReceiver::ProcessInputMessage(const std::vector<byte>& msg) {
  // HID packets should always be 128 bytes.
  if (msg.size() != 128) {
    return;
  }

  InputData data;

  int buttons = (msg[80] << 16) | (msg[2] << 8) | msg[3];
  data.buttons = static_cast<InputData::ButtonMask>(buttons);

  // Convert the integer stick values to floating point -1..1 range.
  float* sticks[] = { &data.left_stick_x, &data.left_stick_y,
                      &data.right_stick_x, &data.right_stick_y };
  for (size_t i = 0; i < sizeof (sticks) / sizeof (sticks[0]); ++i) {
    s16 val_int = (msg[7 + 2*i] << 8) | msg[6 + 2*i];

    // Clamp to min/max range.
    val_int = std::max(kDrcStickMin, std::min(kDrcStickMax, val_int));

    // Transform to -MID, +MID.
    s16 mid = (kDrcStickMax - kDrcStickMin) / 2;
    val_int -= kDrcStickMin + mid;

    // Divide by MID to move to -1.0, +1.0.
    *sticks[i] = static_cast<float>(val_int) / mid;

    // Apply the dead zone.
    if (*sticks[i] > -kStickDeadZone && *sticks[i] < kStickDeadZone) {
      *sticks[i] = 0.0;
    }
  }

  // Read touchscreen points and average.
  int ts_x = 0, ts_y = 0;
  const int kTsPointsCount = 10;
  for (int i = 0; i < kTsPointsCount; ++i) {
    int base = 36 + 4 * i;

    ts_x += ((msg[base + 1] & 0xF) << 8) | msg[base];
    ts_y += ((msg[base + 3] & 0xF) << 8) | msg[base + 2];
  }
  ts_x /= kTsPointsCount;
  ts_y /= kTsPointsCount;

  // Use the calibration values to convert to (0, 854) / (0, 480) then
  // normalize to (0, 1).
  data.ts_x = std::max(0.0, std::min(1.0, (ts_ox_ + ts_x * ts_w_) / 853.0));
  data.ts_y = std::max(0.0, std::min(1.0, (ts_oy_ + ts_y * ts_h_) / 479.0));

  // Read touchscreen pressure intensity.
  int ts_pressure = 0;
  ts_pressure |= ((msg[37] >> 4) & 7) << 0;
  ts_pressure |= ((msg[39] >> 4) & 7) << 3;
  ts_pressure |= ((msg[41] >> 4) & 7) << 6;
  ts_pressure |= ((msg[43] >> 4) & 7) << 9;

  // TODO(delroth): make meaningful
  data.ts_pressure = ts_pressure;
  data.ts_pressed = (ts_pressure != 0);

  data.battery_charge = msg[5];
  data.audio_volume = msg[14];
  data.power_status = static_cast<InputData::PowerStatus>(msg[4]);

  data.valid = true;
  SetCurrent(data);

  // Run the callbacks
  for (auto cb : cbs_) {
    cb(data);
  }
}

void InputReceiver::ProcessInputTimeout() {
  SetCurrent(InputData());
}

}  // namespace drc
