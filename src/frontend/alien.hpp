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
      vec2 target;
      float minDist = 10000.0f;
      bool foundCrew = false;
      if (!actor.targetPlayer) {
        for (auto t : state.scene.with(state.crewComp)) {
          auto& crew = t->get(state.crewComp);
          if (crew.health > 0) {
            auto& task = t->get(state.taskComp);
            auto dist = mth::length(actor.pos - task.pos);
            if (dist < minDist) {
              minDist = dist;
              target = task.pos;
              foundCrew = true;
            }
          }
        }
      }
      auto& playerActor = state.player->get(state.actorComp);
      auto playerDist = mth::length(playerActor.pos - actor.pos);
      if (!foundCrew || playerDist < aggroRange) {
        target = playerActor.pos;
      }
      auto wayPoint = mMap->nextWaypoint(actor.pos, target);
      if (mth::length(wayPoint - actor.pos) > 0.2f) {
        actor.move = mth::normal(wayPoint - actor.pos);
      } else {
        actor.move = vec2{0,0};
      }
      melee(self);
    }
    ActorBehavior::onUpdate(self, dt);
  }
};

#endif // FRONTEND_ALIEN_HPP
