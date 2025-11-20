#include <cassert>
#include <iostream>

#include "poller.h"

using namespace std::chrono_literals;

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
          const auto before = std::chrono::steady_clock::now();
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
          {
            const auto after = std::chrono::steady_clock::now();
            const auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(after - before);
            if (0 < difference.count()) {
              std::cout << result << " " << difference << std::endl;
            }
          }
        }
      } else {
        for (std::size_t index = 0; index < files_.size(); ++index) {
          if (0ms < events_[index]->frequency_) {
            events_[index]->timeout();
          }
        }
      }
    }
  }
}
