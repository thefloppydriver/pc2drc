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

#include <cassert>
#include <cstring>
#include <drc/internal/astrm-packet.h>
#include <drc/internal/utils.h>
#include <drc/types.h>
#include <vector>

namespace drc {

AstrmPacket::AstrmPacket() {
  ResetPacket();
}

AstrmPacket::AstrmPacket(const std::vector<byte>& packet) {
  memcpy(pkt_.data(), packet.data(), packet.size());
}

AstrmPacket::~AstrmPacket() {
}

void AstrmPacket::ResetPacket() {
  memset(pkt_.data(), 0, pkt_.size());
}

AstrmFormat AstrmPacket::Format() const {
  return static_cast<AstrmFormat>(pkt_[0] >> 5);
}

AstrmPacketType AstrmPacket::PacketType() const {
  return static_cast<AstrmPacketType>(ExtractBits(pkt_[0], 2, 3));
}

u16 AstrmPacket::SeqId() const {
  return ((pkt_[0] & 3) << 8) | pkt_[1];
}

u32 AstrmPacket::Timestamp() const {
  return (pkt_[7] << 24) | (pkt_[6] << 16) | (pkt_[5] << 8) | pkt_[4];
}

const byte* AstrmPacket::Payload() const {
  return pkt_.data() + 8;
}

size_t AstrmPacket::PayloadSize() const {
  return (pkt_[2] << 8) | pkt_[3];
}

void AstrmPacket::SetFormat(AstrmFormat format) {
  pkt_[0] = InsertBits(pkt_[0], static_cast<byte>(format), 5, 8);
}

void AstrmPacket::SetPacketType(AstrmPacketType type) {
  pkt_[0] = InsertBits(pkt_[0], static_cast<byte>(type), 2, 3);
}

void AstrmPacket::SetSeqId(u16 seqid) {
  pkt_[0] = InsertBits(pkt_[0], seqid >> 8, 0, 2);
  pkt_[1] = seqid & 0xFF;
}

void AstrmPacket::SetTimestamp(u32 ts) {
  pkt_[4] = ts & 0xFF;
  pkt_[5] = (ts >> 8) & 0xFF;
  pkt_[6] = (ts >> 16) & 0xFF;
  pkt_[7] = (ts >> 24) & 0xFF;
}

void AstrmPacket::SetPayload(const byte* payload, size_t size) {
  assert(size <= kMaxAstrmPayloadSize);
  memcpy(pkt_.data() + 8, payload, size);

  pkt_[2] = size >> 8;
  pkt_[3] = size & 0xFF;
}

#define ASTRM_FLAG_FUNCS(FlagName, idx, pos) \
  bool AstrmPacket::FlagName##Flag() const { \
    return ExtractBits(pkt_[idx], pos, pos + 1); \
  } \
  \
  void AstrmPacket::Set##FlagName##Flag(bool flag) { \
    pkt_[idx] = InsertBits(pkt_[idx], flag, pos, pos + 1); \
  }

ASTRM_FLAG_FUNCS(Mono, 0, 4)
ASTRM_FLAG_FUNCS(Vibrate, 0, 3)

}  // namespace drc
