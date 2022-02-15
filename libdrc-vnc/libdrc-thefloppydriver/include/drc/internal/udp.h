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

#include <drc/internal/events.h>
#include <drc/types.h>
#include <functional>
#include <netinet/in.h>
#include <string>
#include <vector>

namespace drc {

class UdpClient {
 public:
  explicit UdpClient(const std::string& dst_addr);
  virtual ~UdpClient();

  bool Start();
  void Stop();

  bool Send(const byte* data, size_t size);
  bool Send(const std::vector<byte>& msg) {
    return Send(msg.data(), msg.size());
  }

 private:
  int sock_fd_;
  std::string dst_addr_;
  sockaddr_in dst_addr_parsed_;
};

class UdpServer : public ThreadedEventMachine {
 public:
  typedef std::function<void(const std::vector<byte>&)> ReceiveCallback;
  typedef std::function<void(void)> TimeoutCallback;

  explicit UdpServer(const std::string& bind_addr);
  virtual ~UdpServer();

  bool Start();
  void Stop();

  void SetTimeout(u64 us) { timeout_us_ = us; }

  void SetReceiveCallback(ReceiveCallback cb) { recv_cb_ = cb; }
  void SetTimeoutCallback(TimeoutCallback cb) { timeout_cb_ = cb; }

 protected:
  virtual void InitEventsAndRun();

 private:
  int sock_fd_;
  u64 timeout_us_;

  std::string bind_addr_;

  ReceiveCallback recv_cb_;
  TimeoutCallback timeout_cb_;
};

}  // namespace drc
