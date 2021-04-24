#ifndef FRONTEND_ALIEN_HPP
#define FRONTEND_ALIEN_HPP

#include "state.hpp"
#include "actor.hpp"

class AlienBehavior : public ActorBehavior {
public:
  AlienBehavior(Map* map)
    : ActorBehavior(map) {

  }
  virtual void onUpdate(Entity* self, float dt) override {
    float const aggroRange = 3.0f;
    
    auto& state = GameState::instance();
    auto& actor = self->get(state.actorComp);
    if (actor.health > 0) {
      auto& playerActor = state.player->get(state.actorComp);
      auto dist = mth::length(playerActor.pos - actor.pos);
      if (dist < aggroRange && dist > 0.2f) {
        actor.move = mth::normal(playerActor.pos - actor.pos);
      } else {
        actor.move = vec2{0,0};
      }
      melee(self);
    }
    ActorBehavior::onUpdate(self, dt);
  }
};

#endif // FRONTEND_ALIEN_HPP
