////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_ASYNC_IO_HPP
#define BACKEND_ASYNC_IO_HPP

////////////////////////////////////////////////////////////////////////////////

#include "coro.hpp"

#include <unistd.h>
#include <sys/socket.h>

#include <errno.h>
#include <tuple>

#include <iostream>

////////////////////////////////////////////////////////////////////////////////
#if 0
class IOScheduler {
public:
  IOScheduler() : mLoop(ev_default_loop(0)) {}
  void run() {
    std::cout << "a" << std::endl;
    ev_run(mLoop, 0);
    std::cout << "b" << std::endl;
  }
  struct ev_loop* loop() const { return mLoop; }
private:
  struct ev_loop *mLoop;
};

template<typename HandlerType>
void event_cb(struct ev_loop * /* loop */,
              struct ev_io *watcher,
              int /* revents */ ) {
  std::cout << "event_cb" << std::endl;
  auto handler = static_cast<HandlerType*>(watcher->data);
  handler->resume();
}

template<typename Signature>
class IOOperator;
template<class R, class...Args>
class IOOperator<R(int, Args...)> {
public:
  using Signature = R(int, Args...);
  explicit IOOperator(IOScheduler& scheduler, Signature* fn, int fd, int events)
    : mScheduler(scheduler), mFn(fn) {
    std::cout << "op " << this << std::endl;
    ev_io_init(&mWatcher, event_cb<IOOperator>, fd, events);
    mWatcher.data = this;
  }
  explicit IOOperator(IOOperator&& op)
    : mScheduler(op.mScheduler), mFn(op.mFn) {
    std::cout << "op " << this << std::endl;
    ev_io_stop(mScheduler.loop(), &op.mWatcher);
    ev_io_init(&mWatcher, event_cb<IOOperator>, op.mWatcher.fd, op.mWatcher.events);
    mWatcher.data = this;
  }
  IOOperator(IOOperator const&) = delete;
  IOOperator& operator = (IOOperator const&) = delete;
  auto& set(Args... args) {
    mArgs = std::tuple<Args...>{std::forward<Args>(args)...};
    return *this;
  }
  void resume() {
    std::cout << "op resume " << this << std::endl;
    std::cout << "op resume " << mFn << std::endl;
    if (mHandle && !mHandle.done()) {
      if (await_ready()) {
        std::cout << "stop" << std::endl;
        ev_io_stop(mScheduler.loop(), &mWatcher);
        std::cout << "resume" << std::endl;
        mHandle.resume();
        std::cout << "resumed" << std::endl;
      }
    }
  }
  bool await_ready() {
    mResult = std::apply(mFn, std::tuple_cat(std::make_tuple(mWatcher.fd), mArgs));
    if (mResult < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      std::cout << strerror(errno) << std::endl;
      return false;
    }
    return true;
  }
  void await_suspend(std::experimental::coroutine_handle<> handle) {
    std::cout << "await_suspend" << std::endl;
    mHandle = handle;
    if (mHandle && !mHandle.done()) {
      std::cout << "start" << std::endl;
      ev_io_start(mScheduler.loop(), &mWatcher);
      std::cout << "started" << std::endl;
    }
  }
  auto await_resume() {
    std::cout << "await_resume" << std::endl;
    return mResult;
  }
private:
  IOScheduler& mScheduler;
  struct ev_io mWatcher;
  std::experimental::coroutine_handle<> mHandle = nullptr;
  Signature* mFn;
  std::tuple<Args...> mArgs;
  R mResult;
};

struct IOReader final : public IOOperator<decltype(::read)> {
  IOReader(IOScheduler& scheduler, int fd)
    : IOOperator(scheduler, ::read, fd, EV_READ) {}
};
struct IOWriter final : public IOOperator<decltype(::write)> {
  IOWriter(IOScheduler& scheduler, int fd)
    : IOOperator(scheduler, ::write, fd, EV_WRITE) {}
};
struct IOAcceptor final : public IOOperator<decltype(::accept)> {
  IOAcceptor(IOScheduler& scheduler, int fd)
    : IOOperator(scheduler, ::accept, fd, EV_READ) {}
};
#endif
////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_ASYNC_IO_HPP

////////////////////////////////////////////////////////////////////////////////
