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
    auto& playerActor = state.player->get(state.actorComp);
    if (mth::length(playerActor.pos - actor.pos) < aggroRange) {
      actor.move = mth::normal(playerActor.pos - actor.pos);
    } else {
      actor.move = vec2{0,0};
    }
    ActorBehavior::onUpdate(self, dt);
  }
};

#endif // FRONTEND_ALIEN_HPP
