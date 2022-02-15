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
#include <condition_variable>
#include <drc/internal/cmd-packet.h>
#include <drc/internal/cmd-protocol.h>
#include <drc/internal/udp.h>
#include <drc/types.h>
#include <mutex>
#include <string>
#include <vector>

namespace drc {

namespace {
const int kTimeoutMs = 1000;
const int kMaxRetries = 10;
}  // namespace

CmdClient::CmdClient(const std::string& cmd_dst, const std::string& cmd_bind)
    : udp_client_(new UdpClient(cmd_dst)),
      udp_server_(new UdpServer(cmd_bind)),
      seqid_(0) {
}

CmdClient::~CmdClient() {
  Stop();
}

bool CmdClient::Start() {
  udp_server_->SetReceiveCallback([&](const std::vector<byte>& msg) {
    PacketReceived(msg);
  });

  if (!udp_client_->Start() || !udp_server_->Start()) {
    Stop();
    return false;
  }

  StartEM();
  return true;
}

void CmdClient::Stop() {
  StopEM();
  udp_server_->Stop();
  udp_client_->Stop();
}

void CmdClient::AsyncQuery(CmdQueryType qtype, const byte* payload,
                           size_t size, CmdState::ReplyCallback cb) {
  u16 seqid;
  CmdState* cmd_st;

  {
    std::lock_guard<std::mutex> lk(state_mutex_);

    seqid = seqid_++;
    assert(cmd_in_flight_.find(seqid) == cmd_in_flight_.end());
    cmd_st = &cmd_in_flight_[seqid];
  }

  cmd_st->state = CmdPacketType::kQuery;
  cmd_st->query_type = qtype;
  cmd_st->query_payload.assign(payload, payload + size);
  cmd_st->reply_callback = cb;
  cmd_st->retries_count = 0;

  SendQuery(seqid, cmd_st);
}

bool CmdClient::Query(CmdQueryType qtype, const byte* payload,
                      size_t size, std::vector<byte>* reply) {
  bool success = false;
  bool callback_done = false;
  std::mutex mtx;
  std::condition_variable cv;
  std::unique_lock<std::mutex> outer_lck(mtx);

  AsyncQuery(qtype, payload, size,
      [&](bool succ, const std::vector<byte>& data) {
        std::unique_lock<std::mutex> inner_lck(mtx);
        if (reply) {
          reply->assign(data.begin(), data.end());
        }
        success = succ;
        callback_done = true;
        cv.notify_all();
      });

  cv.wait(outer_lck, [&callback_done]() {
    return callback_done;
  });

  return success;
}

CmdState* CmdClient::FindState(u16 seqid) {
  std::lock_guard<std::mutex> lk(state_mutex_);
  auto it = cmd_in_flight_.find(seqid);
  if (it == cmd_in_flight_.end()) {
    return NULL;
  }
  return &it->second;
}

void CmdClient::RemoveState(u16 seqid) {
  std::lock_guard<std::mutex> lk(state_mutex_);
  cmd_in_flight_.erase(seqid);
}

void CmdClient::PacketReceived(const std::vector<byte>& msg) {
  CmdPacket pkt(msg);


  // If the packet is a reply, send a reply ack regardless in response.
  if (pkt.PacketType() == CmdPacketType::kReply) {
    SendReplyAck(pkt.SeqId(), pkt.QueryType());
  }

  u16 seqid = pkt.SeqId();
  CmdState* st = FindState(seqid);
  if (st == NULL) {
    return;
  }

  if (pkt.PacketType() != st->state) {
    return;
  }

  if (pkt.PacketType() == CmdPacketType::kReply) {
    CancelEvent(st->timeout_evt);
    st->timeout_evt = NULL;
  }

  if (pkt.PacketType() == CmdPacketType::kQueryAck) {
    st->state = CmdPacketType::kReply;
  } else if (pkt.PacketType() == CmdPacketType::kReply) {
    if (st->reply_callback != nullptr) {
      std::vector<byte> repl(pkt.Payload(), pkt.Payload() + pkt.PayloadSize());
      st->reply_callback(true, repl);
    }
    RemoveState(seqid);
  }
}

void CmdClient::SendQuery(u16 seqid, CmdState* cmd_st) {
  CmdPacket pkt;
  pkt.SetPacketType(CmdPacketType::kQuery);
  pkt.SetQueryType(cmd_st->query_type);
  pkt.SetSeqId(seqid);
  pkt.SetPayload(cmd_st->query_payload.data(), cmd_st->query_payload.size());

  cmd_st->timeout_evt = NewTimerEvent(kTimeoutMs * 1000000,
                                      [this, seqid](Event* ev) {
    return this->RetryOperation(seqid);
  });

  cmd_st->state = CmdPacketType::kQueryAck;
  udp_client_->Send(pkt.GetBytes(), pkt.GetSize());
}

void CmdClient::SendReplyAck(u16 seqid, CmdQueryType qtype) {
  CmdPacket pkt;
  pkt.SetPacketType(CmdPacketType::kReplyAck);
  pkt.SetQueryType(qtype);
  pkt.SetSeqId(seqid);
  udp_client_->Send(pkt.GetBytes(), pkt.GetSize());
}

bool CmdClient::RetryOperation(u16 seqid) {
  CmdState* state = FindState(seqid);
  assert(state != NULL);

  state->retries_count++;
  if (state->retries_count >= kMaxRetries) {
    if (state->reply_callback != nullptr) {
      std::vector<byte> dummy;
      state->reply_callback(false, dummy);
    }
    RemoveState(seqid);
  } else {
    SendQuery(seqid, state);
  }
  return false;
}

}  // namespace drc
