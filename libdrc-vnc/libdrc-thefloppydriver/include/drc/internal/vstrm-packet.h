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

#include <array>
#include <drc/video-frame-rate.h>
#include <drc/types.h>
#include <vector>

namespace drc {

const size_t kVstrmHeaderSize = 16;

// Limit our payload size to around 1400 bytes (DRC MTU is about 1800 bytes, so
// increasing this could help performance in the future).
const size_t kMaxVstrmPayloadSize = 1694; //1586; //set to 1600 if this doesn't work pls thx <3 -thefloppydriver

class VstrmPacket {
 public:
  VstrmPacket();
  explicit VstrmPacket(const std::vector<byte>& packet);
  virtual ~VstrmPacket();

  u16 SeqId() const;
  u32 Timestamp() const;
  bool InitFlag() const;
  bool FrameBeginFlag() const;
  bool ChunkEndFlag() const;
  bool FrameEndFlag() const;
  bool IdrFlag() const;
  VideoFrameRate FrameRate() const;
  const byte* Payload() const;
  size_t PayloadSize() const;

  void SetSeqId(u16 seqid);
  void SetTimestamp(u32 ts);
  void SetInitFlag(bool flag);
  void SetFrameBeginFlag(bool flag);
  void SetChunkEndFlag(bool flag);
  void SetFrameEndFlag(bool flag);
  void SetIdrFlag(bool flag);
  void SetFrameRate(VideoFrameRate framerate);
  void SetPayload(const byte* payload, size_t size);

  const byte* GetBytes() const { return pkt_.data(); }
  size_t GetSize() const { return kVstrmHeaderSize + PayloadSize(); }

  // Reset the packet to use default values for everything.
  void ResetPacket();

 private:
  bool GetExtOption(u8 opt, u8* val = NULL) const;
  void SetExtOption(u8 opt, u8* val = NULL);
  void ClearExtOption(u8 opt, bool has_val);

  std::array<byte, kVstrmHeaderSize + kMaxVstrmPayloadSize> pkt_;
};

}  // namespace drc
