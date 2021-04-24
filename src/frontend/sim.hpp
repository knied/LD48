#ifndef FRONTEND_SIM_HPP
#define FRONTEND_SIM_HPP

#include "state.hpp"
#include "debug.hpp"

#include <common/gjk.hpp>

class Sim final {
public:
  Sim() {}
  void update(float /*dt*/) {
    auto& state = GameState::instance();
    auto mask = ecs::fingerprint(state.transComp, state.physicalComp);
    for (auto [e0, e1] : state.scene.pairs_with(mask, mask)) {
      auto& physical0 = e0->get(state.physicalComp);
      auto& physical1 = e1->get(state.physicalComp);
      if (physical0.type == Comp::Physical::Static &&
          physical1.type == Comp::Physical::Static) {
        continue;
      }
      auto m0 = world(e0, state.transComp);
      auto m1 = world(e1, state.transComp);
      gjk::sphere c0{ physical0.radius };
      gjk::sphere c1{ physical1.radius };
      gjk::transformed_convex c1_(mth::inverse_transformation(m1) * m0, &c1);
      gjk::simplex simplex;
      if (gjk::gjk(c0, c1_, simplex)) {
        if (e0->has(state.behaviorComp)) {
          auto& behavior = e0->get(state.behaviorComp);
          if (behavior.onContactHandler != nullptr) {
            behavior.onContactHandler->onContact(e0, e1);
          }
        }
        if (e1->has(state.behaviorComp)) {
          auto& behavior = e1->get(state.behaviorComp);
          if (behavior.onContactHandler != nullptr) {
            behavior.onContactHandler->onContact(e1, e0);
          }
        }
      } else {
        vec3 p0, p1;
        gjk::closest(simplex, p0, p1);
        auto& gizmos = dbg::gizmos::instance();
        gizmos.drawLine(mth::transform_point(m0, p0),
                        mth::transform_point(m0, p1),
                        vec4{1, 1, 0, 1});
      }
    }
  }
};

#endif // FRONTEND_SIM_HPP
