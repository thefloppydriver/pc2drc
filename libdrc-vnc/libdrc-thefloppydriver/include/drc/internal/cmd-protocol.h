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

#include <drc/internal/cmd-packet.h>
#include <drc/internal/cmd-protocol.h>
#include <drc/internal/events.h>
#include <drc/types.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace drc {

class UdpClient;
class UdpServer;

struct CmdState {
  // Will be called when a CmdServer received a query for which the callback
  // has been registered.
  typedef std::function<void(const std::vector<byte>&,
                             std::vector<byte>&)> QueryCallback;

  // Will be called when a CmdClient receives a response from the server.
  typedef std::function<void(bool, const std::vector<byte>&)> ReplyCallback;

  CmdPacketType state;
  CmdQueryType query_type;
  QueryCallback query_callback;
  ReplyCallback reply_callback;
  std::vector<byte> query_payload;
  std::vector<byte> reply_payload;
  Event* timeout_evt;
  int retries_count;
};

class CmdClient : public ThreadedEventMachine {
 public:
  CmdClient(const std::string& cmd_dst, const std::string& cmd_bind);
  virtual ~CmdClient();

  bool Start();
  void Stop();

  void AsyncQuery(CmdQueryType qtype, const byte* payload, size_t size,
                  CmdState::ReplyCallback cb);

  // Be careful: this can take up to 10s to retry and detect timeout.
  bool Query(CmdQueryType qtype, const byte* payload, size_t size,
             std::vector<byte>* reply);

 private:
  CmdState* FindState(u16 seqid);
  void RemoveState(u16 seqid);

  void PacketReceived(const std::vector<byte>& msg);

  void SendQuery(u16 seqid, CmdState* cmd_st);
  void SendReplyAck(u16 seqid, CmdQueryType qtype);
  bool RetryOperation(u16 seqid);

  std::unique_ptr<UdpClient> udp_client_;
  std::unique_ptr<UdpServer> udp_server_;

  std::mutex state_mutex_;
  u16 seqid_;
  std::map<u16, CmdState> cmd_in_flight_;
};

class CmdServer {
  // TODO(delroth): should probably reuse some of CmdClient.
};

}  // namespace drc
