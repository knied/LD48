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
    auto move = dt * actor.move;
    mMap->tryMove(actor.pos, move, radius);
    auto npos = actor.pos + move;
    for (auto other : state.scene.with(state.actorComp)) {
      if (other == self) continue;

      auto& otherActor = other->get(state.actorComp);
      
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
    actor.pos += move;
    trans.position = vec3{actor.pos(0), 0.0f, actor.pos(1)};
    
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

  out.vd.push_back({ vec3{0,0,0}, color }); // 0
  out.vd.push_back({ vec3{-a,b,0}, color }); // 1
  out.vd.push_back({ vec3{0,a+b,0}, color }); // 2
  out.vd.push_back({ vec3{a,b,0}, color }); // 3
  out.vd.push_back({ vec3{0,b,-a}, color }); // 4
  out.vd.push_back({ vec3{0,b,a}, color }); // 5

  out.id.push_back(0);
  out.id.push_back(4);
  out.id.push_back(1);
  out.id.push_back(1);
  out.id.push_back(4);
  out.id.push_back(2);
  out.id.push_back(2);
  out.id.push_back(4);
  out.id.push_back(3);
  out.id.push_back(3);
  out.id.push_back(4);
  out.id.push_back(0);

  out.id.push_back(0);
  out.id.push_back(1);
  out.id.push_back(5);
  out.id.push_back(1);
  out.id.push_back(2);
  out.id.push_back(5);
  out.id.push_back(2);
  out.id.push_back(3);
  out.id.push_back(5);
  out.id.push_back(3);
  out.id.push_back(0);
  out.id.push_back(5);

  return out;
}

#endif // FRONTEND_ACTOR_HPP
