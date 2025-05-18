#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

/* restrict constructor to unique_ptr only */
struct Events {
  virtual ~Events() { }
  Events(const short e) : events(e) { }

  virtual void pollin(/*...*/) { }
  virtual void pollout(/*...*/) { }
  virtual void pollerr(/*...*/) { }
  virtual void pollhup(/*...*/) { }
  /* maybe others */

  const short events = 0;
};

struct Poller {
  ~Poller() { }
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
  struct timespec time_{.tv_sec = 60, .tv_nsec = 0,};
  std::atomic_bool running_ = false;
};
