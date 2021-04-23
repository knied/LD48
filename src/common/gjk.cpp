////////////////////////////////////////////////////////////////////////////////

#include "gjk.hpp"
#include <cassert>
#include <array>

#define DEBUG_OUTPUT
#ifdef DEBUG_OUTPUT
#include <iostream>
#endif

////////////////////////////////////////////////////////////////////////////////

namespace gjk {

////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_OUTPUT
static std::ostream&
operator << (std::ostream& stream, vec3 const& v) {
  stream << "(" << v(0) << "," << v(1) << "," << v(2) << ")";
  return stream;
}
static std::ostream&
operator << (std::ostream& stream, gjk::support const& s) {
  stream << s.s;
  return stream;
}
static std::ostream&
operator << (std::ostream& stream, gjk::simplex const& s) {
  stream << "{" << std::endl;
  for (int i = 0; i < s.n; ++i) {
    stream << s.s[i] << std::endl;
  }
  stream << "}" << std::endl;
  return stream;
}
#endif
bool gjk(convex const& c0, convex const& c1, simplex& out) {
  float const eps = 0.0001f;
  vec3 dir{1, 0, 0};
  auto& simplex = out.s;
  auto& n = out.n;
  // Helpers to manimpulate the simplex
  auto reduce1 = [&simplex, &n](int i0) {
    simplex[0] = simplex[i0];
    n = 1;
  };
  auto reduce2 = [&simplex, &n](int i0, int i1) {
    support tmp[]
      = { simplex[0], simplex[1], simplex[2], simplex[3] };
    simplex[0] = tmp[i0];
    simplex[1] = tmp[i1];
    n = 2;
  };
  auto reduce3 = [&simplex, &n](int i0, int i1, int i2) {
    support tmp[]
      = { simplex[0], simplex[1], simplex[2], simplex[3] };
    simplex[0] = tmp[i0];
    simplex[1] = tmp[i1];
    simplex[2] = tmp[i2];
    n = 3;
  };
  auto extend = [&simplex, &n, eps](vec3 const& s0, vec3 const& s1) {
    //std::cout << "extend " << s0 << " / " << s1 << std::endl;
    support support(s0, s1);
    switch (n) {
    case 0: break;
    case 1: { // point
      auto v01 = support.s - simplex[0].s;
      if (mth::length2(v01) < eps) {
        return false; // no collision
      }
    } break;
    case 2: { // line
      auto v10 = simplex[0].s - simplex[1].s;
      auto v21 = simplex[1].s - support.s;
      auto normal = mth::cross(v10, v21);
      if (mth::length2(normal) / mth::length2(v10) < eps) {
        return false; // no collision
      }
    } break;
    case 3: { // triangle
      auto v02 = simplex[2].s - simplex[0].s;
      auto v01 = simplex[1].s - simplex[0].s;
      auto v03 = support.s - simplex[0].s;
      auto normal = mth::normal(mth::cross(v02, v01));
      if (mth::dot(normal, v03) < eps) {
        return false; // no collision
      }
    } break;
    default:
      //std::cout << n << std::endl;
      assert(false && "Invalid simplex size");
    };
    simplex[n++] = support;
    return true; // continue
  };

  int iterations = 0;
  while (iterations++ < 50) {
    {
      auto l = mth::length(dir);
      if (l < eps) {
        // The direction vector is almost zero. We assume the origin to be
        // contained in the current simplex.
        std::cout << out << std::endl;
        return true; // collision
      }
      dir = dir / l;
    }
    if (!extend(c0.support(dir),
                c1.support(-dir))) {
      // The new support point would be very close to the simplex. We call it
      // done here and consider the two convex shapes: not colliding.
      return false; // no collision
    }
    switch (n) {
    case 1: { // point
      // We need to extend to a line
      dir = -simplex[0].s;
    } break;
    case 2: { // line
      auto v1_ = -simplex[1].s;
      auto v10 = simplex[0].s - simplex[1].s;
      if (mth::dot(v10, v1_) > eps) {
        // We need to extend to a triangle
        dir = mth::cross(cross(v10, v1_), v10);
      } else {
        // We can eliminate the first support and extend to a new line.
        dir = v1_;
        reduce1(1);
      }
    } break;
    case 3: { // triangle
      auto v2_ = -simplex[2].s;
      auto v20 = simplex[0].s - simplex[2].s;
      auto v21 = simplex[1].s - simplex[2].s;
      auto normal = cross(v21, v20);
      if (mth::dot(mth::cross(normal, v20), v2_) > eps) {
        if (mth::dot(v20, v2_) > 0) {
          dir = mth::cross(mth::cross(v20, v2_), v20);
          reduce2(0, 2);
        } else if (mth::dot(v21, v2_) > 0) {
          dir = mth::cross(mth::cross(v21, v2_), v21);
          reduce2(1, 2);
        } else {
          dir = v2_;
          reduce1(2);
        }
      } else if (mth::dot(mth::cross(v21, normal), v2_) > eps) {
        if (mth::dot(v21, v2_) > 0) {
          dir = mth::cross(mth::cross(v21, v2_), v21);
          reduce2(1, 2);
        } else {
          dir = v2_;
          reduce1(2);
        }
      } else {
        if (mth::dot(normal, v2_) > eps) {
          dir = normal;
        } else {
          dir = -normal;
          reduce3(2, 1, 0);
        }
      }
    } break;
    case 4: { // tetrahedon
      auto v3_ = -simplex[3].s;
      auto v_3 = simplex[3].s;
      auto v30 = simplex[0].s - simplex[3].s;
      auto v31 = simplex[1].s - simplex[3].s;
      auto v32 = simplex[2].s - simplex[3].s;
      auto normal123 = mth::cross(v32, v31);
      auto normal013 = mth::cross(v31, v30);
      auto normal203 = mth::cross(v30, v32);
      if (mth::dot(normal123, v3_) > 0) {
        if (mth::dot(mth::cross(normal123, v32), v_3) > 0) {
          if (mth::dot(mth::cross(normal203, v32), v3_) < 0) {
            if (mth::dot(mth::cross(normal203, v30), v_3) < 0) {
              dir = normal203;
              reduce3(2, 0, 3);
            } else if (mth::dot(mth::cross(normal013, v30), v3_) < 0) {
              dir = normal013;
              reduce3(0, 1, 3);
            } else if (mth::dot(v30, v3_) > 0) {
              dir = mth::cross(mth::cross(v30, v3_), v30);
              reduce2(0, 3);
            } else {
              dir = v3_;
              reduce1(3);
            }
          } else if (mth::dot(v32, v3_) > 0) {
            dir = cross(cross(v32, v3_), v32);
            reduce2(2, 3);
          } else {
            dir = v3_;
            reduce1(3);
          }
        } else if (mth::dot(mth::cross(normal123, v31), v3_) > 0) {
          if (mth::dot(mth::cross(normal013, v31), v_3) < 0) {
            dir = normal013;
            reduce3(0, 1, 3);
          } else if (mth::dot(v31, v3_) > 0) {
            dir = mth::cross(mth::cross(v31, v3_), v31);
            reduce2(1, 3);
          } else {
            dir = v3_;
            reduce1(3);
          }
        } else {
          dir = normal123;
          reduce3(1, 2, 3);
        }
      } else if (mth::dot(normal013, v3_) > 0) {
        if (mth::dot(mth::cross(normal013, v3_), v_3) > 0) {
          if (mth::dot(v31, v3_) > 0) {
            dir = mth::cross(mth::cross(v31, v3_), v31);
            reduce2(1, 3);
          } else {
            dir = v3_;
            reduce1(3);
          }
        } else if (mth::dot(mth::cross(normal013, v30), v3_) > 0) {
          if (mth::dot(mth::cross(normal203, v30), v3_) < 0) {
            if (mth::dot(mth::cross(normal203, v32), v3_) < 0) {
              dir = normal203;
              reduce3(2, 0, 3);
            } else if (mth::dot(v32, v3_) > 0) {
              dir = mth::cross(mth::cross(v32, v3_), v32);
              reduce2(2, 3);
            } else {
              dir = v3_;
              reduce1(3);
            }
          } else if (mth::dot(v30, v3_) > 0) {
            dir = mth::cross(mth::cross(v30, v3_), v30);
            reduce2(0, 3);
          } else {
            dir = v3_;
            reduce1(3);
          }
        } else {
          dir = normal013;
          reduce3(0, 1, 3);
        }
      } else if (mth::dot(normal203, v3_) > 0) {
        if (mth::dot(mth::cross(normal203, v32), v3_) > 0) {
          if (mth::dot(v32, v3_) > 0) {
            dir = mth::cross(mth::cross(v32, v3_), v32);
            reduce2(2, 3);
          } else {
            dir = v3_;
            reduce1(3);
          }
        } else if (mth::dot(mth::cross(normal203, v30), v_3) > 0) {
          if (mth::dot(v30, v3_) > 0) {
            dir = mth::cross(mth::cross(v30, v3_), v30);
            reduce2(0, 3);
          } else {
            dir = v3_;
            reduce1(3);
          }
        } else {
          dir = normal203;
          reduce3(2, 0, 3);
        }
      } else {
        return true; // collision
      }
    } break;
    default:
      assert(false && "Invalid simplex size");
    };
  }
  //std::cout << "WARNING: GJK was cut short" << std::endl;
  return false;
}

vec3 epa(convex const& c0, convex const& c1, simplex const& hint) {
  (void)c0;
  (void)c1;
  (void)hint;
  assert(false && "Not implemented");
  // TODO: Iteratively refine simplex to find the shortest separating vector
  return vec3{0,0,0};
}

// Project origin onto line through p0 and p1
// p = c0 * p0 + c1 * p1
static inline void
origin_on_line(vec3 const& p0, vec3 const& p1,
               float& c0, float& c1) {
  auto l = p1 - p0;
  c1 = mth::dot(-l, p0) / mth::dot(l, l);
  c0 = 1.0f - c1;
}

// Project origin onto triangle with corners p0, p1 and p2
// p = c0 * p0 + c1 * p1 + c2 * p2
static inline void
origin_on_triangle(vec3 const& p0, vec3 const& p1, vec3 const& p2,
                   float& c0, float& c1, float& c2) {
  auto n = cross(p1 - p0, p2 - p0);
  float ndot = mth::dot(n, n);
  auto p = n * mth::dot(p0, n) / ndot;
  auto v0 = p1 - p0;
  auto v1 = p2 - p0;
  auto v2 = p - p0;
  float d00 = mth::dot(v0, v0);
  float d01 = mth::dot(v0, v1);
  float d11 = mth::dot(v1, v1);
  float d20 = mth::dot(v2, v0);
  float d21 = mth::dot(v2, v1);
  float denom = d00 * d11 - d01 * d01;
  c1 = (d11 * d20 - d01 * d21) / denom;
  c2 = (d00 * d21 - d01 * d20) / denom;
  c0 = 1.0f - c1 - c2;
}

void closest(simplex const& in, vec3& p0, vec3& p1) {
  auto const& simplex = in.s;
  switch (in.n) {
  case 1: {
    p0 = simplex[0].s0;
    p1 = simplex[0].s1;
  } break;
  case 2: {
    float c0, c1;
    origin_on_line(simplex[0].s, simplex[1].s, c0, c1);
    p0 = c0 * simplex[0].s0 + c1 * simplex[1].s0;
    p1 = c0 * simplex[0].s1 + c1 * simplex[1].s1;
  } break;
  case 3: {
    float c0, c1, c2;
    origin_on_triangle(simplex[0].s, simplex[1].s, simplex[2].s, c0, c1, c2);
    p0 = c0 * simplex[0].s0 + c1 * simplex[1].s0 + c2 * simplex[2].s0;
    p1 = c0 * simplex[0].s1 + c1 * simplex[1].s1 + c2 * simplex[2].s1;
  } break;
  default:
    assert(false && "Invalid simplex size");
  };
}

void
line_triangle(vec3 const& pl, vec3 const& nl,
              vec3 const& p0, vec3 const& p1, vec3 const& p2,
              float& c0, float& c1, float& c2) {
  auto u = p1 - p0;
  auto v = p2 - p0;
  auto n = mth::cross(u, v);
  auto p = pl + nl * (mth::dot(n, p0 - pl) / mth::dot(n, nl));
  auto w = p - p0;
  auto uv = mth::dot(u, v);
  auto uu = mth::dot(u, u);
  auto vv = mth::dot(v, v);
  auto wv = mth::dot(w, v);
  auto wu = mth::dot(w, u);
  auto r = uv * uv - uu * vv;
  c1 = (uv * wv - vv * wu) / r;
  c2 = (uv * wu - uu * wv) / r;
  c0 = 1.0f - c2 - c1;
}

#if 0
// Test if line (pl + t * nl) has common points with triangle (p0, p1, p2)
static inline bool
line_triangle_test(vec3 const& pl, vec3 const& nl,
                   vec3 const& p0, vec3 const& p1, vec3 const& p2) {
  auto u = p1 - p0;
  auto v = p2 - p0;
  auto n = mth::cross(u, v);
  if (mth::dot(nl, n) > 0) return false;
  auto p = pl + nl * (mth::dot(n, p0 - pl) / mth::dot(n, nl));
  auto w = p - p0;
  auto uv = mth::dot(u, v);
  auto uu = mth::dot(u, u);
  auto vv = mth::dot(v, v);
  auto wv = mth::dot(w, v);
  auto wu = mth::dot(w, u);
  auto r = uv * uv - uu * vv;
  auto s = (uv * wv - vv * wu) / r;
  auto t = (uv * wu - uu * wv) / r;
  return s >= 0 && t >= 0 && (s + t) <= 1;
}
#endif

float separate(convex const& c0, convex const& c1, vec3 const& dir) {
  float constexpr eps = 0.0001f;
  auto ndir = mth::normal(dir);
  auto n = ndir;

  // point
  //std::cout << "n: "  << n << " / " << mth::length(n) << std::endl;
  auto s0 = support(c0.support(n), c1.support(-n));
  //std::cout << "s0: " << s0.s << std::endl;
  //std::cout << "test: " << mth::cross(vec3{1,0,0}, vec3{0,1,0}) << std::endl;
  //std::cout << "cross: " << mth::cross(s0.s, n) << std::endl;
  auto t = mth::cross(mth::cross(s0.s, n), n);
  //std::cout << "t: " << t << std::endl;
  auto tl = mth::length(t);
  if (tl < eps) {
    //std::cout << "a" << std::endl;
    return mth::dot(s0.s, ndir); // close enough [tested]
  }
  n = t / tl;
    
  // line
  //std::cout << "n: "  << n << " / " << mth::length(n) << std::endl;
  auto s1 = support(c0.support(n), c1.support(-n));
  //std::cout << "s1: " << s1.s << std::endl;
  if (mth::dot(s1.s, n) < eps) {
    //std::cout << "b" << std::endl;
    return 0.0f; // the two shapes do no overlap in the plane with normal dir [tested]
  }
  bool expandToTriangle = false;
  while (!expandToTriangle) {
    auto t = mth::normal(mth::cross(ndir, s1.s - s0.s));
    auto d = mth::dot(t, s0.s);
    if (d > eps) {
      n = -t;
      expandToTriangle = true;
    } else if (d < -eps) {
      n = t;
      expandToTriangle = true;
    } else {
      // crossing origin
      auto nl = mth::normal(s1.s - s0.s);
      n = mth::cross(nl, t);
      //std::cout << "n: "  << n << " / " << mth::length(n) << std::endl;
      auto st = support(c0.support(n), c1.support(-n));
      //std::cout << "st: " << st.s << std::endl;
      auto t = mth::cross(mth::cross(st.s, ndir), ndir);
      auto tl = mth::length(t);
      if (tl < eps) {
        //std::cout << "c" << std::endl;
        return mth::dot(st.s, ndir); // close enough [tested]
      }
      t = t / tl;
      //std::cout << "t: " << t << " : " << mth::dot(t, s0.s) << " > " << mth::dot(t, s1.s) << std::endl;
      //auto dt = mth::dot(t, st.s);
      auto d0 = mth::dot(t, s0.s);
      auto d1 = mth::dot(t, s1.s);
      if (d0 > d1) {
        //std::cout << "keep s0" << std::endl;
        if (mth::length(s1.s - st.s) < eps) {
          auto a = d0 * (d0 - d1);
          auto r0 = mth::dot(ndir, s0.s);
          auto r1 = mth::dot(ndir, s1.s);
          return a * r0 + (1 - a) * r1;
        }
        s1 = st;
      } else {
        //std::cout << "keep s1" << std::endl;
        if (mth::length(s0.s - st.s) < eps) {
          auto a = d1 * (d1 - d0);
          auto r0 = mth::dot(ndir, s0.s);
          auto r1 = mth::dot(ndir, s1.s);
          return a * r1 + (1 - a) * r0;
        }
        s0 = st;
      }
    }
  }

  //std::cout << "e" << std::endl;
  assert(false && "implement me");
  
  //auto s2 = support(c0.support(n), c1.support(-n));
  return 0.0f;
  
#if 0
  vec3 const o{0,0,0};
  std::array<support,3> supports;
  if (line_triangle_test(o, dir, hint.s[1].s, hint.s[0].s, hint.s[3].s)) {
    supports = { hint.s[1], hint.s[0], hint.s[3] };
  } else if (line_triangle_test(o, dir, hint.s[2].s, hint.s[1].s, hint.s[3].s)) {
    supports = { hint.s[2], hint.s[1], hint.s[3] };
  } else if (line_triangle_test(o, dir, hint.s[0].s, hint.s[2].s, hint.s[3].s)) {
    supports = { hint.s[0], hint.s[2], hint.s[3] };
  } else if (line_triangle_test(o, dir, hint.s[1].s, hint.s[2].s, hint.s[0].s)) {
    supports = { hint.s[1], hint.s[2], hint.s[0] };
  } else {
    assert(false);
    return o;
  }

  // Refine the triangle
  int i = 0;
  vec3 n;
  while (i++ < 100) {
    n = mth::normal(mth::cross(supports[1].s - supports[0].s,
                                    supports[2].s - supports[0].s));
    auto s = support(c0.support(n), c1.support(-n));
    if (mth::dot(n, s.s) - mth::dot(n, supports[0].s) < eps) {
      break;
    }
    if (line_triangle_test(o, dir, supports[0].s, supports[1].s, s.s)) {
      supports = { supports[0], supports[1], s };
    } else if (line_triangle_test(o, dir, supports[0].s, s.s, supports[2].s)) {
      supports = { supports[0], s, supports[2] };
    } else if (line_triangle_test(o, dir, s.s, supports[1].s, supports[2].s)) {
      supports = { s, supports[1], supports[2] };
    } else {
      break;
    }
  }
  return -dir * (mth::dot(n, supports[0].s) / mth::dot(n, dir));
#endif
}
////////////////////////////////////////////////////////////////////////////////

} // namespace gjk

////////////////////////////////////////////////////////////////////////////////
