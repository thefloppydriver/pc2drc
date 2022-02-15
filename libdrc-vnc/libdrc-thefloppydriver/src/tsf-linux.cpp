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

#include <drc/types.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>

namespace drc {

namespace {

int GetInterfaceOfIpv4(const std::string& addr, std::string* out) {
  struct ifaddrs* ifa = NULL;
  char ip_buffer[16];
  int rv = EAI_FAIL;

  if (getifaddrs(&ifa) == -1 || ifa == NULL) {
    return rv;
  }

  struct ifaddrs* p = ifa;
  while (p) {
    if ((p->ifa_flags & IFF_RUNNING) && (p->ifa_addr != NULL)) {
      if (p->ifa_addr->sa_family == AF_INET) {
        rv = getnameinfo(p->ifa_addr, sizeof (struct sockaddr_in), ip_buffer,
                         sizeof (ip_buffer), NULL, 0, NI_NUMERICHOST);
        if ((rv == 0) && (addr == ip_buffer)) {
          break;
        }
      }
    }
    p = p->ifa_next;
  }

  if (p) {
    *out = p->ifa_name;
  } else {
    rv = EAI_FAIL;
  }

  freeifaddrs(ifa);
  return rv;
}

}  // namespace

int GetTsf(u64 *tsf) {
  static int fd = -1;

  if (fd == -1) {
    std::string drc_if;

    if (GetInterfaceOfIpv4("192.168.1.10", &drc_if) == 0) {
      std::string tsf_path = std::string("/sys/class/net/") + drc_if +
                             "/device/tsf";
      fd = open(tsf_path.c_str(), O_RDONLY);
      if (fd == -1) {
        tsf_path = std::string("/sys/class/net/") + drc_if + "/tsf";
        fd = open(tsf_path.c_str(), O_RDONLY);
      }
    }

    if (fd == -1) {
      return -1;
    }
  }
  if (pread(fd, tsf, sizeof (*tsf), 0) != sizeof (*tsf)) {
    perror("pread failed - GetTsf");
  }
  return 0;
}

}  // namespace drc
