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

#include <drc/types.h>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <unistd.h>

namespace drc {

struct Event {
  // Return true to keep the event, false to drop it.
  typedef std::function<bool(Event*)> CallbackType;

  Event() : fd(-1) {}
  ~Event() { if (fd != -1) close(fd); }

  int fd;
  int read_size;
  CallbackType callback;
};

struct TriggerableEvent : public Event {
  void Trigger();
};

struct TimerEvent : public Event {
  // Reset the timer to fire <nanoseconds> from now.
  void RearmTimer(u64 nanoseconds);
};

class EventMachine {
 public:
  EventMachine();
  virtual ~EventMachine();

  // The returned Event objects are owned by the EventMachine instance - do not
  // delete them manually.
  TriggerableEvent* NewTriggerableEvent(Event::CallbackType cb);
  TimerEvent* NewTimerEvent(u64 nanoseconds, Event::CallbackType cb);
  Event* NewRepeatedTimerEvent(u64 nanoseconds, Event::CallbackType cb);
  Event* NewSocketEvent(int fd, Event::CallbackType cb);

  void StartEM();
  void StopEM() { if (running_) stop_evt_->Trigger(); }
  bool EMRunning() { return running_; }

  void CancelEvent(Event* evt);

 protected:
  void ProcessEvents();
  void ClearUserEvents();  // Does not clear the stop event.
  bool running_;

 private:
  Event* NewEvent(int fd, int read_size, Event::CallbackType cb);

  int epoll_fd_;
  std::map<int, std::unique_ptr<Event>> events_;
  TriggerableEvent* stop_evt_;
};

// EventMachine that runs on a separate thread.
class ThreadedEventMachine : public EventMachine {
 public:
  void StartEM();
  void StopEM();

 protected:
  // This is called in the event machine thread. Use this to define events
  // that share stack state.
  virtual void InitEventsAndRun();

 private:
  std::thread th_;
};

}  // namespace drc
