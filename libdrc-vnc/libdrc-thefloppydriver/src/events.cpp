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

#include <array>
#include <cassert>
#include <cstring>
#include <drc/internal/events.h>
#include <drc/types.h>
#include <fcntl.h>
#include <functional>
#include <map>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <vector>

namespace drc {

void TriggerableEvent::Trigger() {
  u64 val = 1;
  if (write(fd, &val, sizeof (val)) != sizeof (val)) {
    perror("write failed - TriggerableEvent::Trigger");
  }
}

void TimerEvent::RearmTimer(u64 ns) {
  s64 seconds = ns / 1000000000;
  ns = ns % 1000000000;
  struct itimerspec its = {
    { 0, 0 },
    { seconds, (s64)ns }
  };
  timerfd_settime(fd, 0, &its, NULL);
}

EventMachine::EventMachine() : running_(false) {
  epoll_fd_ = epoll_create(64);
  stop_evt_ = NewTriggerableEvent([&](Event*) {
    running_ = false;
    ClearUserEvents();
    return true;
  });
}

EventMachine::~EventMachine() {
  close(epoll_fd_);
}

TriggerableEvent* EventMachine::NewTriggerableEvent(Event::CallbackType cb) {
  Event* evt = NewEvent(eventfd(0, EFD_NONBLOCK), sizeof (u64), cb);
  return reinterpret_cast<TriggerableEvent*>(evt);
}

TimerEvent* EventMachine::NewTimerEvent(u64 ns, Event::CallbackType cb) {
  s64 seconds = ns / 1000000000;
  ns = ns % 1000000000;
  struct itimerspec its = {
    { 0, 0 },
    { seconds, (s64)ns }
  };

  int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  timerfd_settime(fd, 0, &its, NULL);
  return reinterpret_cast<TimerEvent*>(NewEvent(fd, sizeof (u64), cb));
}

Event* EventMachine::NewRepeatedTimerEvent(u64 ns, Event::CallbackType cb) {
  s64 seconds = ns / 1000000000;
  ns = ns % 1000000000;

  struct itimerspec its = {
    { seconds, (s64)ns },
    { seconds, (s64)ns }
  };

  int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  timerfd_settime(fd, 0, &its, NULL);
  return NewEvent(fd, sizeof (u64), cb);
}

Event* EventMachine::NewSocketEvent(int fd, Event::CallbackType cb) {
  return NewEvent(fd, 0, cb);
}

void EventMachine::StartEM() {
  running_ = true;
  ProcessEvents();
}

void EventMachine::CancelEvent(Event* evt) {
  auto it = events_.find(evt->fd);
  assert(it != events_.end());
  events_.erase(it);
}

void EventMachine::ProcessEvents() {
  std::array<struct epoll_event, 64> triggered;

  while (running_) {
    int nfds = epoll_wait(epoll_fd_, triggered.data(), triggered.size(), -1);
    if (nfds == -1 && errno == EINTR) {
      continue;
    }
    assert(nfds != -1);

    for (int i = 0; i < nfds; ++i) {
      auto it = events_.find(triggered[i].data.fd);
      if (it == events_.end()) {
        // This can happen when the stop event is triggered at the same time as
        // another event. The stop event handler will clear all registered
        // events, but ProcessEvents() is not aware of that.
        continue;
      }

      Event* evt = it->second.get();
      if (evt->read_size) {
        std::vector<byte> buffer(evt->read_size);
        if (read(evt->fd, buffer.data(), evt->read_size) != evt->read_size) {
          fprintf(stderr, "read failed - EventMachine::ProcessEvents:"
                          "(fd:%d, size:%d): %s\n", evt->fd, evt->read_size,
                          strerror(errno));
        }
      }

      bool keep = evt->callback(evt);
      if (!keep) {
        events_.erase(it);
      }
    }
  }
}

void EventMachine::ClearUserEvents() {
  auto it = events_.begin();
  while (it != events_.end()) {
    if (it->second.get() == stop_evt_) {
      events_.erase(it++);
    } else {
      ++it;
    }
  }
}

Event* EventMachine::NewEvent(int fd, int read_size, Event::CallbackType cb) {
  assert(fd >= 0);

  Event* evt = new Event();
  evt->fd = fd;
  evt->read_size = read_size;
  evt->callback = cb;

  struct epoll_event epoll_evt;
  memset(&epoll_evt, 0, sizeof (epoll_evt));
  epoll_evt.events = EPOLLIN;
  epoll_evt.data.fd = fd;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &epoll_evt);

  events_[fd].reset(evt);
  return evt;
}

void ThreadedEventMachine::StartEM() {
  running_ = true;
  th_ = std::thread(&ThreadedEventMachine::InitEventsAndRun, this);
}

void ThreadedEventMachine::StopEM() {
  if (running_) {
    EventMachine::StopEM();
    th_.join();
  }
}

void ThreadedEventMachine::InitEventsAndRun() {
  // Default implementation simply starts processing events.
  ProcessEvents();
}

}  // namespace drc
