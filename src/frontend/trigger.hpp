#ifndef FRONTEND_TRIGGER_HPP
#define FRONTEND_TRIGGER_HPP

#include "state.hpp"

class TriggerBehavior : public Comp::Behavior::OnUpdate {
  virtual void onUpdate(Entity* self, float /*dt*/) override {
    float const radius = 0.2f;
    auto& state = GameState::instance();
    auto& trigger = self->get(state.triggerComp);
    auto& trans = self->get(state.transComp);
    bool active = false;
    for (auto other : state.scene.with(state.actorComp)) {
      auto& otherActor = other->get(state.actorComp);
      auto& behavior = other->get(state.behaviorComp);
      if (behavior.onTriggerHandler != nullptr) {
        if (mth::length(otherActor.pos - trigger.pos) < 2.0f * radius) {
          active = true;
          behavior.onTriggerHandler->onTrigger(other, self, true);
        } else {
          active = false;
          behavior.onTriggerHandler->onTrigger(other, self, false);
        }
      }
    }
    trans.position = vec3{trigger.pos(0), active ? -0.05f : 0.0f, trigger.pos(1)};
  }
};

#endif // FRONTEND_TRIGGER_HPP
