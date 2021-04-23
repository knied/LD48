////////////////////////////////////////////////////////////////////////////////

#ifndef COMMON_GJK_HPP
#define COMMON_GJK_HPP

////////////////////////////////////////////////////////////////////////////////

#include "mth.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace gjk {

////////////////////////////////////////////////////////////////////////////////

using vec3 = mth::vector<float,3>;
using mat4 = mth::matrix<float,4,4>;

struct convex {
  virtual vec3 support(vec3 const& dir) const = 0;
};

struct transformed_convex : public convex {
  transformed_convex(mat4 const& trans, convex const* c)
    : local_to_world(trans)
    , world_to_local(mth::inverse_transformation(trans))
    , c(c) {}
  virtual vec3 support(vec3 const& dir) const override {
    return mth::transform_point(world_to_local, c->support(
                                  mth::transform_vector(local_to_world, dir)));
  }
private:
  mat4 local_to_world;
  mat4 world_to_local;
  convex const* c;
};

struct sphere : public convex {
  sphere(float radius) : radius(radius) {}
  virtual vec3 support(vec3 const& dir) const override {
    return radius * dir;
  }
private:
  float radius;
};

struct support {
  support() = default;
  support(vec3 const& s0, vec3 const& s1)
    : s(s0 - s1), s0(s0), s1(s1) {}
  vec3 s, s0, s1;
};

struct simplex {
  support s[4];
  int n = 0;
};

bool gjk(convex const& c0, convex const& c1, simplex& out);
vec3 epa(convex const& c0, convex const& c1, simplex const& hint); 
void closest(simplex const& in, vec3& p0, vec3& p1);
float separate(convex const& c0, convex const& c1, vec3 const& dir);

// helper
void
line_triangle(vec3 const& pl, vec3 const& nl,
              vec3 const& p0, vec3 const& p1, vec3 const& p2,
              float& c0, float& c1, float& c2);

////////////////////////////////////////////////////////////////////////////////

} // namespace gjk

////////////////////////////////////////////////////////////////////////////////

#endif // COMMON_GJK_HPP

////////////////////////////////////////////////////////////////////////////////
