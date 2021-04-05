#ifndef COMMON_ECS_HPP
#define COMMON_ECS_HPP

#include "coro.hpp"
#include <vector>
#include <unordered_map>

namespace ecs {

class base_component {
public:
  base_component(base_component&&) = delete;
  base_component& operator = (base_component&&) = delete;
  base_component(base_component const&) = delete;
  base_component& operator = (base_component const&) = delete;
  base_component(std::uint32_t mask)
    : _mask(mask) {}
  virtual ~base_component() = default;
  virtual void remove(std::uint32_t id) = 0;
  inline std::uint32_t mask() const { return _mask; }
private:
  std::uint32_t _mask;
};

template<typename T>
class component final : public base_component {
public:
  component(component&&) = delete;
  component& operator = (component&&) = delete;
  component(component const&) = delete;
  component& operator = (component const&) = delete;
  inline T& add(std::uint32_t id) {
    auto t = _map.insert(std::make_pair(id, T{}));
    return t.first->second;
  }
  virtual void remove(std::uint32_t id) override {
    _map.erase(id);
  }
  inline T& get(std::uint32_t id) {
    return _map.at(id);
  }
  inline auto size() const {
    return _map.size();
  }
private:
  friend class scene;
  component(std::uint32_t mask)
    : base_component(mask) {}
  std::unordered_map<std::uint32_t, T> _map;
};
inline constexpr std::uint32_t cmask() { return true; }
template<typename T, typename... Ts>
inline std::uint32_t cmask(component<T> const* c, component<Ts> const* ...cs) {
  return c->mask() | cmask(cs...);
}

class entity final {
public:
  entity(entity&&) = delete;
  entity& operator = (entity&&) = delete;
  entity(entity const&) = delete;
  entity& operator = (entity const&) = delete;
  template<typename T>
  inline T& add(component<T>* c) {
    _mask = _mask | c->mask();
    return c->add(_id);
  }
  inline void remove(base_component* c) {
    _mask = _mask ^ c->mask();
    c->remove(_id);
  }
  template<typename... Ts>
  inline bool has(component<Ts>* ...cs) const {
    auto mask = cmask(cs...);
    return (_mask & mask) == mask;
  }
  template<typename T>
  inline T& get(component<T>* c) const {
    return c->get(_id);
  }
  inline bool operator == (entity const& other) const {
    return _id == other._id;
  }
  inline bool operator != (entity const& other) const {
    return _id != other._id;
  }
  inline std::uint32_t id() const { return _id; }
  inline std::uint32_t mask() const { return _mask; }
private:
  friend class scene;
  entity(std::uint32_t id) : _id(id) {}
  std::uint32_t _id;
  std::uint32_t _mask = 0;
};

class scene final {
public:
  inline entity* spawn() {
    auto id = _next_id++;
    auto e = new entity(id);
    _entities.insert(std::make_pair(id, std::unique_ptr<entity>(e)));
    return e;
  }
  inline void despawn(entity* e) {
    auto id = e->id();
    auto mask = e->mask();
    std::size_t i = 0;
    while (mask > 0) {
      if (mask & 1) {
        e->remove(_components[i].get());
      }
      i++;
      mask = mask >> 1;
    }
    _entities.erase(id);
  }

  template<typename T>
  inline component<T>* create_component() {
    auto c = new component<T>(1 << _components.size());
    _components.push_back(std::unique_ptr<base_component>(c));
    return c;
  }

  template<typename T, typename... Ts>
  inline coro::generator<entity*>
  with(component<T>* c, component<Ts>* ...cs) {
    for (auto const& item : c->_map) {
      auto e = _entities.at(item.first).get();
      if (e->has(cs...)) {
        co_yield e;
      }
    }
  }
  // TODO: pairs of entities

  inline auto size() const {
    return _entities.size();
  }
private:
  std::uint32_t _next_id = 0;
  std::unordered_map<std::uint32_t, std::unique_ptr<entity>> _entities;
  std::vector<std::unique_ptr<base_component>> _components;
};

} // ecs

#endif // COMMON_ECS_HPP
