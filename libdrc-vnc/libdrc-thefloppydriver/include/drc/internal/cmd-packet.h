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

const size_t kCmdHeaderSize = 8;
const size_t kMaxCmdPayloadSize = 1400;

enum class CmdPacketType {
  kQuery = 0,
  kQueryAck = 1,
  kReply = 2,
  kReplyAck = 3
};

enum class CmdQueryType {
  kGenericCommand = 0,
  kUvcUacCommand = 1,
  kTimeCommand = 2
};

class CmdPacket {
 public:
  CmdPacket();
  explicit CmdPacket(const std::vector<byte>& packet);
  virtual ~CmdPacket();

  CmdPacketType PacketType() const;
  CmdQueryType QueryType() const;
  u16 SeqId() const;
  const byte* Payload() const;
  size_t PayloadSize() const;

  void SetPacketType(CmdPacketType type);
  void SetQueryType(CmdQueryType type);
  void SetSeqId(u16 seqid);
  void SetPayload(const byte* payload, size_t size);

  const byte* GetBytes() const { return pkt_.data(); }
  size_t GetSize() const { return kCmdHeaderSize + PayloadSize(); }

  // Reset the packet to use default values for everything.
  void ResetPacket();

 private:
  std::array<byte, kCmdHeaderSize + kMaxCmdPayloadSize> pkt_;
};

const size_t kGenericCmdHeaderSize = 12;
const size_t kMaxGenericCmdPayloadSize =
    kMaxCmdPayloadSize - kGenericCmdHeaderSize;

class GenericCmdPacket {
 public:
  enum FlagsType {
    kQueryFlag = 0x40,
  };

  GenericCmdPacket();
  explicit GenericCmdPacket(const std::vector<byte>& packet);
  virtual ~GenericCmdPacket();

  u16 TransactionId() const;
  bool FinalFragment() const;
  u16 FragmentId() const;
  u8 Flags() const;
  u8 ServiceId() const;
  u8 MethodId() const;
  u16 ErrorCode() const;
  const byte* Payload() const;
  size_t PayloadSize() const;

  void SetTransactionId(u16 tid);
  void SetFinalFragment(bool v);
  void SetFragmentId(u16 fid);
  void SetFlags(u8 flags);
  void SetServiceId(u8 sid);
  void SetMethodId(u8 mid);
  void SetErrorCode(u16 error_code);
  void SetPayload(const byte* payload, size_t size);

  const byte* GetBytes() const { return pkt_.data(); }
  size_t GetSize() const { return kGenericCmdHeaderSize + PayloadSize(); }

  // Reset the packet to use default values for everything.
  void ResetPacket();

 private:
  std::array<byte, kGenericCmdHeaderSize + kMaxGenericCmdPayloadSize> pkt_;
};

const size_t kUvcUacCmdPacketSize = 48;

class UvcUacCmdPacket {
 public:
  UvcUacCmdPacket();
  explicit UvcUacCmdPacket(const std::vector<byte>& packet);
  virtual ~UvcUacCmdPacket();

  bool MicEnabled() const;
  bool MicMuted() const;
  s16 MicVolume() const;
  s16 MicJackVolume() const;
  u16 MicSamplingRate() const;
  bool CamEnabled() const;
  u8 CamPowerFrequency() const;
  bool CamAutoExposureEnabled() const;

  void SetMicEnabled(bool flag);
  void SetMicMuted(bool flag);
  void SetMicVolume(s16 vol);
  void SetMicJackVolume(s16 vol);
  void SetMicSamplingRate(u16 sampling_rate);
  void SetCamEnabled(bool enabled);
  void SetCamPower(u8 power);
  void SetCamPowerFrequency(u8 freq);
  void SetCamAutoExposureEnabled(bool auto_exp);

  const byte* GetBytes() const { return pkt_.data(); }
  size_t GetSize() const { return kUvcUacCmdPacketSize; }

  // Reset the packet to use default values for everything.
  void ResetPacket();

 private:
  std::array<byte, kUvcUacCmdPacketSize> pkt_;
};

const size_t kTimeCmdPacketSize = 8;

class TimeCmdPacket {
 public:
  TimeCmdPacket();
  explicit TimeCmdPacket(const std::vector<byte>& packet);
  virtual ~TimeCmdPacket();

  u16 Days() const;
  u32 Seconds() const;

  void SetDays(u16 days);
  void SetSeconds(u32 seconds);

  const byte* GetBytes() const { return pkt_.data(); }
  size_t GetSize() const { return kTimeCmdPacketSize; }

  // Reset the packet to use default values for everything.
  void ResetPacket();

 private:
  std::array<byte, kTimeCmdPacketSize> pkt_;
};

}  // namespace drc
