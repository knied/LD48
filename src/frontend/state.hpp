#ifndef FRONTEND_GAMESTATE_HPP
#define FRONTEND_GAMESTATE_HPP

#include <common/mth.hpp>
#include <common/ecs.hpp>

using vec2 = mth::vector<float,2>;
using vec3 = mth::vector<float,3>;
using vec4 = mth::vector<float,4>;
using mat3 = mth::matrix<float,3,3>;
using mat4 = mth::matrix<float,4,4>;
using rot3 = mth::quaternion<float>;

using Scene = ecs::scene;
using Entity = ecs::entity;

class Drawable;

namespace Comp {

struct Camera {
  float znear = 0.1f;
  float zfar = 1000.0f;
  float fov = 70.0f;
};

struct Shape {
  vec4 color = vec4{1,1,1,1};
  float flash = 0.0f;
  Drawable const* drawable = nullptr;
  bool visible = true;
};

struct Physical {
  enum Type { Static, Collider };
  Type type = Static;
  int contacts = 0;
  float radius = 1.0f; // FIXME: replace with arbitrary convex
};

struct Transformation {
  vec3 position = vec3{0,0,0};
  rot3 rotation = mth::from_euler(0.0f, 0.0f, 0.0f);
  Entity* parent = nullptr;
  mat4 local() const {
    return mth::transformation(rotation, position);
  }
};

struct Behavior {
  struct OnUpdate {
    virtual void onUpdate(Entity* self, float dt) = 0;
  };
  struct OnTrigger {
    virtual void onTrigger(Entity* self, Entity* other, bool on) = 0;
  };
  struct OnContact {
    virtual void onContact(Entity* self, Entity* other) = 0;
  };
  OnUpdate* onUpdateHandler = nullptr;
  OnTrigger* onTriggerHandler = nullptr;
  OnContact* onContactHandler = nullptr;
};

struct Actor {
  int faction = 0;
  int health = 100;
  vec2 lookDir = vec2{1,0};
  vec2 move = vec2{0,0};
  vec2 pos = vec2{0,0};
  float hitAnim = 0.0f;
  float deathAnim = 1.0f;
  float cooldown = 0.0f;
};

struct Projectile {
  int faction = 0;
  int damage = 0;
  vec2 move = vec2{0,0};
  vec2 pos = vec2{0,0};
};

struct Trigger {
  int id = 0;
  vec2 pos = vec2{0,0};
};

} // namespace Comp

using CameraComponent = ecs::component<Comp::Camera>;
using ShapeComponent = ecs::component<Comp::Shape>;
using PhysicalComponent = ecs::component<Comp::Physical>;
using TransformationComponent = ecs::component<Comp::Transformation>;
using BehaviorComponent = ecs::component<Comp::Behavior>;
using ActorComponent = ecs::component<Comp::Actor>;
using ProjectileComponent = ecs::component<Comp::Projectile>;
using TriggerComponent = ecs::component<Comp::Trigger>;

struct GameState {
  ecs::scene scene;
  CameraComponent* cameraComp;
  ShapeComponent* shapeComp;
  PhysicalComponent* physicalComp;
  TransformationComponent* transComp;
  BehaviorComponent* behaviorComp;
  ActorComponent* actorComp;
  ProjectileComponent* projectileComp;
  TriggerComponent* triggerComp;
  Entity* player = nullptr;
  Entity* camera = nullptr;
  std::vector<Entity*> projectiles;
  std::size_t nextProjectile = 0;
  bool playerOnCharger = false;

  static GameState& instance() {
    static GameState i;
    return i;
  }

  void reset() {
    player = nullptr;
    camera = nullptr;
    projectiles.clear();
    nextProjectile = 0;
    playerOnCharger = false;
    scene = ecs::scene();
    cameraComp = scene.create_component<Comp::Camera>();
    shapeComp = scene.create_component<Comp::Shape>();
    physicalComp = scene.create_component<Comp::Physical>();
    transComp = scene.create_component<Comp::Transformation>();
    behaviorComp = scene.create_component<Comp::Behavior>();
    actorComp = scene.create_component<Comp::Actor>();
    projectileComp = scene.create_component<Comp::Projectile>();
    triggerComp = scene.create_component<Comp::Trigger>();
  }
  
private:
  GameState() {
    reset();
  } 
};

static mat4
world(Entity* e, TransformationComponent* transComp) {
  mat4 result = mth::identity<float,4,4>();
  while (e != nullptr) {
    if (e->has(transComp)) {
      auto& trans = e->get(transComp);
      result = trans.local() * result;
      e = trans.parent;
    } else {
      e = nullptr;
    }
  }
  return result;
}

#endif // FRONTEND_GAMESTATE_HPP
