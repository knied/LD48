////////////////////////////////////////////////////////////////////////////////

#include "../gjk.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

using vec3 = mth::vector<float,3>;

struct point_cloud : public gjk::convex {
  float padding;
  std::vector<vec3> points;
  point_cloud(float padding, std::vector<vec3> points)
    : padding(padding)
    , points(points) {}
  virtual vec3 support(vec3 const& dir) const {
    auto max = mth::dot(dir, points[0]);
    std::size_t maxi = 0;
    for (std::size_t i = 1; i < points.size(); ++i) {
      auto t = mth::dot(dir, points[i]);
      if (t > max) {
        max = t;
        maxi = i;
      }
    }
    return points[maxi] + padding * dir;
  }
};

static std::ostream&
operator << (std::ostream& stream, vec3 const& v) {
  stream << "(" << v(0) << "," << v(1) << "," << v(2) << ")";
  return stream;
}

#if 0
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

TEST(gjk, point_test) {
  auto c0 = point_cloud{ 0.0f, { vec3{0,0,0} } };
  auto c1 = point_cloud{ 0.0f, { vec3{1,0,0} } };
  auto c2 = point_cloud{ 0.0f, { vec3{1,1,0} } };
  auto c3 = point_cloud{ 0.0f, { vec3{1,1,1} } };
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c0, c1, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c0, c2, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c0, c3, tmp));
  }

  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c1, c0, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c1, c2, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c1, c3, tmp));
  }

  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c2, c0, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c2, c1, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c2, c3, tmp));
  }

  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c3, c0, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c3, c1, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c3, c2, tmp));
  }

  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c0, c0, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c1, c1, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c2, c2, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c3, c3, tmp));
  }
}

TEST(gjk, x_point_sphere_test) {
  auto c0 = point_cloud{ 1.0f, { vec3{0,0,0} } };
  auto c1 = point_cloud{ 0.0f, { vec3{0.5f,0,0} } };
  auto c2 = point_cloud{ 0.0f, { vec3{1,0,0} } };
  auto c3 = point_cloud{ 0.0f, { vec3{1.5f,0,0} } };
  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c0, c1, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c0, c2, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c0, c3, tmp));
  }
}

TEST(gjk, y_point_sphere_test) {
  auto c0 = point_cloud{ 1.0f, { vec3{0,0,0} } };
  auto c1 = point_cloud{ 0.0f, { vec3{0,0.5f,0} } };
  auto c2 = point_cloud{ 0.0f, { vec3{0,1,0} } };
  auto c3 = point_cloud{ 0.0f, { vec3{0,1.5f,0} } };
  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c0, c1, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c0, c2, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c0, c3, tmp));
  }
}

TEST(gjk, z_point_sphere_test) {
  auto c0 = point_cloud{ 1.0f, { vec3{0,0,0} } };
  auto c1 = point_cloud{ 0.0f, { vec3{0,0,0.5f} } };
  auto c2 = point_cloud{ 0.0f, { vec3{0,0,1} } };
  auto c3 = point_cloud{ 0.0f, { vec3{0,0,1.5f} } };
  {
    gjk::simplex tmp;
    EXPECT_TRUE(gjk::gjk(c0, c1, tmp));
  }
  {
    gjk::simplex tmp;
    // FIXME: shouldn't this collide?
    //EXPECT_TRUE(gjk::gjk(c0, c2, tmp));
  }
  {
    gjk::simplex tmp;
    EXPECT_FALSE(gjk::gjk(c0, c3, tmp));
  }
}

TEST(gjk, line_triangle_1) {
  auto p0 = vec3{ -1, -1, 0 };
  auto p1 = vec3{ 1, -1, 0 };
  auto p2 = vec3{ 0, 1, 0 };
  auto pl = vec3{ 0, 0, 0 };
  auto nl = vec3{ 0, 0, 1 };
  float c0, c1, c2;
  gjk::line_triangle(pl, nl, p0, p1, p2, c0, c1, c2);
  EXPECT_FLOAT_EQ(c0, 0.25f);
  EXPECT_FLOAT_EQ(c1, 0.25f);
  EXPECT_FLOAT_EQ(c2, 0.5f);
  std::cout << c0 << " " << c1 << " " << c2 << std::endl;
  std::cout << (c0 * p0 + c1 * p1 + c2 * p2) << std::endl;
}

TEST(gjk, line_triangle_2) {
  auto p0 = vec3{ -1, -1, 0 };
  auto p1 = vec3{ 1, -1, 0 };
  auto p2 = vec3{ 0, 1, 0 };
  auto pl = vec3{ 0.5f, 0, 0 };
  auto nl = vec3{ 0, 0, 1 };
  float c0, c1, c2;
  gjk::line_triangle(pl, nl, p0, p1, p2, c0, c1, c2);
  EXPECT_FLOAT_EQ(c0, 0.0f);
  EXPECT_FLOAT_EQ(c1, 0.5f);
  EXPECT_FLOAT_EQ(c2, 0.5f);
  std::cout << c0 << " " << c1 << " " << c2 << std::endl;
  std::cout << (c0 * p0 + c1 * p1 + c2 * p2) << std::endl;
}

TEST(gjk, line_triangle_3) {
  auto p0 = vec3{ -1, -1, 0 };
  auto p1 = vec3{ 1, -1, 0 };
  auto p2 = vec3{ 0, 1, 0 };
  auto pl = vec3{ -2, 0, 10 };
  auto nl = vec3{ 0, 0, 1 };
  float c0, c1, c2;
  gjk::line_triangle(pl, nl, p0, p1, p2, c0, c1, c2);
  EXPECT_FLOAT_EQ(c0, 1.25f);
  EXPECT_FLOAT_EQ(c1, -0.75f);
  EXPECT_FLOAT_EQ(c2, 0.5f);
  std::cout << c0 << " " << c1 << " " << c2 << std::endl;
  std::cout << (c0 * p0 + c1 * p1 + c2 * p2) << std::endl;
}

TEST(gjk, line_triangle_4) {
  auto p0 = vec3{ 0, 0, 0 };
  auto p1 = vec3{ 1, 0, 1 };
  auto p2 = vec3{ 0, 1, 1 };
  auto pl = vec3{ 0, 0, 1 };
  auto nl = vec3{ 1, 0, 0 };
  float c0, c1, c2;
  gjk::line_triangle(pl, nl, p0, p1, p2, c0, c1, c2);
  EXPECT_FLOAT_EQ(c0, 0.0f);
  EXPECT_FLOAT_EQ(c1, 1.0f);
  EXPECT_FLOAT_EQ(c2, 0.0f);
  std::cout << c0 << " " << c1 << " " << c2 << std::endl;
  std::cout << (c0 * p0 + c1 * p1 + c2 * p2) << std::endl;
}

TEST(gjk, lift_1) {
  auto c0 = point_cloud{ 1.0f, { vec3{-0.5f,0,0} } };
  auto c1 = point_cloud{ 1.0f, { vec3{0.5f,0,0} } };
  auto d = lift(c0, c1, vec3{1,0,0});
  EXPECT_FLOAT_EQ(d, 1.0f);
  std::cout << d << std::endl;
}

TEST(gjk, lift_2) {
  auto c0 = point_cloud{ 1.0f, { vec3{-1.5f,0,0} } };
  auto c1 = point_cloud{ 1.0f, { vec3{1.5f,0,0} } };
  auto d = lift(c0, c1, vec3{1,0,0});
  EXPECT_FLOAT_EQ(d, -1.0f);
  std::cout << d << std::endl;
}

TEST(gjk, lift_3) {
  auto c0 = point_cloud{ 1.0f, { vec3{-0.5f,2,0} } };
  auto c1 = point_cloud{ 1.0f, { vec3{0.5f,-2,0} } };
  auto d = lift(c0, c1, vec3{1,0,0});
  EXPECT_FLOAT_EQ(d, 0.0f);
  std::cout << d << std::endl;
}

TEST(gjk, lift_4) {
  auto c0 = point_cloud{ 1.0f, { vec3{-0.5f,0.5f,0} } };
  auto c1 = point_cloud{ 1.0f, { vec3{0.5f,-0.5f,0} } };
  auto d = lift(c0, c1, vec3{1,0,0});
  // FIXME: distance mismatch?
  //EXPECT_FLOAT_EQ(d, 2.0f * mth::sin(mth::pi / 4.0f) - 1.0f);
  std::cout << d << std::endl;
}

static point_cloud unit_cube(vec3 const& o) {
  return point_cloud{ 0.0f, {
      o + vec3{-1,-1,-1}, o + vec3{1,-1,-1}, o + vec3{1,1,-1},  o + vec3{1,1,1},
      o + vec3{-1,1,-1},  o + vec3{-1,1,1},  o + vec3{-1,-1,1}, o + vec3{1,-1,1}
    }};
}
TEST(gjk, lift_5) {
  auto c0 = unit_cube(vec3{-0.5f,-0.5f,0});
  auto c1 = unit_cube(vec3{0.5f,0.5f,0});
  auto d = lift(c0, c1, vec3{1,0,0});
  EXPECT_FLOAT_EQ(d, 1.0f);
  std::cout << d << std::endl;
}

TEST(gjk, lift_6) {
  auto c0 = unit_cube(vec3{0.5f,-0.5f,0});
  auto c1 = unit_cube(vec3{-0.5f,0.5f,0});
  auto d = lift(c0, c1, vec3{1,0,0});
  EXPECT_FLOAT_EQ(d, 3.0f);
  std::cout << d << std::endl;
}

TEST(gjk, lift_7) {
  auto c0 = unit_cube(vec3{-0.5f,-0.5f,0});
  auto c1 = unit_cube(vec3{0.5f,0.5f,0});
  auto d = lift(c0, c1, vec3{1,1,0});
  EXPECT_FLOAT_EQ(d, mth::sqrt(2.0f));
  std::cout << d << std::endl;
}

TEST(gjk, lift_8) {
  auto c0 = point_cloud{ 1.0f, { vec3{-0.5f,0.5f,0} } };
  auto c1 = point_cloud{ 1.0f, { vec3{0.5f,-0.5f,0} } };
  auto d = lift(c0, c1, mth::normal(vec3{1,1,0}));
  EXPECT_FLOAT_EQ(d, mth::sqrt(2.0f));
  std::cout << d << std::endl;
}

TEST(gjk, lift_9) {
  auto c0 = point_cloud{ 1.0f, { vec3{-0.5f,-0.5f,-0.5f} } };
  auto c1 = point_cloud{ 1.0f, { vec3{0.5f,0.5f,0.5f} } };
  gjk::simplex tmp;
  EXPECT_TRUE(gjk::gjk(c0, c1, tmp));
  auto d = lift(c0, c1, mth::normal(vec3{1,1,0}));
  // FIXME: distance mismatch?
  //EXPECT_FLOAT_EQ(d, mth::sqrt(2.0f));
  std::cout << d << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
