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
#include <drc/types.h>
#include <vector>

namespace drc {

const size_t kAstrmHeaderSize = 8;
const size_t kMaxAstrmPayloadSize = 1700;

enum class AstrmFormat {
  kPcm24KHz = 0,
  kPcm48KHz = 1,
  kAlaw24KHz = 2,
  kAlaw48KHz = 3,
  kUlaw24KHz = 4,
  kUlaw48KHz = 5,
};

enum class AstrmPacketType {
  kAudioData = 0,
  kVideoFormat = 1,
};

class AstrmPacket {
 public:
  AstrmPacket();
  explicit AstrmPacket(const std::vector<byte>& packet);
  virtual ~AstrmPacket();

  AstrmFormat Format() const;
  bool MonoFlag() const;
  bool VibrateFlag() const;
  AstrmPacketType PacketType() const;
  u16 SeqId() const;
  u32 Timestamp() const;
  const byte* Payload() const;
  size_t PayloadSize() const;

  void SetFormat(AstrmFormat format);
  void SetMonoFlag(bool flag);
  void SetVibrateFlag(bool flag);
  void SetPacketType(AstrmPacketType type);
  void SetSeqId(u16 seqid);
  void SetTimestamp(u32 timestamp);
  void SetPayload(const byte* payload, size_t size);

  const byte* GetBytes() const { return pkt_.data(); }
  size_t GetSize() const { return kAstrmHeaderSize + PayloadSize(); }

  // Reset the packet to use default values for everything.
  void ResetPacket();

 private:
  std::array<byte, kAstrmHeaderSize + kMaxAstrmPayloadSize> pkt_;
};

}  // namespace drc
