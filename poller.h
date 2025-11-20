#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

/* restrict constructor to unique_ptr only */
struct Events {
  using Frequency = std::chrono::milliseconds;
  virtual ~Events() { }
  Events(const short e) : events(e) { }
  Events(const Frequency & f) : frequency_(f) { }
  virtual void pollin() { }
  virtual void pollout() { }
  virtual void pollerr() { }
  virtual void pollhup() { }
  virtual void timeout() { }
  const short events = 0;
  const Frequency frequency_{0};
};

struct Poller {
  ~Poller() = default;
  Poller(const std::chrono::nanoseconds & timeout) :
    time_{.tv_sec = 0, .tv_nsec = timeout.count(), } { }

  template<class T>
  T & add(int fd, std::unique_ptr<T> && t) {
    T & result = *t;
    add(fd, std::unique_ptr<Events>(std::move(t)));
    return result;
  }

  void add(int fd, std::unique_ptr<Events> && events);
  void off() { running_ = false; }
  void on() { running_ = true; }
  void poll();

private:
  std::vector<struct pollfd> files_ = {};
  std::vector<std::unique_ptr<Events>> events_ = {};
  const struct timespec time_;
  std::atomic_bool running_ = false;
};
