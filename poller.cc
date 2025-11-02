#include <cassert>

#include "poller.h"

void Poller::add(int fd, std::unique_ptr<Events> && events) {
  assert(static_cast<bool>(events));
  files_.push_back({.fd = fd, .events = events->events, .revents = 0,});
  events_.emplace_back(std::move(events));
  assert(events_.size() == files_.size());
}

void Poller::poll() {
  int result = 0;
  while (running_) {
    if ( ! files_.empty()) {
      result = ppoll(files_.data(), files_.size(), &time_, nullptr);
      if (0 < result) {
        for (std::size_t index = 0; index < files_.size(); ++index) {
          if (0 != (files_[index].revents & POLLIN)) {
            events_[index]->pollin();
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
      } else {
        for (std::size_t index = 0; index < files_.size(); ++index) {
          events_[index]->timeout();
        }
      }
    }
  }
}
