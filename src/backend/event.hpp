////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_EVENT_HPP
#define BACKEND_EVENT_HPP

////////////////////////////////////////////////////////////////////////////////

#include "coro.hpp"
#include <cassert>
#include <ev.h>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

namespace event {

////////////////////////////////////////////////////////////////////////////////

class scheduler {
public:
  scheduler() : _loop(::ev_default_loop(0)) {}
  void run() {
    ::ev_run(_loop, 0);
  }
  void run_once() {
    ::ev_run(_loop, EVRUN_ONCE);
  }
  template<typename return_type>
  auto run(coro::sync_task<return_type>& t) {
    t.start();
    ::ev_run(_loop, 0);
    return t.result();
  }
  struct ev_loop* loop() noexcept {
    return _loop;
  }
private:
  struct ev_loop* _loop;
};

template<typename io_operation_type>
static void event_cb(struct ev_loop* /* loop */,
                     struct ev_io* watcher,
                     int /* revents */ ) {
  auto operation = static_cast<io_operation_type*>(watcher->data);
  operation->resume();
}

template<typename impl_type, typename signature_type>
class io_operation;
template<typename impl_type, typename return_type, typename ...arg_types>
class io_operation<impl_type, return_type(arg_types...)> final {
public:
  friend void event_cb<io_operation>(struct ev_loop*, struct ev_io*, int);
  using signature_type = return_type(arg_types...);
  explicit io_operation(scheduler& s, int fd, arg_types... args)
    : _s(s), _args(std::forward<arg_types>(args)...) {
    _watcher.data = this;
    ev_io_init(&_watcher, event_cb<io_operation>, fd, impl_type::events);
  }
  ~io_operation() {
    ev_io_stop(_s.loop(), &_watcher);
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
    ev_io_start(_s.loop(), &_watcher);
  }
  auto await_resume() {
    return _result;
  }
private:
  void resume() {
    assert(_handle && !_handle.done());
    if (await_ready()) {
      ev_io_stop(_s.loop(), &_watcher);
      _handle.resume();
    }
  }
private:
  struct ev_io _watcher;
  scheduler& _s;
  return_type _result;
  std::tuple<arg_types...> _args;
  std::experimental::coroutine_handle<> _handle;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace event

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_EVENT_HPP

////////////////////////////////////////////////////////////////////////////////
