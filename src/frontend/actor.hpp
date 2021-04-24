#ifndef FRONTEND_ACTOR_HPP
#define FRONTEND_ACTOR_HPP

#include "geometry.hpp"

inline geometry::mesh
actorMesh() {
  geometry::mesh out;
  out.lines = false; // triangles

  float const a = 0.1f;
  float const b = 0.8f;
  vec4 color{1,1,1,1};

  out.vd.push_back({ vec3{0,0,0}, color }); // 0
  out.vd.push_back({ vec3{-a,b,0}, color }); // 1
  out.vd.push_back({ vec3{0,a+b,0}, color }); // 2
  out.vd.push_back({ vec3{a,b,0}, color }); // 3
  out.vd.push_back({ vec3{0,b,-a}, color }); // 4
  out.vd.push_back({ vec3{0,b,a}, color }); // 5

  out.id.push_back(0);
  out.id.push_back(4);
  out.id.push_back(1);
  out.id.push_back(1);
  out.id.push_back(4);
  out.id.push_back(2);
  out.id.push_back(2);
  out.id.push_back(4);
  out.id.push_back(3);
  out.id.push_back(3);
  out.id.push_back(4);
  out.id.push_back(0);

  out.id.push_back(0);
  out.id.push_back(1);
  out.id.push_back(5);
  out.id.push_back(1);
  out.id.push_back(2);
  out.id.push_back(5);
  out.id.push_back(2);
  out.id.push_back(3);
  out.id.push_back(5);
  out.id.push_back(3);
  out.id.push_back(0);
  out.id.push_back(5);

  return out;
}

#endif // FRONTEND_ACTOR_HPP
