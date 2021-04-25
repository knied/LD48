#ifndef FRONTEND_TASKS_HPP
#define FRONTEND_TASKS_HPP

inline geometry::mesh
consoleMesh() {
  geometry::mesh out;
  out.lines = false; // triangles

  float a = 0.4f; // half width
  float b = 0.4f; // depth
  float c = 0.4f; // height front
  float d = 0.5f; // height back
  vec3 norm{0,0,0};
  vec4 color{1,1,1,1};
  out.vd.push_back({ vec3{-a, 0, -0.5f + 0}, norm, color });
  out.vd.push_back({ vec3{-a, 0, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{-a, c, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{-a, 0, -0.5f + 0}, norm, color });
  out.vd.push_back({ vec3{-a, c, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{-a, d, -0.5f + 0}, norm, color });

  out.vd.push_back({ vec3{a, 0, -0.5f + 0}, norm, color });
  out.vd.push_back({ vec3{a, c, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{a, 0, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{a, 0, -0.5f + 0}, norm, color });
  out.vd.push_back({ vec3{a, d, -0.5f + 0}, norm, color });
  out.vd.push_back({ vec3{a, c, -0.5f + b}, norm, color });

  out.vd.push_back({ vec3{-a, 0, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{a, 0,  -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{a, c,  -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{-a, 0, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{a, c,  -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{-a, c, -0.5f + b}, norm, color });

  out.vd.push_back({ vec3{-a, c, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{a, c,  -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{a, d,  -0.5f + 0}, norm, color });
  out.vd.push_back({ vec3{-a, c, -0.5f + b}, norm, color });
  out.vd.push_back({ vec3{a, d,  -0.5f + 0}, norm, color });
  out.vd.push_back({ vec3{-a, d, -0.5f + 0}, norm, color });

  for (std::size_t i = 0; i < out.vd.size(); ++i) {
    out.id.push_back(i);
  }
  out.calculateNormals();

  return out;
}

#endif // FRONTEND_TASKS_HPP
