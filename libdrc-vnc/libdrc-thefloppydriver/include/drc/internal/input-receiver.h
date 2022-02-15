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

#pragma once

#include <drc/input.h>
#include <drc/types.h>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace drc {

class UdpServer;

class InputReceiver {
 public:
  typedef std::function<void(const InputData&)> Callback;

  explicit InputReceiver(const std::string& hid_bind);
  virtual ~InputReceiver();

  bool Start();
  void Stop();

  void AddCallback(Callback cb) { cbs_.push_back(cb); }

  void Poll(InputData* data);

  void SetMargins(float margin_x, float size_x, float margin_y, float size_y);

  void SetCalibrationPoints(s32 raw_1_x, s32 raw_1_y, s32 raw_2_x, s32 raw_2_y,
                            s32 ref_1_x, s32 ref_1_y, s32 ref_2_x, s32 ref_2_y);

  void Recalibrate();

 private:
  void SetCurrent(const InputData& new_current);

  void ProcessInputMessage(const std::vector<byte>& data);
  void ProcessInputTimeout();

  std::unique_ptr<UdpServer> server_;

  InputData current_;
  std::mutex current_mutex_;

  std::vector<Callback> cbs_;

  // Touchscreen calibration parameters.
  s32 ref_1_x_, ref_1_y_, ref_2_x_, ref_2_y_;
  s32 raw_1_x_, raw_1_y_, raw_2_x_, raw_2_y_;
  float margin_x_, size_x_, margin_y_, size_y_;
  float ts_ox_, ts_oy_, ts_w_, ts_h_;
};

}  // namespace drc
