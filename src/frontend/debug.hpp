#ifndef FRONTEND_DEBUG_HPP
#define FRONTEND_DEBUG_HPP

#include "geometry.hpp"

namespace dbg {

using vec2 = mth::vector<float,2>;
using vec3 = mth::vector<float,3>;
using vec4 = mth::vector<float,4>;

class gizmos final {
private:
  gizmos() = default;
public:
  void drawLine(vec3 const& p0, vec3 const&  p1,
                vec4 const& color) {
    mMesh.vd.push_back({ p0, color});
    mMesh.vd.push_back({ p1, color});
    mMesh.id.push_back((unsigned int)mMesh.id.size());
    mMesh.id.push_back((unsigned int)mMesh.id.size());
  }
  void clear() {
    mMesh.vd.clear();
    mMesh.id.clear();
  }

  geometry::mesh const& mesh() const {
    return mMesh;
  }

  static gizmos& instance() {
    static gizmos i;
    return i;
  }

private:
  geometry::mesh mMesh;
};

};

#endif // FRONTEND_DEBUG_HPP
