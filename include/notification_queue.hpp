#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

using lock_t = std::unique_lock<std::mutex>;

class notification_queue {

  std::deque<std::function<void()>> _q;
  bool _done{false};
  std::mutex _mutex;
  std::condition_variable _ready;

public:
  bool try_pop(std::function<void()> &x) {
    lock_t lock{_mutex, std::try_to_lock};
    if (!lock || _q.empty())
      return false;
    x = std::move(_q.front());
    _q.pop_front();
    return true;
  }

  template <typename F> bool try_push(F &&f) {
    {
      lock_t lock{_mutex, std::try_to_lock};
      if (!lock)
        return false;
      _q.emplace_back(std::forward<F>(f));
    }
    _ready.notify_one();
    return true;
  }

  void done() {
    {
      lock_t lock{_mutex};
      _done = true;
    }
    _ready.notify_all();
  }

  bool pop(std::function<void()> &x) {
    lock_t lock{_mutex};
    while (_q.empty())
      _ready.wait(lock);
    if (_q.empty())
      return false;
    x = std::move(_q.front());
    _q.pop_front();
    return true;
  }

  template <typename F> void push(F &&f) {
    {
      lock_t lock{_mutex};
      _q.emplace_back(std::forward<F>(f));
    }
    _ready.notify_one();
  }
};
