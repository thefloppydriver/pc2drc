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

#include <drc/pixel-format.h>
#include <drc/types.h>
#include <future>
#include <map>
#include <utility>
#include <vector>

struct SwsContext;

namespace drc {

// Defined as a tuple to avoid redefining == and <.
// Fields are w/h/pixfmt/flipv/stretch/keep_ar.
typedef std::tuple<u16, u16, PixelFormat, bool, bool, bool>
  VideoConverterParams;

class VideoConverter {
 public:
  typedef std::function<void(std::vector<byte>*)> DoneCallback;

  VideoConverter();
  virtual ~VideoConverter();

  bool Start();
  void Stop();

  void PushFrame(std::vector<byte>* frame, const VideoConverterParams& params);

  void SetDoneCallback(DoneCallback cb) { done_cb_ = cb; }

 private:
  SwsContext* GetContextForParams(const VideoConverterParams& params);
  void DoConversion();

  DoneCallback done_cb_;
  std::map<VideoConverterParams, SwsContext*> ctxs_;

  std::future<void> current_conv_;

  std::vector<byte> current_frame_;
  VideoConverterParams current_params_;
};

}  // namespace drc
