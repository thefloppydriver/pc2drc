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
#include <drc/internal/vstrm-packet.h>
#include <drc/internal/utils.h>
#include <drc/screen.h>
#include <drc/types.h>
#include <vector>

namespace drc {

VstrmPacket::VstrmPacket() {
  ResetPacket();
}

VstrmPacket::VstrmPacket(const std::vector<byte>& packet) {
  memcpy(pkt_.data(), packet.data(), packet.size());
}

VstrmPacket::~VstrmPacket() {
}

void VstrmPacket::ResetPacket() {
  memset(pkt_.data(), 0, pkt_.size());
  pkt_[0] = 0xF0;  // 4 bit magic (F), 2 bit packet type (0), 2 bit seqid (0)
  pkt_[2] = 0x08;  // Has timestamp = 1
  pkt_[8] = 0x83;  // "Force decoding" option, enabled on DRH vstrm messages
  pkt_[9] = 0x85;  // Macro blocks per row option, value set below
  pkt_[10] = 6;    // 6 macroblock row per chunk (6 * 5 = 480 / 16)
}

u16 VstrmPacket::SeqId() const {
  return ((pkt_[0] & 3) << 8) | pkt_[1];
}

u32 VstrmPacket::Timestamp() const {
  return (pkt_[4] << 24) | (pkt_[5] << 16) | (pkt_[6] << 8) | pkt_[7];
}

bool VstrmPacket::IdrFlag() const {
  return GetExtOption(0x80);
}

VideoFrameRate VstrmPacket::FrameRate() const {
  byte framerate;
  if (!GetExtOption(0x82, &framerate)) {
    return VideoFrameRate::kUnknown;
  }
  return static_cast<VideoFrameRate>(framerate);
}

const byte* VstrmPacket::Payload() const {
  return pkt_.data() + 16;
}

size_t VstrmPacket::PayloadSize() const {
  return ((pkt_[2] & 7) << 8) | pkt_[3];
}

void VstrmPacket::SetSeqId(u16 seqid) {
  pkt_[0] = InsertBits(pkt_[0], seqid >> 8, 0, 2);
  pkt_[1] = seqid & 0xFF;
}

void VstrmPacket::SetTimestamp(u32 ts) {
  pkt_[4] = (ts >> 24) & 0xFF;
  pkt_[5] = (ts >> 16) & 0xFF;
  pkt_[6] = (ts >> 8) & 0xFF;
  pkt_[7] = ts & 0xFF;
}

void VstrmPacket::SetIdrFlag(bool flag) {
  if (flag) {
    SetExtOption(0x80);
  } else {
    ClearExtOption(0x80, false);
  }
}

void VstrmPacket::SetFrameRate(VideoFrameRate framerate) {
  if (framerate == VideoFrameRate::kUnknown) {
    ClearExtOption(0x82, true);
  } else {
    byte val = static_cast<byte>(framerate);
    SetExtOption(0x82, &val);
  }
}

void VstrmPacket::SetPayload(const byte* payload, size_t size) {
  assert(size <= kMaxVstrmPayloadSize);
  memcpy(pkt_.data() + 16, payload, size);

  pkt_[2] = InsertBits(pkt_[2], size >> 8, 0, 3);
  pkt_[3] = size & 0xFF;
}

#define VSTRM_FLAG_FUNCS(FlagName, idx, pos) \
  bool VstrmPacket::FlagName##Flag() const { \
    return ExtractBits(pkt_[idx], pos, pos + 1); \
  } \
  \
  void VstrmPacket::Set##FlagName##Flag(bool flag) { \
    pkt_[idx] = InsertBits(pkt_[idx], flag, pos, pos + 1); \
  }

VSTRM_FLAG_FUNCS(Init, 2, 7)
VSTRM_FLAG_FUNCS(FrameBegin, 2, 6)
VSTRM_FLAG_FUNCS(ChunkEnd, 2, 5)
VSTRM_FLAG_FUNCS(FrameEnd, 2, 4)

bool VstrmPacket::GetExtOption(u8 opt, u8* val) const {
  for (int i = 0; i < 8; ++i) {
    if (pkt_[8 + i] == opt) {
      if (val) {
        if (i == 7) {
          return false;
        } else {
          *val = pkt_[8 + i + 1];
        }
      }
      return true;
    }
  }
  return false;
}

void VstrmPacket::SetExtOption(u8 opt, u8* val) {
  int i;
  for (i = 0; i < 8 && pkt_[8 + i] != 0x00; ++i) {
    if (pkt_[8 + i] == opt) {
      if (val) {
        pkt_[8 + i + 1] = *val;
      }
      return;
    }
  }
  int required_space = (val == NULL ? 1 : 2);
  assert((i + required_space <= 8) || "out of space in vstrm ext header");
  pkt_[8 + i] = opt;
  if (val) {
    pkt_[8 + i + 1] = *val;
  }
}

void VstrmPacket::ClearExtOption(u8 opt, bool has_val) {
  for (int i = 0; i < 8; ++i) {
    if (pkt_[8 + i] == opt) {
      for (int j = i + 1 + has_val; j < 8; ++i, ++j) {
        pkt_[8 + i] = pkt_[8 + j];
        pkt_[8 + j] = 0;
      }
    }
  }
}

}  // namespace drc
