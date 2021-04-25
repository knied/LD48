#ifndef FRONTEND_PLAYER_HPP
#define FRONTEND_PLAYER_HPP

#include "state.hpp"
#include "input.hpp"
#include "actor.hpp"
#include "debug.hpp"

class PlayerBehavior : public ActorBehavior
                     , public Comp::Behavior::OnTrigger {
public:
  PlayerBehavior(Map* map)
    : ActorBehavior(map)
    , mUpKey("KeyW")
    , mDownKey("KeyS")
    , mLeftKey("KeyA")
    , mRightKey("KeyD")
    , mUseKey("KeyE"){
  }
  virtual void onUpdate(Entity* self, float dt) override {
    auto& state = GameState::instance();
    auto& actor = self->get(state.actorComp);
    if (actor.health > 0) {
      int forward = mUpKey.pressed() - mDownKey.pressed();
      int strafe = mRightKey.pressed() - mLeftKey.pressed();
      if (forward != 0 || strafe != 0) {
        vec2 fw = (float)forward * actor.lookDir;
        vec2 sw = (float)strafe * vec2{-actor.lookDir(1), actor.lookDir(0)};
        actor.move = 2.0f * mth::normal(fw + sw);
      } else {
        actor.move = vec2{0,0};
      }
    }
    ActorBehavior::onUpdate(self, dt);
    if (actor.health > 0) {
      if (mMouse.mousedownMain() > 0 || mMouse.pressedMain() > 0) {
        fire(self);
      }

      // test pathing
      /*for (auto other : state.scene.with(state.triggerComp)) {
        auto& trigger = other->get(state.triggerComp);
        auto p = mMap->nextWaypoint(actor.pos, trigger.pos);
        dbg::gizmos::instance().drawLine(vec3{actor.pos(0), 0.1f, actor.pos(1)},
                                         vec3{p(0), 0.1f, p(1)}, vec4{0,0,1,1});
                                         }*/
    }
  }
  virtual void onTrigger(Entity* self, Entity* /*other*/, bool on) override {
    auto& state = GameState::instance();
    auto& instr = self->get(state.instrComp);
    if (on && state.objectivesDone) {
      instr.msg = "go to standby";
      instr.show = true;
      instr.pressE = true;
      if (mUseKey.keydown() > 0) {
        game_play_sound("standby");
        state.levelComplete = true;
      }
    } else if (on && !state.objectivesDone) {
      instr.msg = "There is more work to be done";
      instr.show = true;
      instr.pressE = false;
    } else {
      instr.show = false;
    }
  }
private:
  input::key_observer mUpKey, mDownKey, mLeftKey, mRightKey, mUseKey;
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
