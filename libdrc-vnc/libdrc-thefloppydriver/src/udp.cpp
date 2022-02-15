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

#include <arpa/inet.h>
#include <cstring>
#include <drc/internal/udp.h>
#include <drc/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <stdlib.h>

namespace drc {

namespace {

// Pre-swaps the port to convert it to network format.
bool ParseBindAddress(const std::string& bind_addr, in_addr* addr, u16* port) {
  size_t pos = bind_addr.find_last_of(':');
  if (pos == std::string::npos) {
    return false;
  }

  std::string ip_str = bind_addr.substr(0, pos);
  if (!inet_aton(ip_str.c_str(), addr)) {
    return false;
  }

  std::string port_str = bind_addr.substr(pos + 1);
  *port = htons(static_cast<u16>(strtol(port_str.c_str(), NULL, 10)));
  return true;
}

}  // namespace

UdpClient::UdpClient(const std::string& dst_addr)
    : sock_fd_(-1),
      dst_addr_(dst_addr) {
}

UdpClient::~UdpClient() {
  Stop();
}

bool UdpClient::Start() {
  memset(&dst_addr_parsed_, 0, sizeof (dst_addr_parsed_));
  dst_addr_parsed_.sin_family = AF_INET;
  if (!ParseBindAddress(dst_addr_, &dst_addr_parsed_.sin_addr,
                        &dst_addr_parsed_.sin_port)) {
    return false;
  }

  sock_fd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock_fd_ == -1) {
    return false;
  }

  return true;
}

void UdpClient::Stop() {
  if (sock_fd_ != -1) {
    close(sock_fd_);
    sock_fd_ = -1;
  }
}

bool UdpClient::Send(const byte* data, size_t size) {
  if (sendto(sock_fd_, data, size, 0,
             reinterpret_cast<sockaddr*>(&dst_addr_parsed_),
             sizeof (dst_addr_parsed_)) < 0) {
    return false;
  }
  return true;
}

UdpServer::UdpServer(const std::string& bind_addr)
    : sock_fd_(-1),
      timeout_us_(0),
      bind_addr_(bind_addr) {
}

UdpServer::~UdpServer() {
  Stop();
}

bool UdpServer::Start() {
  sockaddr_in sin;
  memset(&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  if (!ParseBindAddress(bind_addr_, &sin.sin_addr, &sin.sin_port)) {
    return false;
  }

  sock_fd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock_fd_ == -1) {
    return false;
  }

  fcntl(sock_fd_, F_SETFL, O_NONBLOCK);
  if (bind(sock_fd_, reinterpret_cast<sockaddr*>(&sin), sizeof (sin)) == -1) {
    Stop();
    return false;
  }

  StartEM();
  return true;
}

void UdpServer::Stop() {
  StopEM();
  if (sock_fd_ != -1) {
    close(sock_fd_);
    sock_fd_ = -1;
  }
}

void UdpServer::InitEventsAndRun() {
  u64 timeout_ns = static_cast<u64>(timeout_us_) * 1000;
  TimerEvent* timeout_evt = NewTimerEvent(timeout_ns, [&](Event*) {
    timeout_cb_();
    return true;
  });

  NewSocketEvent(sock_fd_, [&](Event*) {
    timeout_evt->RearmTimer(timeout_ns);

    sockaddr_in sender;
    socklen_t sender_len = sizeof (sender);
    int msg_max_size = 2000; //hmmmmmmmmmmmmmmmmmm
    std::vector<byte> msg(msg_max_size);

    int size = recvfrom(sock_fd_, msg.data(), msg_max_size, 0,
                        reinterpret_cast<sockaddr*>(&sender), &sender_len);
    if (size >= 0) {
      msg.resize(size);
      recv_cb_(msg);
    }
    return true;
  });

  ThreadedEventMachine::InitEventsAndRun();
}

}  // namespace drc
