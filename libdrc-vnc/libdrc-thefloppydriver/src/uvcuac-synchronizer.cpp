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
#include <drc/input.h>
#include <drc/internal/uvcuac-synchronizer.h>
#include <vector>

namespace drc {

UvcUacStateSynchronizer::UvcUacStateSynchronizer(CmdClient* cmd_client)
    : cmd_client_(cmd_client) {
}

UvcUacStateSynchronizer::~UvcUacStateSynchronizer() {
  Stop();
}

bool UvcUacStateSynchronizer::Start() {
  StartEM();
  return true;
}

void UvcUacStateSynchronizer::Stop() {
  StopEM();
}

void UvcUacStateSynchronizer::InitEventsAndRun() {
  NewRepeatedTimerEvent(1000 * 1000 * 1000, [&](Event*) {
    std::vector<byte> drc_reply_state;
    bool success = cmd_client_->Query(CmdQueryType::kUvcUacCommand,
                                      state_.GetBytes(), state_.GetSize(),
                                      &drc_reply_state);
    if (success) {
      const byte * d = drc_reply_state.data();
      state_.SetMicVolume(d[0] | (d[1] << 8));
      state_.SetMicJackVolume(d[2] | (d[3] << 8));

      // TODO(parlane) complete when someone finds out the
      //               meaning of these fields
      u32 unk_mc_video_cmos = d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24);
      (void)unk_mc_video_cmos;
      u32 unk8 = d[8] | (d[9] << 8) | (d[10] << 16) | (d[11] << 24);
      (void)unk8;

      state_.SetMicEnabled(d[12] != 0);
      state_.SetCamPowerFrequency(d[13]);
      state_.SetCamAutoExposureEnabled(d[14] != 0);
    }
    return true;
  });

  ThreadedEventMachine::InitEventsAndRun();
}

}  // namespace drc
