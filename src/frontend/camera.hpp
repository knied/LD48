#ifndef FRONTEND_CAMERA_HPP
#define FRONTEND_CAMERA_HPP

#include "state.hpp"
#include "input.hpp"

class DebugCamera final {
public:
  DebugCamera(GameState* state)
    : mState(state)
    , mAngle{0,0}
    , mPos{0,0}
      //, mModel(mth::identity<float,4,4>())
    , mUpKey("KeyW")
    , mDownKey("KeyS")
    , mLeftKey("KeyA")
    , mRightKey("KeyD") {
    mEntity = mState->scene.spawn();
    mEntity->add(mState->cameraComp);
    mEntity->add(mState->transComp);
  }
  ~DebugCamera() {
    mState->scene.despawn(mEntity);
  }
  void update(float dt) {
    auto drot = 0.25f * dt * vec2{
      (float)mMouse.movementX(),
      (float)mMouse.movementY()
    };
    mAngle -= drot;
    while (mAngle(0) >= 2 * mth::pi) {
      mAngle(0) -= 2 * mth::pi;
    }
    if (mAngle(1) < -0.5f * mth::pi) {
      mAngle(1) = -0.5f * mth::pi;
    }
    if (mAngle(1) > 0.5f * mth::pi) {
      mAngle(1) = 0.5f * mth::pi;
    }
    auto& trans = mEntity->get(mState->transComp);
    trans.rotation = mth::from_euler(mAngle(1), mAngle(0), 0.0f);
    
    //mModel = mth::rotation();
    int frontBack = mUpKey.pressed() - mDownKey.pressed();
    int leftRight = mRightKey.pressed() - mLeftKey.pressed();
    if (frontBack != 0 || leftRight != 0) {
      auto dmove = 4.0f * dt * mth::normal(vec2{
          (float)leftRight,
          (float)frontBack
        });
      //mPos += (mth::baseX(mModel) * dmove(0) - mth::baseZ(mModel) * dmove(1));
      auto baseX = mth::transform(trans.rotation, vec3{1,0,0});
      auto baseZ = mth::transform(trans.rotation, vec3{0,0,1});
      mPos += (baseX * dmove(0) - baseZ * dmove(1));
    }
    trans.position = mPos;
    //mModel = mth::translation(mPos) * mModel;
  }
  Entity* entity() const {
    return mEntity;
  }
  /*auto model() const {
    return mModel;
    }*/
private:
  GameState* mState;
  Entity* mEntity;
  vec2 mAngle;
  vec3 mPos;
  //mat4 mModel;
  input::key_observer mUpKey, mDownKey, mLeftKey, mRightKey;
  input::mouse_observer mMouse;
};

#endif // FRONTEND_CAMERA_HPP 
