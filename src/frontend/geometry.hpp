#ifndef FRONTEND_GEOMETRY_HPP
#define FRONTEND_GEOMETRY_HPP

#include <common/mth.hpp>
#include <common/noise.hpp>
#include <vector>

namespace geometry {

using vec2 = mth::vector<float,2>;
using vec3 = mth::vector<float,3>;
using vec4 = mth::vector<float,4>;
using mat4 = mth::matrix<float,4,4>;

struct vertex { vec3 pos; vec3 norm; vec4 color; };

struct mesh {
  bool lines = true;
  std::vector<vertex> vd;
  std::vector<unsigned int> id;

  void calculateNormals(std::size_t first = 0, std::size_t last = 0) {
    if (last == 0) last = id.size();
    assert(lines == false);
    for (auto& v : vd) {
      v.norm = vec3{0,0,0};
    }
    for (std::size_t i = first; i < last; i += 3) {
      auto& p0 = vd[i + 0].pos;
      auto& p1 = vd[i + 1].pos;
      auto& p2 = vd[i + 2].pos;
      auto normal = mth::normal(mth::cross(p1 - p0, p2 - p0));
      vd[i + 0].norm += normal;
      vd[i + 1].norm += normal;
      vd[i + 2].norm += normal;
    }
    for (auto& v : vd) {
      v.norm = mth::normal(v.norm);
    }
  }

  static mesh merge(mesh const& m0, mesh const& m1) {
    mesh out;
    out.lines = m0.lines;
    out.vd = m0.vd;
    out.id = m0.id;
    auto idx = m0.vd.size();
    out.vd.insert(out.vd.end(), m1.vd.begin(), m1.vd.end());
    for (auto i : m1.id) {
      out.id.push_back(idx + i);
    }
    return out;
  }
};

inline vec3
normal_on_sphere(float phi, float theta) {
  auto sin_theta = mth::sin(theta);
  auto cos_theta = mth::cos(theta);
  auto sin_phi = mth::sin(phi);
  auto cos_phi = mth::cos(phi);
  return vec3{
    sin_theta * cos_phi,
    sin_theta * sin_phi,
    cos_theta
  };
}

inline mesh
generate_cylinder(float radius, float l, int detail, vec4 const& color) {
  mesh out;
  out.lines = false;
  if (detail < 3) {
    detail = 3;
  }
  for (int t = 0; t < detail; ++t) {
    auto theta = 2.0f * (float)mth::pi * (float)t / (float)(detail);
    vec3 normal{mth::sin(theta), 0.0f, mth::cos(theta)};
    auto position = radius * normal;
    out.vd.push_back({position, normal, color});
    out.vd.push_back({position + vec3{0,l,0}, normal, color});
    
    out.vd.push_back({position, vec3{0,-1,0}, color});
    out.vd.push_back({position + vec3{0,l,0}, vec3{0,1,0}, color});
  }
  
  for (int t = 0; t < detail - 1; ++t) {
    out.id.push_back(4*t);
    out.id.push_back(4*t+4);
    out.id.push_back(4*t+5);
    out.id.push_back(4*t);
    out.id.push_back(4*t+5);
    out.id.push_back(4*t+1);
    out.id.push_back(4*detail-2);
    out.id.push_back(4*t+6);
    out.id.push_back(4*t+2);
    out.id.push_back(4*detail-1);
    out.id.push_back(4*t+3);
    out.id.push_back(4*t+7);
  }
  out.id.push_back((4*detail-4));
  out.id.push_back(0);
  out.id.push_back(1);
  out.id.push_back((4*detail-4));
  out.id.push_back(1);
  out.id.push_back((4*detail-4)+1);
  return out;
}
                                          
inline mesh
generate_sphere(float radius, int detail, vec4 const& color, bool wire = false) {
  mesh out;
  out.lines = wire;
  if (detail < 3) {
    detail = 3;
  }
  for (int t = 0; t < detail; ++t) {
    auto theta = (float)mth::pi * (float)t / (float)(detail - 1);
    for (int p = 0; p < ((t > 0 && t < detail - 1) ? 2 * detail : 2 * detail - 1); ++p) {
      auto phi = (float)mth::pi * (float)p / (float)(detail-1);
      auto normal = normal_on_sphere(phi, theta);
      auto position = radius * normal;
      out.vd.push_back({position, normal, color});
    }
  }
  if (wire) {
    for (int t = 0; t < detail - 1; ++t) {
      auto i0 = t * 2 * detail - 1;
      auto i1 = (t + 1) * 2 * detail - 1;
      for (int p = 0; p < (2 * detail - 1); ++p) {
        if (t > 0) {
          out.id.push_back(i0 + p);
          out.id.push_back(i1 + p);
          out.id.push_back(i1 + p);
          out.id.push_back(i0 + p + 1);
          out.id.push_back(i0 + p + 1);
          out.id.push_back(i0 + p);
        }
        if (t < detail-2) {
          out.id.push_back(i0 + p + 1);
          out.id.push_back(i1 + p);
          out.id.push_back(i1 + p);
          out.id.push_back(i1 + p + 1);
          out.id.push_back(i1 + p + 1);
          out.id.push_back(i0 + p + 1);
        }
      }
    }
  } else {
    for (int t = 0; t < detail - 1; ++t) {
      auto i0 = t * 2 * detail - 1;
      auto i1 = (t + 1) * 2 * detail - 1;
      for (int p = 0; p < (2 * detail - 1); ++p) {
        if (t > 0) {
          out.id.push_back(i0 + p);
          out.id.push_back(i1 + p);
          out.id.push_back(i0 + p + 1);
        }
        if (t < detail-2) {
          out.id.push_back(i0 + p + 1);
          out.id.push_back(i1 + p);
          out.id.push_back(i1 + p + 1);
        }
      }
    }
  }
  return out;
}

/*inline mesh
generate_box(float x, float y, float z,
             vec4 const& color, bool wire = false) {
  auto hx = 0.5f * x;
  auto hy = 0.5f * y;
  auto hz = 0.5f * z;
  mesh out;
  out.vd = {
    // front
    { vec3{ -hx, -hy, -hz }, color },
    { vec3{ hx, -hy, -hz }, color },
    { vec3{ hx, hy, -hz }, color },
    { vec3{ -hx, -hy, -hz }, color },
    { vec3{ hx, hy, -hz }, color },
    { vec3{ -hx, hy, -hz }, color },
    // back
    { vec3{ -hx, -hy, hz }, color },
    { vec3{ hx, hy, hz }, color },
    { vec3{ hx, -hy, hz }, color },
    { vec3{ -hx, -hy, hz }, color },
    { vec3{ -hx, hy, hz }, color },
    { vec3{ hx, hy, hz }, color },
    // top
    { vec3{ -hx, hy, -hz }, color },
    { vec3{ hx, hy, -hz }, color },
    { vec3{ hx, hy, hz }, color },
    { vec3{ -hx, hy, -hz }, color },
    { vec3{ hx, hy, hz }, color },
    { vec3{ -hx, hy, hz }, color },
    // bottom
    { vec3{ -hx, -hy, -hz }, color },
    { vec3{ hx, -hy, hz }, color },
    { vec3{ hx, -hy, -hz }, color },
    { vec3{ -hx, -hy, -hz }, color },
    { vec3{ -hx, -hy, hz }, color },
    { vec3{ hx, -hy, hz }, color },
    // left
    { vec3{ -hx, -hy, hz }, color },
    { vec3{ -hx, -hy, -hz }, color },
    { vec3{ -hx, hy, -hz }, color },
    { vec3{ -hx, -hy, hz }, color },
    { vec3{ -hx, hy, -hz }, color },
    { vec3{ -hx, hy, hz }, color },
    // right
    { vec3{ hx, -hy, hz }, color },
    { vec3{ hx, hy, -hz }, color },
    { vec3{ hx, -hy, -hz }, color },
    { vec3{ hx, -hy, hz }, color },
    { vec3{ hx, hy, hz }, color },
    { vec3{ hx, hy, -hz }, color },
  };
  if (wire) {
    for (int i = 0; i < 12; ++i) {
      out.id.push_back(3 * i + 0);
      out.id.push_back(3 * i + 1);
      out.id.push_back(3 * i + 1);
      out.id.push_back(3 * i + 2);
      out.id.push_back(3 * i + 2);
      out.id.push_back(3 * i + 0);
    }
  } else {
    for (unsigned int i = 0; i < out.vd.size(); ++i) {
      out.id.push_back(i);
    }
  }
  return out;
  }*/

/*inline mesh
generate_terrain(int width, int height,
                 vec4 const& color, bool wire = false) {
  mesh out;
  auto index_at = [height](int x, int y) {
    return x * (height + 1) + y;
  };
  for (int x = 0; x < width + 1; ++x) {
    for (int y = 0; y < height + 1; ++y) {
      auto h = (float)perlin::noise((double)x / (double)(width) * 5.0,
                                    (double)y / (double)(height) * 5.0,
                                    0.0);
      out.vd.push_back({vec3{ (float)x, 1.0f * h, (float)-y }, color});
    }
  }
  if (wire) {
    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        out.id.push_back(index_at(x,y));
        out.id.push_back(index_at(x+1,y));
        out.id.push_back(index_at(x+1,y));
        out.id.push_back(index_at(x+1,y+1));
        out.id.push_back(index_at(x+1,y+1));
        out.id.push_back(index_at(x,y+1));
        out.id.push_back(index_at(x,y+1));
        out.id.push_back(index_at(x,y));
        out.id.push_back(index_at(x,y));
        out.id.push_back(index_at(x+1,y+1));
      }
    }
  } else {
    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        out.id.push_back(index_at(x,y));
        out.id.push_back(index_at(x+1,y));
        out.id.push_back(index_at(x+1,y+1));
        out.id.push_back(index_at(x,y));
        out.id.push_back(index_at(x+1,y+1));
        out.id.push_back(index_at(x,y+1));
      }
    }
  }
  return out;
  }*/

} // geometry

#endif // FRONTEND_GEOMETRY_HPP
