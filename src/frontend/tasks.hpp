#ifndef FRONTEND_TASKS_HPP
#define FRONTEND_TASKS_HPP

#include "state.hpp"
#include "input.hpp"
#include "map.hpp"

class TaskBehavior : public Comp::Behavior::OnUpdate {
public:
  TaskBehavior(Map* map)
    : mMap(map)
    , mUseKey("KeyE"){
    mSounds.push_back("task1");
    mSounds.push_back("task2");
    mSounds.push_back("task3");
  }
  virtual void onUpdate(Entity* self, float dt) override {
    float const radius = 0.2f;
    auto& state = GameState::instance();
    auto& task = self->get(state.taskComp);
    auto& trans = self->get(state.transComp);
    auto& instr = self->get(state.instrComp);
    if (!task.complete) {
      instr.pressE = true;
      auto& playerActor = state.player->get(state.actorComp);
      if (mth::length(playerActor.pos - task.pos) < 2.0f * radius) {
        if (mUseKey.keydown() > 0) {
          if (task.plant) {
            game_play_sound("water");
          } else {
            game_play_sound(mSounds[mNextSound++].c_str());
            if (mNextSound >= mSounds.size()) mNextSound = 0;
          }
          task.complete = true;
          instr.show = false;
        } else {
          instr.show = true;
        }
      } else {
        instr.show = false;
      }
    } else {
      if (self->has(state.crewComp)) {
        auto& playerActor = state.player->get(state.actorComp);
        if (mth::length(playerActor.pos - task.pos) < 2.0f * radius) {
          auto& crew = self->get(state.crewComp);
          instr.msg = crew.name + (crew.health > 0 ? "" : " is Dead");
          instr.pressE = false;
          instr.show = true;
        } else {
          instr.show = false;
        }
      } else {
        instr.show = false;
      }
    }

    if (self->has(state.crewComp)) {
      auto& crew = self->get(state.crewComp);
      auto& shape = self->get(state.shapeComp);
      float t = (float)std::max(0, crew.health) / 100.0f;
      shape.color = vec4{0.2f + 0.6f * (1.0f - t), 0.2f + 0.6f * t, 0.2f, 1.0f};
      shape.flash = crew.hitAnim;
      crew.hitAnim *= (1.0f - 2.0f * dt);
    }

    trans.position = vec3{task.pos(0), 0, task.pos(1)};
    auto pWall = mMap->nextWall(task.pos);
    vec3 p0 = trans.position;
    vec3 p1 = vec3{pWall(0), 0, pWall(1)};
    auto m = mth::look_at_matrix(p1, p0, vec3{0,1,0});
    trans.rotation = mth::rotation_from_matrix(m);
  }
private:
  Map* mMap;
  input::key_observer mUseKey;
  std::vector<std::string> mSounds;
  std::size_t mNextSound = 0;
};

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

inline geometry::mesh
plantMesh() {
  geometry::mesh out;
  out.lines = false;

  float a = 0.2f; // stem
  float b = 0.2f; // mid
  float c = 0.4f;
  auto idx = out.vd.size();
  vec3 norm{0,0,0};
  vec4 color{0.1f, 0.7f, 0.1f, 1.0f};
  out.vd.push_back({ vec3{ 0, a, 0 }, norm, color });
  out.vd.push_back({ vec3{ b, a + b, 0 }, norm, color });
  out.vd.push_back({ vec3{ 0, a + b, -b }, norm, color });
  out.vd.push_back({ vec3{ 0, a + b, -b }, norm, color });
  out.vd.push_back({ vec3{ b, a + b, 0 }, norm, color });
  out.vd.push_back({ vec3{ c, a, -c }, norm, color });
  out.vd.push_back({ vec3{ 0, a, 0 }, norm, color });
  out.vd.push_back({ vec3{ 0, a + b, -b }, norm, color });
  out.vd.push_back({ vec3{ b, a + b, 0 }, norm, color });
  out.vd.push_back({ vec3{ 0, a + b, -b }, norm, color });
  out.vd.push_back({ vec3{ c, a, -c }, norm, color });
  out.vd.push_back({ vec3{ b, a + b, 0 }, norm, color });

  out.vd.push_back({ vec3{ -0, a, 0 }, norm, color });
  out.vd.push_back({ vec3{ -b, a + b, 0 }, norm, color });
  out.vd.push_back({ vec3{ -0, a + b, b }, norm, color });
  out.vd.push_back({ vec3{ -0, a + b, b }, norm, color });
  out.vd.push_back({ vec3{ -b, a + b, 0 }, norm, color });
  out.vd.push_back({ vec3{ -c, a, c }, norm, color });
  out.vd.push_back({ vec3{ -0, a, 0 }, norm, color });
  out.vd.push_back({ vec3{ -0, a + b, b }, norm, color });
  out.vd.push_back({ vec3{ -b, a + b, 0 }, norm, color });
  out.vd.push_back({ vec3{ -0, a + b, b }, norm, color });
  out.vd.push_back({ vec3{ -c, a, c }, norm, color });
  out.vd.push_back({ vec3{ -b, a + b, 0 }, norm, color });

  for (std::size_t i = idx; i < out.vd.size(); ++i) {
    out.id.push_back(i);
  }
  out.calculateNormals();
  
  return geometry::mesh::merge(out, geometry::generate_cylinder(0.15f, 0.2f, 8, vec4{0.5f, 0.4f, 0.2f, 1.0f}));
}

#endif // FRONTEND_TASKS_HPP
