#pragma once

#include <array>
#include <csignal>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>

namespace tinydhcpd {
extern volatile std::sig_atomic_t last_signal; // defined in daemon.cpp

template <class T> class Epoll {
  static constexpr size_t MAX_EVENTS = 1;

  T &subject;
  int epoll_fd;
  struct epoll_event epoll_ctl_cfg;
  std::array<struct epoll_event, MAX_EVENTS> events;

public:
  Epoll(T &subject, uint32_t events)
      : subject(subject), epoll_ctl_cfg{.events = events, .data = {}},
        events() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
      throw std::runtime_error("Failed to create epoll structure!");
    }
    epoll_ctl_cfg.data.fd = subject;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, subject, &epoll_ctl_cfg) == -1) {
      throw std::runtime_error("Failed at epoll_ctl!");
    }
  }

  Epoll(Epoll<T> &&other) : Epoll() { swap(*this, other); }

  Epoll<T> &operator=(Epoll<T> other) {
    swap(*this, other);
    return *this;
  }

  ~Epoll() noexcept { close(epoll_fd); }

  // forbid copy construction and assignment as exactly one epoll instance
  // should be wrapping a given subject at any time
  Epoll(const Epoll<T> &other) = delete;
  Epoll<T> &operator=(const Epoll<T> &other) = delete;

  friend void swap(Epoll<T> &first, Epoll<T> &second) noexcept {
    using std::swap;
    swap(first.subject, second.subject);
    swap(first.epoll_fd, second.epoll_fd);
    swap(first.epoll_ctl_cfg, second.epoll_ctl_cfg);
    swap(first.events, second.events);
  }

  void poll_loop() {
    while (true) {
      if (tinydhcpd::last_signal == SIGINT ||
          tinydhcpd::last_signal == SIGTERM) {
        return;
      }

      if (epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1) == -1) {
        if (errno == EINTR) {
          continue;
        }
        throw std::runtime_error("epoll_wait failed");
      }
      if ((events[0].events & EPOLLIN) > 0) {
        subject.handle_epollin();
      }
      if ((events[0].events & EPOLLOUT) > 0) {
        subject.handle_epollout();
      }
    }
  }
};
} // namespace tinydhcpd
