////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_CORO_HPP
#define BACKEND_CORO_HPP

////////////////////////////////////////////////////////////////////////////////

#include <experimental/coroutine>
#include <optional>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////

namespace coro {

////////////////////////////////////////////////////////////////////////////////

struct final_awaitable final {
  constexpr bool await_ready() const noexcept {
    return false;
  }
  template<typename promise_type>
  constexpr std::experimental::coroutine_handle<>
  await_suspend(std::experimental::coroutine_handle<promise_type> handle) const noexcept {
    auto c = handle.promise().continuation();
    return c ? c : std::experimental::noop_coroutine();
  }
  constexpr void await_resume() const noexcept {}
};

template<typename task_type, typename return_type>
class task_promise final {
  enum class state { running, done, exception };
public:
  task_promise() {}
  task_promise(task_promise const&) = delete;
  task_promise& operator = (task_promise const&) = delete;
  task_promise(task_promise&&) = delete;
  task_promise& operator = (task_promise&&) = delete;
  ~task_promise() {
    switch (_state) {
    case state::done: _return_value.~return_type(); break;
    case state::exception: _exception.~exception_ptr(); break;
    default: break;
    }
  }
  task_type get_return_object() {
    using handle_type = std::experimental::coroutine_handle<task_promise>;
    return task_type(handle_type::from_promise(*this));
  }
  void unhandled_exception() {
    ::new (static_cast<void*>(std::addressof(_exception)))
      std::exception_ptr(std::current_exception());
    _state = state::exception;
  }
  template<typename type>
  void return_value(type&& value) {
    ::new (static_cast<void*>(std::addressof(_return_value)))
      return_type(std::forward<type>(value));
    _state = state::done;
  }
  return_type& result() & {
    if (_state == state::exception) {
      std::rethrow_exception(_exception);
    }
    return _return_value;
  }
  return_type result() && {
    if (_state == state::exception) {
      std::rethrow_exception(_exception);
    }
    return std::move(_return_value);
  }
  auto initial_suspend() noexcept {
    return std::experimental::suspend_always{};
  }
  auto final_suspend() noexcept {
    return final_awaitable{};
  }
  void set_continuation(std::experimental::coroutine_handle<> handle) noexcept {
    _continuation = handle;
  }
  std::experimental::coroutine_handle<> continuation() const {
    return _continuation;
  }
private:
  std::experimental::coroutine_handle<> _continuation = nullptr;
  state _state = state::running;
  union {
    return_type _return_value;
    std::exception_ptr _exception;
  };
};

template<typename task_type>
class task_promise<task_type, void> final {
public:
  task_promise() {}
  task_promise(task_promise const&) = delete;
  task_promise& operator = (task_promise const&) = delete;
  task_promise(task_promise&&) = delete;
  task_promise& operator = (task_promise&&) = delete;
  task_type get_return_object() {
    using handle_type = std::experimental::coroutine_handle<task_promise>;
    return task_type(handle_type::from_promise(*this));
  }
  void unhandled_exception() {
    _exception = std::current_exception();
  }
  void return_void() {}
  void result() {
    if (_exception) {
      std::rethrow_exception(_exception);
    }
  }
  auto initial_suspend() noexcept {
    return std::experimental::suspend_always{};
  }
  auto final_suspend() noexcept {
    return final_awaitable{};
  }
  void set_continuation(std::experimental::coroutine_handle<> handle) noexcept {
    _continuation = handle;
  }
  std::experimental::coroutine_handle<> continuation() const {
    return _continuation;
  }
private:
  std::experimental::coroutine_handle<> _continuation = nullptr;
  std::exception_ptr _exception = nullptr;
};

template<typename return_type>
class sync_task final {
public:
  using promise_type = task_promise<sync_task, return_type>;
  using handle_type = std::experimental::coroutine_handle<promise_type>;
  explicit sync_task(handle_type&& handle) noexcept
    : _handle(std::move(handle)) {}
  sync_task(sync_task&& other) noexcept
    : _handle(std::move(other._handle)) {
    other._handle = nullptr;
  }
  sync_task& operator = (sync_task&& other) noexcept {
    if (std::addressof(other) != this) {
      std::swap(other._handle, _handle);
    }
    return *this;
  }
  ~sync_task() {
    if (_handle) {
      _handle.destroy();
    }
  }
  sync_task(sync_task const&) = delete;
  sync_task& operator = (sync_task const&) = delete;
  sync_task& start() noexcept {
    if (_handle && !_handle.done()) {
      _handle.resume();
    }
    return *this;
  }
  template<typename then_return_type>
  auto& then(sync_task<then_return_type>& task) noexcept {
    if (_handle && !_handle.done() && !task.done()) {
      _handle.promise().set_continuation(task.handle());
    }
    return task;
  }
  constexpr bool done() const noexcept {
    return _handle && _handle.done();
  }
  constexpr return_type result() const {
    return _handle.promise().result();
  }
  std::experimental::coroutine_handle<> handle() const {
    return _handle;
  }
private:
  std::experimental::coroutine_handle<promise_type> _handle;
};

template<typename return_type>
class async_task final {
public:
  using promise_type = task_promise<async_task, return_type>;
  using handle_type = std::experimental::coroutine_handle<promise_type>;
  explicit async_task(handle_type&& handle) noexcept
    : _handle(std::move(handle)) {}
  async_task(async_task&& other) noexcept
    : _handle(std::move(other._handle)) {
    other._handle = nullptr;
  }
  async_task& operator = (async_task&& other) noexcept {
    if (std::addressof(other) != this) {
      std::swap(other._handle, _handle);
    }
    return *this;
  }
  ~async_task() {
    if (_handle) {
      _handle.destroy();
    }
  }
  async_task(async_task const&) = delete;
  async_task& operator = (async_task const&) = delete;
  constexpr bool done() const noexcept {
    return _handle && _handle.done();
  }

  auto operator co_await () const & noexcept {
    struct awaitable {
      std::experimental::coroutine_handle<promise_type> _handle;
      constexpr bool await_ready() noexcept { return false; }
      auto await_suspend(std::experimental::coroutine_handle<> handle) {
        _handle.promise().set_continuation(handle);
        return _handle;
      }
      decltype(auto) await_resume() {
        return _handle.promise().result();
      }
    };
    return awaitable{ _handle };
  }
  auto operator co_await () const && noexcept {
    struct awaitable {
      std::experimental::coroutine_handle<promise_type> _handle;
      constexpr bool await_ready() noexcept { return false; }
      auto await_suspend(std::experimental::coroutine_handle<> handle) {
        _handle.promise().set_continuation(handle);
        return _handle;
      }
      decltype(auto) await_resume() {
        return std::move(_handle.promise()).result();
      }
    };
    return awaitable{ _handle };
  }
private:
  handle_type _handle;
};

template<typename return_type>
using task = async_task<return_type>;

template<typename T>
struct generator {
  struct promise_type {
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    generator<T> get_return_object() {
      return generator<T>(std::experimental::coroutine_handle<promise_type>::from_promise(*this));
    }
    auto yield_value(T const& value) {
      _current = &value;
      return std::experimental::suspend_always{};
    }
    void return_void() {}
    void unhandled_exception() {
      _exception = std::current_exception();
    }
    void rethrow_exception() {
      if (_exception) {
        std::rethrow_exception(_exception);
      }
    }
    
    T const* _current;
    std::exception_ptr _exception = nullptr;
  };
  
  struct iterator {
    explicit iterator(std::experimental::coroutine_handle<promise_type> coroutine)
      : _coroutine(coroutine) {}
    iterator() = default;
    
    bool operator != (iterator const& other) const {
      return _coroutine != other._coroutine;
    }
    bool operator == (iterator const& other) const {
      return _coroutine == other._coroutine;
    }
    iterator& operator ++ () {
      assert(_coroutine && "Incrementing invalid iterator");
      _coroutine.resume();
      if (_coroutine.done()) {
        auto tmp = _coroutine;
        _coroutine = nullptr;
        tmp.promise().rethrow_exception();
      }
      return *this;
    }
    T const& operator * () const {
      return *_coroutine.promise()._current;
    }
    T const* operator -> () const {
      return _coroutine.promise()._current;
    }
      
    std::experimental::coroutine_handle<promise_type> _coroutine = nullptr;
  };
  
  explicit generator(std::experimental::coroutine_handle<promise_type> coroutine)
    : _coroutine(coroutine) {}
  ~generator() {
    if (_coroutine) {
      _coroutine.destroy();
    }
  }
  
  generator() = default;
  generator(generator const&) = delete;
  generator const& operator = (generator const&) = delete;
  generator(generator&& other)
    : _coroutine(other._coroutine) {
    other._coroutine = nullptr;
  }
  generator& operator = (generator&& other) {
    if (&other != this) {
      _coroutine = other._coroutine;
      other._coroutine = nullptr;
    }
  }
  
  iterator begin() {
    if (_coroutine) {
      _coroutine.resume();
      if (_coroutine.done()) {
        _coroutine.promise().rethrow_exception();
        return end();
      }
    }
    return iterator(_coroutine);
  }
  iterator end() {
    return iterator();
  }
  
  std::experimental::coroutine_handle<promise_type> _coroutine = nullptr;
};

template<typename T>
struct async_generator {
  struct promise_type {
    auto initial_suspend() {
      return std::experimental::suspend_always{};
    }
    auto final_suspend() noexcept {
      return final_awaitable();
    }
    async_generator<T> get_return_object() {
      return async_generator<T>(std::experimental::coroutine_handle<promise_type>::from_promise(*this));
    }
    auto yield_value(T const& value) {
      _current = value;
      return final_awaitable();
    }
    auto yield_value(T&& value) {
      _current = std::move(value);
      return final_awaitable();
    }
    void return_void() {}
    void unhandled_exception() {
      _exception = std::current_exception();
    }
    void rethrow_exception() {
      if (_exception) {
        std::rethrow_exception(_exception);
      }
    }
    std::experimental::coroutine_handle<> continuation() const {
      return _continuation;
    }

    std::experimental::coroutine_handle<> _continuation = nullptr;
    std::optional<T> _current;
    std::exception_ptr _exception = nullptr;
  };
  
  struct iterator {
    constexpr bool await_ready() noexcept {
      if (_coroutine.promise()._current.has_value()) {
        return true;
      }
      return false;
    }
    auto await_suspend(std::experimental::coroutine_handle<> handle) {
      _coroutine.promise()._continuation = handle;
      return _coroutine;
    }
    decltype(auto) await_resume() {
      if (_coroutine.done()) {
        auto tmp = _coroutine;
        _coroutine = nullptr;
        tmp.promise().rethrow_exception();
      }
      return *this;
    }

    explicit iterator(std::experimental::coroutine_handle<promise_type> coroutine)
      : _coroutine(coroutine) {}
    iterator() = default;
    
    bool operator != (iterator const& other) const {
      return _coroutine != other._coroutine;
    }
    bool operator == (iterator const& other) const {
      return _coroutine == other._coroutine;
    }
    iterator& operator ++ () {
      if (_coroutine) {
        _coroutine.promise()._current.reset();
      }
      return *this;
    }
    T operator * () {
      return std::move(_coroutine.promise()._current.value());
    }
      
    std::experimental::coroutine_handle<promise_type> _coroutine = nullptr;
  };
  
  explicit async_generator(std::experimental::coroutine_handle<promise_type> coroutine)
    : _coroutine(coroutine) {}
  ~async_generator() {
    if (_coroutine) {
      _coroutine.destroy();
    }
  }
  
  async_generator() = default;
  async_generator(async_generator const&) = delete;
  async_generator const& operator = (async_generator const&) = delete;
  async_generator(async_generator&& other)
    : _coroutine(other._coroutine) {
    other._coroutine = nullptr;
  }
  async_generator& operator = (async_generator&& other) {
    if (&other != this) {
      _coroutine = other._coroutine;
      other._coroutine = nullptr;
    }
  }

  iterator begin() {
    if (_coroutine && !_coroutine.done()) {
      return iterator(_coroutine);
    }
    return end();
  }
  iterator end() {
    return iterator();
  }
  
  std::experimental::coroutine_handle<promise_type> _coroutine = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace coro

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_CORO_HPP

////////////////////////////////////////////////////////////////////////////////
