#ifndef FRONTEND_ACTOR_HPP
#define FRONTEND_ACTOR_HPP

#include "geometry.hpp"
#include "map.hpp"

class ActorBehavior : public Comp::Behavior::OnUpdate {
public:
  ActorBehavior(Map* map) : mMap(map) {}
  virtual void onUpdate(Entity* self, float dt) override {
    float const radius = 0.2f;
    auto& state = GameState::instance();
    auto& trans = self->get(state.transComp);
    auto& actor = self->get(state.actorComp);
    auto& shape = self->get(state.shapeComp);
    // color & animations
    vec4 color = actor.faction == 0 ?
      vec4{0.2f, 0.2f, 0.8f, 1.0f} : vec4{0.8f, 0.2f, 0.2f, 1.0f};
    //shape.color = actor.hitAnim * vec4{1,1,1,1} + (1.0f - actor.hitAnim) * color;
    shape.color = color;
    shape.flash = actor.hitAnim;
    actor.hitAnim *= (1.0f - 2.0f * dt);
    if (actor.health <= 0) {
      actor.deathAnim *= (1.0f - 2.0f * dt);
    }

    // cooldown
    actor.cooldown -= dt;
    if (actor.cooldown < 0) actor.cooldown = 0;
    
    // collisions
    if (actor.health > 0) {
      auto move = dt * actor.move;
      auto npos = actor.pos + move;
      for (auto other : state.scene.with(state.actorComp)) {
        if (other == self) continue;
        auto& otherActor = other->get(state.actorComp);
        if (otherActor.health <= 0) continue;
        
        auto p0 = vec3{otherActor.pos(0), 0.5f, otherActor.pos(1)};
        auto p1 = vec3{npos(0), 0.5f, npos(1)};
        if (mth::length(otherActor.pos - npos) < 2.0f * radius) {
          auto normal = mth::normal(otherActor.pos - npos);
          auto dot = mth::dot(normal, move);
          if (dot > 0) {
            move = move - dot * normal;
          }
          dbg::gizmos::instance().drawLine(p0, p1, vec4{1, 0, 0, 1});
        } else {
          auto p0 = vec3{otherActor.pos(0), 0.5f, otherActor.pos(1)};
          auto p1 = vec3{npos(0), 0.5f, npos(1)};
          dbg::gizmos::instance().drawLine(p0, p1, vec4{0, 1, 0, 1});
        }
      }
      mMap->tryMove(actor.pos, move, radius);
      actor.pos += move;
    }
    trans.position = vec3{actor.pos(0), actor.deathAnim - 1.0f, actor.pos(1)};
  }

  void fire(Entity* self) {
    auto& state = GameState::instance();
    auto& actor = self->get(state.actorComp);
    if (actor.cooldown > 0.01f) return;
    actor.cooldown = 0.25f;
    Entity* bullet = state.projectiles[state.nextProjectile++];
    if (state.nextProjectile >= state.projectiles.size()) state.nextProjectile = 0;
    auto& projectile = bullet->get(state.projectileComp);
    projectile.pos = actor.pos + 0.1f * actor.lookDir;
    projectile.move = 6.0f * actor.lookDir;
    projectile.damage = 20;
    projectile.faction = actor.faction;
  }

  void melee(Entity* self) {
    float const radius = 0.2f;
    auto& state = GameState::instance();
    auto& actor = self->get(state.actorComp);
    if (actor.cooldown > 0.01f) return;
    actor.cooldown = 0.25f;
    for (auto other : state.scene.with(state.actorComp)) {
      if (other == self) continue;
      auto& otherActor = other->get(state.actorComp);
      if (otherActor.health <= 0) continue;
      if (otherActor.faction == actor.faction) continue;
      if (mth::length(otherActor.pos - actor.pos) < 3.0f * radius) {
        otherActor.health -= 10;
        otherActor.hitAnim = 1.0f;
      }
    }
  }
private:
  Map* mMap;
};

inline geometry::mesh
actorMesh() {
  geometry::mesh out;
  out.lines = false; // triangles

  float const a = 0.1f;
  float const b = 0.8f;
  vec4 color{1,1,1,1};

  vec3 norm{0,0,0};
  out.vd.push_back({ vec3{0,0,0}, norm, color }); // 0
  out.vd.push_back({ vec3{-a,b,0}, norm, color }); // 1
  out.vd.push_back({ vec3{0,a+b,0}, norm, color }); // 2
  out.vd.push_back({ vec3{a,b,0}, norm, color }); // 3
  out.vd.push_back({ vec3{0,b,-a}, norm, color }); // 4
  out.vd.push_back({ vec3{0,b,a}, norm, color }); // 5

  out.vd.push_back({ vec3{0,0,0}, norm, color }); // 0
  out.vd.push_back({ vec3{0,b,a}, norm, color }); // 5
  out.vd.push_back({ vec3{-a,b,0}, norm, color }); // 1
  out.vd.push_back({ vec3{-a,b,0}, norm, color }); // 1
  out.vd.push_back({ vec3{0,b,a}, norm, color }); // 5
  out.vd.push_back({ vec3{0,a+b,0}, norm, color }); // 2
  out.vd.push_back({ vec3{0,a+b,0}, norm, color }); // 2
  out.vd.push_back({ vec3{0,b,a}, norm, color }); // 5
  out.vd.push_back({ vec3{a,b,0}, norm, color }); // 3
  out.vd.push_back({ vec3{a,b,0}, norm, color }); // 3
  out.vd.push_back({ vec3{0,b,a}, norm, color }); // 5
  out.vd.push_back({ vec3{0,0,0}, norm, color }); // 0
  
  out.vd.push_back({ vec3{0,0,0}, norm, color }); // 0
  out.vd.push_back({ vec3{-a,b,0}, norm, color }); // 1
  out.vd.push_back({ vec3{0,b,-a}, norm, color }); // 4
  out.vd.push_back({ vec3{-a,b,0}, norm, color }); // 1
  out.vd.push_back({ vec3{0,a+b,0}, norm, color }); // 2
  out.vd.push_back({ vec3{0,b,-a}, norm, color }); // 4
  out.vd.push_back({ vec3{0,a+b,0}, norm, color }); // 2
  out.vd.push_back({ vec3{a,b,0}, norm, color }); // 3
  out.vd.push_back({ vec3{0,b,-a}, norm, color }); // 4
  out.vd.push_back({ vec3{a,b,0}, norm, color }); // 3
  out.vd.push_back({ vec3{0,0,0}, norm, color }); // 0
  out.vd.push_back({ vec3{0,b,-a}, norm, color }); // 4

  for (std::size_t i = 0; i < out.vd.size(); ++i) {
    out.id.push_back(i);
  }
  out.calculateNormals();

  return out;
}

#endif // FRONTEND_ACTOR_HPP
