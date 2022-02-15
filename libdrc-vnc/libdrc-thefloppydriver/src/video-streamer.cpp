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
#include <cstdio>
#include <drc/internal/astrm-packet.h>
#include <drc/internal/h264-encoder.h>
#include <drc/internal/tsf.h>
#include <drc/internal/udp.h>
#include <drc/internal/video-streamer.h>
#include <drc/internal/vstrm-packet.h>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>




namespace drc {

namespace {



s32 GetTimestamp() {
  u64 tsf;
  GetTsf(&tsf);
  return static_cast<s32>(tsf & 0xFFFFFFFF);
}

void GenerateVstrmPackets(std::vector<VstrmPacket>* vstrm_packets,
                          const H264ChunkArray& chunks, u32 timestamp,
                          bool idr, bool* vstrm_inited, u16* vstrm_seqid,
                          VideoFrameRate frame_rate) {
  // Set the init flag on the first frame ever sent.
  bool init_flag = !*vstrm_inited;
  if (init_flag) {
    *vstrm_inited = true;
  }

  for (size_t i = 0; i < chunks.size(); ++i) {
    bool first_chunk = (i == 0);
    bool last_chunk = (i == chunks.size() - 1);

    const byte* chunk_data;
    size_t chunk_size;
    std::tie(chunk_data, chunk_size) = chunks[i];

    bool first_packet = true;
    do {
      const byte* pkt_data = chunk_data;
      size_t pkt_size = std::min(kMaxVstrmPayloadSize, chunk_size);

      chunk_data += pkt_size;
      chunk_size -= pkt_size;

      bool last_packet = (chunk_size == 0);

      vstrm_packets->resize(vstrm_packets->size() + 1);
      VstrmPacket* pkt = &vstrm_packets->at(vstrm_packets->size() - 1);

      u16 seqid = (*vstrm_seqid)++;
      if (*vstrm_seqid >= 1024) {
        *vstrm_seqid = 0;
      }
      pkt->SetSeqId(seqid);

      pkt->SetPayload(pkt_data, pkt_size);
      pkt->SetTimestamp(timestamp);

      pkt->SetInitFlag(init_flag);
      pkt->SetFrameBeginFlag(first_chunk && first_packet);
      pkt->SetChunkEndFlag(last_packet);
      pkt->SetFrameEndFlag(last_chunk && last_packet);

      pkt->SetIdrFlag(idr);
      pkt->SetFrameRate(frame_rate);

      first_packet = false;
    } while (chunk_size != 0);
  }
}

void GenerateAstrmPacket(AstrmPacket* pkt, u32 ts) {
  pkt->ResetPacket();
  pkt->SetPacketType(AstrmPacketType::kVideoFormat);
  //pkt->SetTimestamp(0x00100000);
  pkt->SetTimestamp(0x00000000);

  byte payload[24] = {
      0x00, 0x00, 0x00, 0x00,
      0x80, 0x3e, 0x00, 0x00,
      0x80, 0x3e, 0x00, 0x00,
      0x80, 0x3e, 0x00, 0x00,
      0x80, 0x3e, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
  };
  payload[0] = ts & 0xFF;
  payload[1] = (ts >> 8) & 0xFF;
  payload[2] = (ts >> 16) & 0xFF;
  payload[3] = (ts >> 24) & 0xFF;
  pkt->SetPayload(payload, sizeof (payload));
}

std::unique_ptr<H264ChunkArray> AsChunkArray(
    const std::vector<std::string>& chunks) {
  auto chunk_array = std::make_unique<H264ChunkArray>();
  size_t index = 0u;
  for (const auto& chunk : chunks) {
    if (index >= kH264ChunksPerFrame) {
      // More chunks than expected.
      break;
    }
    (*chunk_array)[index] = std::make_tuple(
        reinterpret_cast<const unsigned char*>(chunk.data()), chunk.size());
    index++;
  }
  return chunk_array;
}

}  // namespace

VideoStreamer::VideoStreamer(const std::string& vid_dst,
                             const std::string& aud_dst)
    : astrm_client_(new UdpClient(aud_dst)),
      vstrm_client_(new UdpClient(vid_dst)),
      encoder_(new H264Encoder()),
      resync_evt_(NULL) {
}

VideoStreamer::~VideoStreamer() {
  Stop();
}

bool VideoStreamer::Start() {
  if (!astrm_client_->Start() || !vstrm_client_->Start()) {
    Stop();
    return false;
  }

  StartEM();
  return true;
}

void VideoStreamer::Stop() {
  StopEM();
  resync_evt_ = NULL;

  astrm_client_->Stop();
  vstrm_client_->Stop();
}

void VideoStreamer::PushFrame(std::vector<byte>* frame) {
  std::lock_guard<std::mutex> lk(frame_mutex_);
  frame_ = std::move(*frame);
}

void VideoStreamer::PushH264Chunks(std::vector<std::string>&& chunks) {
  std::lock_guard<std::mutex> lk(frame_mutex_);
  h264_chunks_ = chunks;
}

void VideoStreamer::ResyncStream() {
  if (resync_evt_) {
    resync_evt_->Trigger();
  }
}

void VideoStreamer::SetFrameRate(VideoFrameRate frame_rate) {
  frame_rate_ = frame_rate;
}

void VideoStreamer::InitEventsAndRun() {
  bool resync_requested = false;
  resync_evt_ = NewTriggerableEvent([&](Event*) {
    resync_requested = true;
    return true;
  });

  std::vector<std::string> h264_chunks;
  std::vector<byte> encoding_frame;

  bool vstrm_inited = false;
  u16 vstrm_seqid = 0;
  std::vector<VstrmPacket> vstrm_packets;
  bool send_idr = false;

  AstrmPacket astrm_packet;
  s32 next_timestamp = GetTimestamp();
  TimerEvent* timer_evt = NewTimerEvent(1, [&](Event*) {
    if (vstrm_inited) {
      astrm_client_->Send(astrm_packet.GetBytes(), astrm_packet.GetSize());
      for (const auto& pkt : vstrm_packets) {
        vstrm_client_->Send(pkt.GetBytes(), pkt.GetSize());
      }
      vstrm_packets.clear();
      if (send_idr && resync_requested) {
        resync_requested = false;
      }
    }

    s32 timestamp = GetTimestamp();
    LatchOnCurrentChunks(&h264_chunks);
    LatchOnCurrentFrame(&encoding_frame);
    std::unique_ptr<H264ChunkArray> chunk_array;
    if (h264_chunks.size() > 0) {
      chunk_array = AsChunkArray(h264_chunks);
    }
    if (chunk_array || encoding_frame.size() > 0) {
      send_idr = resync_requested || !vstrm_inited;
      const H264ChunkArray& chunks =
          chunk_array ?
              *chunk_array : encoder_->Encode(encoding_frame, send_idr);
      GenerateVstrmPackets(&vstrm_packets, chunks, timestamp, send_idr,
                           &vstrm_inited, &vstrm_seqid, frame_rate_);
      GenerateAstrmPacket(&astrm_packet, timestamp);

      vstrm_inited = true;
      resync_requested = false;
    }

    timestamp = GetTimestamp();
    next_timestamp += static_cast<s32>(1000000.0/59.94);
    s32 delta_next = next_timestamp - timestamp;
    if (delta_next < 1000) {
      delta_next = 1000;
    }
    timer_evt->RearmTimer((u64)delta_next * 1000);

    return true;
  });

  ThreadedEventMachine::InitEventsAndRun();
}

void VideoStreamer::LatchOnCurrentFrame(std::vector<byte>* latched_frame) {
  std::lock_guard<std::mutex> lk(frame_mutex_);
  if (frame_.size() != 0) {
    *latched_frame = std::move(frame_);
  }
}

void VideoStreamer::LatchOnCurrentChunks(
    std::vector<std::string>* latched_chunk) {
  std::lock_guard<std::mutex> lk(frame_mutex_);
  if (h264_chunks_.size() != 0) {
    *latched_chunk = std::move(h264_chunks_);
  }
}

}  // namespace drc
