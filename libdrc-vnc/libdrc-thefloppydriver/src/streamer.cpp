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

#include <cstring>
#include <drc/internal/audio-streamer.h>
#include <drc/internal/cmd-protocol.h>
#include <drc/internal/device-config.h>
#include <drc/internal/input-receiver.h>
#include <drc/internal/udp.h>
#include <drc/internal/uinput-feeder.h>
#include <drc/internal/uvcuac-synchronizer.h>
#include <drc/internal/video-converter.h>
#include <drc/internal/video-streamer.h>
#include <drc/streamer.h>
#include <drc/screen.h>
#include <string>
#include <vector>

namespace drc {

namespace {

bool RunGenericCmd(CmdClient* cli, int service, int method,
                   const std::vector<byte>& msg, std::vector<byte>* repl) {
  GenericCmdPacket pkt;
  pkt.SetFlags(GenericCmdPacket::kQueryFlag);
  pkt.SetServiceId(service);
  pkt.SetMethodId(method);
  pkt.SetPayload(msg.data(), msg.size());

  return cli->Query(CmdQueryType::kGenericCommand, pkt.GetBytes(),
                      pkt.GetSize(), repl);
}

void RunGenericAsyncCmd(CmdClient* cli, int service, int method,
                   const std::vector<byte>& msg, CmdState::ReplyCallback cb) {
  GenericCmdPacket pkt;
  pkt.SetFlags(GenericCmdPacket::kQueryFlag);
  pkt.SetServiceId(service);
  pkt.SetMethodId(method);
  pkt.SetPayload(msg.data(), msg.size());
  cli->AsyncQuery(CmdQueryType::kGenericCommand, pkt.GetBytes(),
                      pkt.GetSize(), cb);
}
}  // namespace

Streamer::Streamer(const std::string& vid_dst,
                   const std::string& aud_dst,
                   const std::string& cmd_dst,
                   const std::string& msg_bind,
                   const std::string& input_bind,
                   const std::string& cmd_bind)
    : msg_server_(new UdpServer(msg_bind)),
      aud_streamer_(new AudioStreamer(aud_dst)),
      cmd_client_(new CmdClient(cmd_dst, cmd_bind)),
      vid_converter_(new VideoConverter()),
      vid_streamer_(new VideoStreamer(vid_dst, aud_dst)),
      input_receiver_(new InputReceiver(input_bind)),
      uvcuac_synchronizer_(new UvcUacStateSynchronizer(cmd_client_.get())),
      device_config_(new DeviceConfig()) {
}

Streamer::~Streamer() {
  Stop();
}

bool Streamer::Start() {
  msg_server_->SetReceiveCallback(
      [=](const std::vector<byte>& msg) {
        if (msg.size() == 4 && !memcmp(msg.data(), "\1\0\0\0", 4)) {
          vid_streamer_->ResyncStream();
        }
      });

  vid_converter_->SetDoneCallback(
      [=](std::vector<byte>* yuv_frame) {
        PushNativeVidFrame(yuv_frame);
      });

  if (!msg_server_->Start() ||
      !aud_streamer_->Start() ||
      !cmd_client_->Start() ||
      !vid_converter_->Start() ||
      !vid_streamer_->Start() ||
      !input_receiver_->Start() ||
      !uvcuac_synchronizer_->Start()) {
    Stop();
    return false;
  }

  SyncUICConfig();
  return true;
}

void Streamer::Stop() {
  uvcuac_synchronizer_->Stop();
  input_receiver_->Stop();
  vid_streamer_->Stop();
  vid_converter_->Stop();
  cmd_client_->Stop();
  aud_streamer_->Stop();
  msg_server_->Stop();
}

void Streamer::ResyncStreamer() {
  vid_streamer_->ResyncStream();
}

void Streamer::PushVidFrame(std::vector<byte>* frame, u16 width, u16 height,
                            PixelFormat pixfmt, FlippingMode flip,
                            StretchMode stretch) {
  bool do_flip = (flip == FlipVertically);
  bool do_stretch = (stretch != NoStretch);
  bool keep_ar = (stretch == StretchKeepAspectRatio);
  vid_converter_->PushFrame(frame,
                            std::make_tuple(width, height, pixfmt, do_flip,
                                            do_stretch, keep_ar));
}

void Streamer::PushNativeVidFrame(std::vector<byte>* frame) {
  vid_streamer_->PushFrame(frame);
}

void Streamer::PushH264Chunks(std::vector<std::string>&& chunks) {
  vid_streamer_->PushH264Chunks(std::move(chunks));
}

void Streamer::PushAudSamples(const std::vector<s16>& samples) {
  aud_streamer_->PushSamples(samples);
}

void Streamer::PollInput(InputData* data) {
  input_receiver_->Poll(data);
}

void Streamer::EnableSystemInputFeeder() {
#ifdef __linux__
  input_receiver_->AddCallback(FeedDrcInputToUinput);
#endif
}

void Streamer::SetFrameRate(VideoFrameRate frame_rate) {
  vid_streamer_->SetFrameRate(frame_rate);
}

void Streamer::SetTSArea(u16 width, u16 height, StretchMode stretch) {
  float margin_x = 0.0f, margin_y = 0.0f, size_x = 1.0f, size_y = 1.0f;
  switch (stretch) {
    case StretchFull: {
      break;
    }
    case NoStretch: {
      margin_x = (kScreenWidth - width) / 2.0f;
      margin_y = (kScreenHeight - height) / 2.0f;

      size_x = static_cast<float>(width/kScreenWidth);
      size_y = static_cast<float>(height/kScreenHeight);
      break;
    }
    case StretchKeepAspectRatio: {
      if ((854 * height) > (480 * width)) {
        int target_w = width * 480 / height;
        margin_x = (kScreenWidth - target_w) / 2.0f;
        size_x = static_cast<float>(target_w/kScreenWidth);
      } else {
        int target_h = height * 854 / width;
        margin_y = (kScreenHeight - target_h) / 2.0f;
        size_y = static_cast<float>(target_h/kScreenHeight);
      }
      break;
    }
  }

  input_receiver_->SetMargins(margin_x, size_x, margin_y, size_y);
}

bool Streamer::SetLcdBacklight(int level, bool wait) {
  std::vector<byte> payload;
  bool rv = true;
  payload.push_back((level + 1) & 0xFF);
  if (wait) {
    rv = RunGenericCmd(cmd_client_.get(), 5, 0x14, payload, nullptr);
  } else {
    RunGenericAsyncCmd(cmd_client_.get(), 5, 0x14, payload, nullptr);
  }
  return rv;
}


bool Streamer::SyncUICConfig(bool wait) {
  std::vector<byte> query;
  bool rv = true;
  CmdState::ReplyCallback uic_config_callback =
    [this](bool success, const std::vector<byte>& reply) {
      if (success) {
        if (reply.size() == 0x310) {
          this->device_config_->LoadFromBlob(reply.data() + 0x10, 0x300);
          this->input_receiver_->SetCalibrationPoints(
              this->device_config_->GetPanelRef1()[0],
              this->device_config_->GetPanelRef1()[1],
              this->device_config_->GetPanelRef2()[0],
              this->device_config_->GetPanelRef2()[1],
              this->device_config_->GetPanelRaw1()[0],
              this->device_config_->GetPanelRaw1()[1],
              this->device_config_->GetPanelRaw2()[0],
              this->device_config_->GetPanelRaw2()[1]);
        }
      }
    };

  if (wait) {
    std::vector<byte> response;
    rv = RunGenericCmd(cmd_client_.get(), 5, 0x06, query, &response);
    uic_config_callback(rv, response);
  } else {
    RunGenericAsyncCmd(cmd_client_.get(), 5, 0x06, query, uic_config_callback);
  }
  return rv;
}

void Streamer::ShutdownPad() {
  // Sends command 0.4.1 with payload of 0x06 (SHUTDOWN)
  std::vector<byte> payload;
  payload.push_back(0x06 & 0xFF);
  RunGenericAsyncCmd(cmd_client_.get(), 4, 0x01, payload, nullptr);
}

}  // namespace drc
