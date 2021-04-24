#ifndef FRONTEND_PLAYER_HPP
#define FRONTEND_PLAYER_HPP

#include "state.hpp"
#include "input.hpp"
#include "actor.hpp"

class PlayerBehavior : public ActorBehavior {
public:
  PlayerBehavior(Map* map)
    : ActorBehavior(map)
    , mUpKey("KeyW")
    , mDownKey("KeyS")
    , mLeftKey("KeyA")
    , mRightKey("KeyD") {
  }
  virtual void onUpdate(Entity* self, float dt) override {
    auto& state = GameState::instance();
    auto& actor = self->get(state.actorComp);
    int forward = mUpKey.pressed() - mDownKey.pressed();
    int strafe = mRightKey.pressed() - mLeftKey.pressed();
    if (forward != 0 || strafe != 0) {
      vec2 fw = (float)forward * actor.lookDir;
      vec2 sw = (float)strafe * vec2{-actor.lookDir(1), actor.lookDir(0)};
      actor.move = 2.0f * mth::normal(fw + sw);
    } else {
      actor.move = vec2{0,0};
    }
    ActorBehavior::onUpdate(self, dt);
    if (mMouse.mousedownMain() > 0) {
      fire(self);
    }
  }
private:
  input::key_observer mUpKey, mDownKey, mLeftKey, mRightKey;
  input::mouse_observer mMouse;
};

class PlayerCameraBehavior : public Comp::Behavior::OnUpdate {
public:
  PlayerCameraBehavior() {}
  virtual void onUpdate(Entity* self, float dt) override {
    auto drot = 0.25f * dt * vec2{
      (float)mMouse.movementX(),
      (float)mMouse.movementY()
    };
    mAngle -= drot;
    while (mAngle(0) >= 2 * mth::pi) {
      mAngle(0) -= 2 * mth::pi;
    }
    if (mAngle(1) < -0.4f * mth::pi) {
      mAngle(1) = -0.4f * mth::pi;
    }
    if (mAngle(1) > -0.15f * mth::pi) {
      mAngle(1) = -0.15f * mth::pi;
    }
    
    auto& state = GameState::instance();
    auto player = state.player;
    assert(player != nullptr);
    auto const& playerTrans = player->get(state.transComp);
    auto& playerActor = player->get(state.actorComp);
    
    auto rot = mth::from_euler(mAngle(1), mAngle(0), 0.0f);
    auto baseZ = mth::transform(rot, vec3{0,0,1});
    auto pos = playerTrans.position + 3.0f * baseZ;
    auto& cameraTrans = self->get(state.transComp);
    cameraTrans.position = pos;
    cameraTrans.rotation = rot;

    playerActor.lookDir = mth::normal(vec2{-baseZ(0), -baseZ(2)});
    
    (void)dt;
  }
private:
  input::mouse_observer mMouse;
  vec2 mAngle;
};

#endif // FRONTEND_PLAYER_HPP
