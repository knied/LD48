#ifndef FRONTEND_PLAYER_HPP
#define FRONTEND_PLAYER_HPP

#include "state.hpp"
#include "input.hpp"

class PlayerBehavior : public Comp::Behavior::OnUpdate {
public:
  PlayerBehavior()
    : mUpKey("KeyW")
    , mDownKey("KeyS")
    , mLeftKey("KeyA")
    , mRightKey("KeyD") {
  }
  virtual void onUpdate(Entity* self, float dt) override {
    int forward = mUpKey.pressed() - mDownKey.pressed();
    int strafe = mRightKey.pressed() - mLeftKey.pressed();
    if (forward != 0 || strafe != 0) {
      auto& state = GameState::instance();
      auto& trans = self->get(state.transComp);
      auto& actor = self->get(state.actorComp);

      vec3 fw = (float)forward * vec3{actor.lookDir(0), 0.0f, actor.lookDir(1)};
      vec3 sw = (float)strafe * vec3{-actor.lookDir(1), 0.0f, actor.lookDir(0)};
      vec3 dir = mth::normal(fw + sw);
      trans.position += 2.0f * dt * dir;
      trans.position(1) = 0;
    }
  }
private:
  input::key_observer mUpKey, mDownKey, mLeftKey, mRightKey;
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
    if (mAngle(1) > -0.1f * mth::pi) {
      mAngle(1) = -0.1f * mth::pi;
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
