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

#include <deque>
#include <drc/internal/astrm-packet.h>
#include <drc/internal/audio-streamer.h>
#include <drc/internal/events.h>
#include <drc/internal/tsf.h>
#include <drc/internal/udp.h>
#include <drc/types.h>
#include <mutex>
#include <string>
#include <vector>

namespace drc {

namespace {

const u32 kSamplesPerSecond = 48000;

// Chosen to be the best compromise between packet size (must be < 1600 bytes)
// and number of packets sent per second, while keeping a round number of
// milliseconds between 2 packets.
//
// 384 samples == one 1536b packet every 8ms
const u32 kSamplesPerPacket = 384;
const u32 kPacketIntervalMs = 1000 / (kSamplesPerSecond / kSamplesPerPacket);

s32 GetTimestamp() {
  u64 tsf;
  GetTsf(&tsf);
  return static_cast<s32>(tsf & 0xFFFFFFFF);
}

}  // namespace

AudioStreamer::AudioStreamer(const std::string& dst)
    : astrm_client_(new UdpClient(dst)) {
}

AudioStreamer::~AudioStreamer() {
  Stop();
}

bool AudioStreamer::Start() {
  if (!astrm_client_->Start()) {
    Stop();
    return false;
  }
  StartEM();
  return true;
}

void AudioStreamer::Stop() {
  StopEM();
  astrm_client_->Stop();
}

void AudioStreamer::InitEventsAndRun() {
  u16 seqid = 0;

  NewRepeatedTimerEvent(kPacketIntervalMs * 1000000, [&](Event*) {
    std::vector<s16> samples;
    PopSamples(&samples, kSamplesPerPacket * 2);

    AstrmPacket pkt;
    pkt.SetPacketType(AstrmPacketType::kAudioData);
    pkt.SetFormat(AstrmFormat::kPcm48KHz);
    pkt.SetMonoFlag(false);
    pkt.SetVibrateFlag(false);
    pkt.SetSeqId(seqid);
    pkt.SetTimestamp(GetTimestamp());

    // TODO(delroth): this assumes a little endian host system
    pkt.SetPayload(reinterpret_cast<byte*>(samples.data()),
                   samples.size() * sizeof (s16));

    astrm_client_->Send(pkt.GetBytes(), pkt.GetSize());
    seqid = (seqid + 1) & 0x3FF;

    return true;
  });

  ThreadedEventMachine::InitEventsAndRun();
}

void AudioStreamer::PushSamples(const std::vector<s16>& samples) {
  std::lock_guard<std::mutex> lk(samples_mutex_);
  if (samples_.size() < kSamplesPerSecond * 32) {  // limit buffer size
    samples_.insert(samples_.end(), samples.begin(), samples.end());
  }
}

void AudioStreamer::PopSamples(std::vector<s16>* samples, u32 count) {
  std::lock_guard<std::mutex> lk(samples_mutex_);
  if (samples_.size() < count * 2) {
    samples->resize(0);
    samples->resize(count);
  } else {
    samples->insert(samples->end(), samples_.begin(),
                    samples_.begin() + count);
    samples_.erase(samples_.begin(), samples_.begin() + count);
  }
}

}  // namespace drc
