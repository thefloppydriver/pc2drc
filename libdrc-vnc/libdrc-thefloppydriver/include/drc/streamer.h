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

#include <drc/video-frame-rate.h>
#include <drc/input.h>
#include <drc/pixel-format.h>
#include <drc/types.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace drc {

class AudioStreamer;
class CmdClient;
class DeviceConfig;
class InputReceiver;
class VideoConverter;
class VideoStreamer;
class UdpServer;
class UvcUacStateSynchronizer;

class Streamer {
 public:
  static constexpr const char* kDefaultMsgBind = "192.168.1.10:50010";
  static constexpr const char* kDefaultInputBind = "192.168.1.10:50022";
  static constexpr const char* kDefaultCmdBind = "192.168.1.10:50023";
  static constexpr const char* kDefaultVideoDest = "192.168.1.11:50120";
  static constexpr const char* kDefaultAudioDest = "192.168.1.11:50121";
  static constexpr const char* kDefaultCmdDest = "192.168.1.11:50123";

  typedef std::function<void(bool, const std::vector<byte>&)>
                             CommandReplyCallback;

  Streamer(const std::string& vid_dst = kDefaultVideoDest,
           const std::string& aud_dst = kDefaultAudioDest,
           const std::string& cmd_dst = kDefaultCmdDest,
           const std::string& msg_bind = kDefaultMsgBind,
           const std::string& input_bind = kDefaultInputBind,
           const std::string& cmd_bind = kDefaultCmdBind);
  virtual ~Streamer();

  bool Start();
  void Stop();

  void ResyncStreamer();
  
  // Data streaming/receiving methods. These methods are used for the core
  // features of the gamepad: video streaming, audio streaming and input
  // polling.

  // Takes ownership of the frame.
  enum FlippingMode {
    NoFlip,
    FlipVertically
  };
  enum StretchMode {
    NoStretch,
    StretchKeepAspectRatio,
    StretchFull
  };
  void PushVidFrame(std::vector<byte>* frame, u16 width, u16 height,
                    PixelFormat pixfmt, FlippingMode flip = NoFlip,
                    StretchMode stretch = StretchKeepAspectRatio);

  // Same as PushVidFrame, but the frame needs to already be in the native
  // format for encoding: YUV420P at ScreenWidth x ScreenHeight.
  //
  // Faster: PushVidFrame requires pixel format conversion before encoding.
  void PushNativeVidFrame(std::vector<u8>* frame);

  void PushH264Chunks(std::vector<std::string>&& chunks);

  // Expects 48KHz samples.
  void PushAudSamples(const std::vector<s16>& samples);

  // Gets the most recent input data received from the Gamepad. Usually
  // refreshed at 180Hz.
  void PollInput(InputData* data);

  // Enables system input feeding, which sends input data received from the
  // GamePad directly to the operating system. This allows an application to
  // use the basic GamePad input data transparently, without any code change.
  void EnableSystemInputFeeder();

  void SetFrameRate(VideoFrameRate frame_rate);

  // Sets the mode the rendering is in so returned coordinates are expressed
  // in terms of source pixels. This is used to simplify input for the
  // modes where StretchFull is not used.

  void SetTSArea(u16 width, u16 height,
                 StretchMode stretch = StretchKeepAspectRatio);

  // More minor features are exposed through the following methods. These
  // methods provide a "bool wait" argument in order to wait for the change to
  // actually be applied. If waiting is disabled (default), these methods are
  // not guaranteed to succeed. Waiting can block for up to 10s.

  // Level must be in [0;4] (0 is minimum, 4 is maximum).
  bool SetLcdBacklight(int level, bool wait = false);

  // Retrieves the device's config from UIC
  bool SyncUICConfig(bool wait = false);

  // Attempts to shutdown the pad, not guaranteed to work.
  void ShutdownPad();

 private:
  std::unique_ptr<UdpServer> msg_server_;

  std::unique_ptr<AudioStreamer> aud_streamer_;
  std::unique_ptr<CmdClient> cmd_client_;
  std::unique_ptr<VideoConverter> vid_converter_;
  std::unique_ptr<VideoStreamer> vid_streamer_;
  std::unique_ptr<InputReceiver> input_receiver_;
  std::unique_ptr<UvcUacStateSynchronizer> uvcuac_synchronizer_;
  std::unique_ptr<DeviceConfig> device_config_;
};

}  // namespace drc
