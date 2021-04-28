#ifndef FRONTEND_PROJECTILE_HPP
#define FRONTEND_PROJECTILE_HPP

#include "state.hpp"
#include "map.hpp"

class ProjectileBehavior : public Comp::Behavior::OnUpdate {
public:
  ProjectileBehavior(Map* map) : mMap(map) {}
  virtual void onUpdate(Entity* self, float dt) {
    float const radius = 0.05f;
    auto& state = GameState::instance();
    auto& shape = self->get(state.shapeComp);
    auto& projectile = self->get(state.projectileComp);
    if (projectile.damage > 0) {
      shape.visible = true;
      auto& trans = self->get(state.transComp);
      auto move = dt * projectile.move;
      if (mMap->tryMove(projectile.pos, move, radius)) {
        // we hit a wall
        projectile.damage = 0;
      } else {
        projectile.pos += move;
        trans.position = vec3{projectile.pos(0), 0.5f, projectile.pos(1)};
        for (auto other : state.scene.with(state.actorComp)) {
          auto& otherActor = other->get(state.actorComp);
          if (otherActor.health <= 0) continue;
          if (otherActor.faction == projectile.faction) continue;
          if (mth::length(otherActor.pos - projectile.pos) < (0.1f + radius)) {
            // we hit an actor
            otherActor.health -= projectile.damage;
            projectile.damage = 0;
            otherActor.hitAnim = 1.0f;
            if (otherActor.health <= 0) {
              game_play_sound("crew_dead");
            } else {
              game_play_sound("hit");
            }
          }
        }
      }
    } else {
      shape.visible = false;
    } 
  }
private:
  Map* mMap;
};

#endif // FRONTEND_PROJECTILE_HPP
