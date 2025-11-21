#include <iostream>
#include <thread>

#include <cassert>

#include <signal.h>

#include "poller.h"

using namespace std::chrono_literals;

void Poller::add(int fd, std::unique_ptr<Events> && events) {
  assert(static_cast<bool>(events));
  files_.push_back({.fd = fd, .events = events->events, .revents = 0,});
  if (0ms < events->frequency) {
    events->next = std::chrono::steady_clock::now() + events->frequency;
    events->timeout();
  }
  events_.emplace_back(std::move(events));
  assert(events_.size() == files_.size());
}

void Poller::poll() {
  sigset_t sigmask;
  sigemptyset(&sigmask);
#if 0
  sigaddset(&sigmask, SIGTTIN);
  {
    struct sigaction sighandler;
    sighandler.sa_flags = 0;
    sighandler.sa_handler = [](int){
      std::cerr << "sighandler" << std::endl;
    };
    sigemptyset(&sighandler.sa_mask);
    sigaction(SIGTTIN, &sighandler, nullptr);
  }
#endif
  while (running_) {
    const int result = ppoll(files_.data(), files_.size(), &time_, &sigmask);
    bool done;
    do {
      done = true;
      const auto now = std::chrono::steady_clock::now();
      std::optional<TimePoint> next;
      for (std::size_t index = 0; index < events_.size(); ++index) {
        auto & task = *events_[index];
        if (0ms < task.frequency) {
          if (now >= task.next) {
            task.next = std::chrono::steady_clock::now() + task.frequency;
            task.timeout();
          }
          if ( ! next.has_value() || next.value() > task.next) {
            next = task.next;
          }
        }
      }
      if (0 < result) {
        for (std::size_t index = 0; index < files_.size(); ++index) {
          if (0 != (files_[index].revents & POLLIN)) {
            done &= events_[index]->pollin(next);
          }
          if (0 != (files_[index].revents & POLLOUT)) {
            events_[index]->pollout();
          }
          if (0 != (files_[index].revents & POLLERR)) {
            events_[index]->pollerr();
          }
          if (0 != (files_[index].revents & POLLHUP)) {
            events_[index]->pollhup();
          }
        }
      }
    } while ( ! done);
  }
}
