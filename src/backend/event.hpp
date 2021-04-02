////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_EVENT_HPP
#define BACKEND_EVENT_HPP

////////////////////////////////////////////////////////////////////////////////

#include "coro.hpp"
#include <cassert>
#include <ev.h>
#include <vector>
#include <queue>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

namespace event {

////////////////////////////////////////////////////////////////////////////////

template<typename awaitable_type, typename event_type>
static void event_cb(struct ev_loop* /* loop */,
                     event_type* watcher,
                     int /* revents */ ) {
  auto awaitable = static_cast<awaitable_type*>(watcher->data);
  awaitable->resume();
}

////////////////////////////////////////////////////////////////////////////////

template<typename impl_type, typename signature_type>
class io_operation;
template<typename impl_type, typename return_type, typename ...arg_types>
class io_operation<impl_type, return_type(arg_types...)> final {
public:
  friend void event_cb<io_operation, ev_io>(struct ev_loop*, struct ev_io*, int);
  using signature_type = return_type(arg_types...);
  template<typename scheduler_type>
  explicit io_operation(scheduler_type& s, int fd, arg_types... args)
    : _loop(s.loop()), _args(std::forward<arg_types>(args)...) {
    _watcher.data = this;
    ev_io_init(&_watcher, (event_cb<io_operation, ev_io>), fd, impl_type::events);
  }
  ~io_operation() {
    ev_io_stop(_loop, &_watcher);
  }
  io_operation(io_operation const&) = delete;
  io_operation& operator = (io_operation const&) = delete;
  io_operation(io_operation&&) = delete;
  io_operation& operator = (io_operation&&) = delete;

  bool await_ready() {
    _result = std::apply(impl_type::func, _args);
    return impl_type::is_ready(_result);
  }
  void await_suspend(std::experimental::coroutine_handle<> handle) {
    _handle = handle;
    assert(_handle && !_handle.done());
    ev_io_start(_loop, &_watcher);
  }
  auto await_resume() {
    _handle = nullptr;
    return _result;
  }
private:
  void resume() {
    assert(_handle && !_handle.done());
    if (await_ready()) {
      ev_io_stop(_loop, &_watcher);
      _handle.resume();
    }
  }
private:
  ev_io _watcher;
  struct ev_loop* _loop;
  return_type _result;
  std::tuple<arg_types...> _args;
  std::experimental::coroutine_handle<> _handle;
};

////////////////////////////////////////////////////////////////////////////////

class signal final {
public:
  friend void event_cb<signal, ev_signal>(struct ev_loop*, struct ev_signal*, int);
  template<typename scheduler_type>
  explicit signal(scheduler_type& s, int signum)
    : _loop(s.loop()) {
    _watcher.data = this;
    ev_signal_init(&_watcher, (event_cb<signal, ev_signal>), signum);
  }
  ~signal() {
    ev_signal_stop(_loop, &_watcher);
  }
  signal(signal const&) = delete;
  signal& operator = (signal const&) = delete;
  signal(signal&&) = delete;
  signal& operator = (signal&&) = delete;

  bool await_ready() {
    return false;
  }
  void await_suspend(std::experimental::coroutine_handle<> handle) {
    _handle = handle;
    assert(_handle && !_handle.done());
    ev_signal_start(_loop, &_watcher);
  }
  void await_resume() {
    _handle = nullptr;
  }
private:
  void resume() {
    assert(_handle && !_handle.done());
    ev_signal_stop(_loop, &_watcher);
    _handle.resume();
  }
private:
  ev_signal _watcher;
  struct ev_loop* _loop;
  std::experimental::coroutine_handle<> _handle;
};

////////////////////////////////////////////////////////////////////////////////

template<typename scheduler_type>
coro::sync_task<void> garbage_collector(scheduler_type& s) {
  while (true) {
    co_await std::experimental::suspend_always{};
    s.cleanup();
  }
}

template<typename scheduler_type>
coro::sync_task<void> signal_handler(scheduler_type& s, int signum) {
  co_await event::signal(s, signum);
  std::cout << "SIGNAL: " << signum << std::endl;
}

class scheduler {
public:
  scheduler()
    : _garbage_collector([this]() -> coro::sync_task<void> {
        while (true) {
          co_await std::experimental::suspend_always{};
          cleanup();
        }
      }())
    , _shutdown_handler([this]() -> coro::sync_task<void> {
        shutdown();
        co_return;
      }())
    , _loop(::ev_default_loop(0)) {
    _garbage_collector.start();
    _tasks.push_back(signal_handler(*this, SIGTERM));
    _tasks.back().start().then(_shutdown_handler);
  }
  void run() {
    ::ev_run(_loop, 0);
  }
  /*void run_once() {
    ::ev_run(_loop, EVRUN_ONCE);
  }*/
  struct ev_loop* loop() noexcept {
    return _loop;
  }

  void execute(coro::sync_task<void>&& task) {
    _tasks.push_back(std::move(task));
    _tasks.back().start()
      .then(_garbage_collector);
  }
  void cleanup() {
    auto done = [](coro::sync_task<void> const& task) {
      if (task.done()) {
        try {
          task.result();
        } catch (std::runtime_error const& err) {
          std::cout << "exception: " << err.what() << std::endl;
        }
      }
      return task.done();
    };
    _tasks.erase(std::remove_if(_tasks.begin(), _tasks.end(), done), _tasks.end());
  }
private:
  void shutdown() {
    std::cout << "shutdown" << std::endl;
    _tasks.clear();
  }
private:
  coro::sync_task<void> _garbage_collector;
  coro::sync_task<void> _shutdown_handler;
  std::vector<coro::sync_task<void>> _tasks;
  struct ev_loop* _loop;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace event

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_EVENT_HPP

////////////////////////////////////////////////////////////////////////////////
