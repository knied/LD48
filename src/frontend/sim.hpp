#ifndef FRONTEND_SIM_HPP
#define FRONTEND_SIM_HPP

#include "state.hpp"
#include "input.hpp"
#include "debug.hpp"

#include <common/gjk.hpp>

class Sim final {
public:
  Sim(GameState* state)
    : mState(state)
    , mLogKey("KeyL") {}
  void update(float dt) {
    if (mLogKey.keydown() > 0) {
      gjk::log = true;
    }
    (void)dt;

    auto mask = ecs::fingerprint(mState->transComp, mState->physicalComp);
    for (auto [e0, e1] : mState->scene.pairs_with(mask, mask)) {
      auto& physical0 = e0->get(mState->physicalComp);
      auto& physical1 = e1->get(mState->physicalComp);
      if (physical0.type == Comp::Physical::Static &&
          physical1.type == Comp::Physical::Static) {
        continue;
      }
      auto m0 = world(e0, mState->transComp);
      auto m1 = world(e1, mState->transComp);
      gjk::sphere c0{ physical0.radius };
      gjk::sphere c1{ physical1.radius };
      gjk::transformed_convex c1_(mth::inverse_transformation(m1) * m0, &c1);
      gjk::simplex simplex;
      if (gjk::gjk(c0, c1_, simplex)) {
        physical0.contacts++;
        physical1.contacts++;
      } else {
        vec3 p0, p1;
        gjk::closest(simplex, p0, p1);
        auto& gizmos = dbg::gizmos::instance();
        gizmos.drawLine(mth::transform_point(m0, p0),
                        mth::transform_point(m0, p1),
                        vec4{1, 1, 0, 1});
      }
    }

    for (auto e : mState->scene.with(mState->physicalComp, mState->shapeComp)) {
      auto& physical = e->get(mState->physicalComp);
      auto& shape = e->get(mState->shapeComp);
      if (physical.contacts > 0) {
        shape.color = vec4{1,0,0,1};
      } else {
        shape.color = vec4{1,1,1,1};
      }
      physical.contacts = 0;
    }
    
    gjk::log = false;
  }

private:
  GameState* mState;
  input::key_observer mLogKey;
};

#endif // FRONTEND_SIM_HPP
