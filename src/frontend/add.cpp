#include "wasm.h"
#include "input.hpp"
#include "websocket.hpp"
#include "graphics.hpp"
#include "geometry.hpp"
#include <stdio.h>
#include <string>
#include <vector>
#include <cstddef>

#include <common/mth.hpp>
#include <common/ecs.hpp>
#include <common/gjk.hpp>

class MySocket : public WebSocket {
public:
  MySocket(char const* url) : WebSocket(url) {}
  virtual void onopen() {
    printf("onopen\n");
    fflush(stdout);
    std::string msg("WASM sent me!!!");
    send(msg.c_str(), msg.size());
  }
  virtual void onclose() {
    printf("onclose\n");
    fflush(stdout);
  }
  virtual void onerror(char const* buffer, unsigned int size) {
    std::string msg(buffer, size);
    printf("onerror: %s\n", msg.c_str());
    fflush(stdout);
  }
  virtual void onmessage(char const* buffer, unsigned int size) {
    std::string msg(buffer, size);
    printf("onmessage: %s\n", msg.c_str());
    fflush(stdout);
  }
};

using vec2 = mth::vector<float,2>;
using vec3 = mth::vector<float,3>;
using vec4 = mth::vector<float,4>;
using mat4 = mth::matrix<float,4,4>;
using rot3 = mth::quaternion<float>;

template<typename T, unsigned int R, unsigned int C>
static constexpr inline unsigned int
entry_count(mth::matrix<T, R, C> const&) noexcept {
  return R * C;
}
template<typename T, unsigned int R, unsigned int C>
static constexpr inline gl::attribute_type
entry_gl_type(mth::matrix<T, R, C> const&) noexcept {
  return gl::atype<T>();
}

struct uniform {
  mat4 mat;
  vec4 color;
};

static std::vector<gl::attribute_def>
vertex_defs() {
  geometry::vertex tmp;
  return {
    { "a_position", sizeof(geometry::vertex), offsetof(geometry::vertex, pos), entry_count(tmp.pos), entry_gl_type(tmp.pos), false },
    { "a_color", sizeof(geometry::vertex), offsetof(geometry::vertex, color), entry_count(tmp.color), entry_gl_type(tmp.color), false }
  };
}

struct InputBuffer {
  int movementX, movementY;
  int clientX, clientY;
};

using Scene = ecs::scene;
using Entity = ecs::entity;

namespace Comp {

struct Camera {
  float znear = 0.1f;
  float zfar = 1000.0f;
  float fov = 70.0f;
};

struct Shape {
  vec4 color = vec4{1,1,1,1};
};

struct Physical {
  int contacts = 0;
  float radius = 1.0f;
};

struct Transformation {
  vec3 position = vec3{0,0,0};
  rot3 rotation = mth::from_euler(0.0f, 0.0f, 0.0f);
  Entity* parent = nullptr;
  mat4 local() const {
    return mth::transformation(rotation, position);
  }
};

} // namespace Comp

using CameraComponent = ecs::component<Comp::Camera>;
using ShapeComponent = ecs::component<Comp::Shape>;
using PhysicalComponent = ecs::component<Comp::Physical>;
using TransformationComponent = ecs::component<Comp::Transformation>;

struct GameState {
  ecs::scene scene;
  CameraComponent* cameraComp;
  ShapeComponent* shapeComp;
  PhysicalComponent* physicalComp;
  TransformationComponent* transComp;

  GameState() {
    cameraComp = scene.create_component<Comp::Camera>();
    shapeComp = scene.create_component<Comp::Shape>();
    physicalComp = scene.create_component<Comp::Physical>();
    transComp = scene.create_component<Comp::Transformation>();
  }
};

static mat4
world(Entity* e, TransformationComponent* transComp) {
  mat4 result = mth::identity<float,4,4>();
  while (e != nullptr) {
    if (e->has(transComp)) {
      auto& trans = e->get(transComp);
      result = trans.local() * result;
      e = trans.parent;
    } else {
      e = nullptr;
    }
  }
  return result;
}

class Sim final {
public:
  Sim(GameState* state)
    : mState(state) {}
  void update(float dt) {
    (void)dt;

    auto mask = ecs::fingerprint(mState->transComp, mState->physicalComp);
    for (auto [e0, e1] : mState->scene.pairs_with(mask, mask)) {
      auto m0 = world(e0, mState->transComp);
      auto& physical0 = e0->get(mState->physicalComp);
      gjk::sphere c0{ physical0.radius };
      auto m1 = world(e1, mState->transComp);
      auto& physical1 = e1->get(mState->physicalComp);
      gjk::sphere c1{ physical1.radius };
      gjk::transformed_convex c1_(mth::inverse_transformation(m0) * m1, &c1);
      gjk::simplex simplex;
      if (gjk::gjk(c0, c1_, simplex)) {
        physical0.contacts++;
        physical1.contacts++;
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
  }

private:
  GameState* mState;
};

class DebugCamera final {
public:
  DebugCamera(GameState* state)
    : mState(state)
    , mAngle{0,0}
    , mPos{0,0}
      //, mModel(mth::identity<float,4,4>())
    , mUpKey("KeyW")
    , mDownKey("KeyS")
    , mLeftKey("KeyA")
    , mRightKey("KeyD") {
    mEntity = mState->scene.spawn();
    mEntity->add(mState->cameraComp);
    mEntity->add(mState->transComp);
  }
  ~DebugCamera() {
    mState->scene.despawn(mEntity);
  }
  void update(float dt) {
    auto drot = 0.25f * dt * vec2{
      (float)mMouse.movementX(),
      (float)mMouse.movementY()
    };
    mAngle -= drot;
    while (mAngle(0) >= 2 * mth::pi) {
      mAngle(0) -= 2 * mth::pi;
    }
    if (mAngle(1) < -0.5f * mth::pi) {
      mAngle(1) = -0.5f * mth::pi;
    }
    if (mAngle(1) > 0.5f * mth::pi) {
      mAngle(1) = 0.5f * mth::pi;
    }
    auto& trans = mEntity->get(mState->transComp);
    trans.rotation = mth::from_euler(mAngle(1), mAngle(0), 0.0f);
    
    //mModel = mth::rotation();
    int frontBack = mUpKey.pressed() - mDownKey.pressed();
    int leftRight = mRightKey.pressed() - mLeftKey.pressed();
    if (frontBack != 0 || leftRight != 0) {
      auto dmove = 4.0f * dt * mth::normal(vec2{
          (float)leftRight,
          (float)frontBack
        });
      //mPos += (mth::baseX(mModel) * dmove(0) - mth::baseZ(mModel) * dmove(1));
      auto baseX = mth::transform(trans.rotation, vec3{1,0,0});
      auto baseZ = mth::transform(trans.rotation, vec3{0,0,1});
      mPos += (baseX * dmove(0) - baseZ * dmove(1));
    }
    trans.position = mPos;
    //mModel = mth::translation(mPos) * mModel;
  }
  Entity* entity() const {
    return mEntity;
  }
  /*auto model() const {
    return mModel;
    }*/
private:
  GameState* mState;
  Entity* mEntity;
  vec2 mAngle;
  vec3 mPos;
  //mat4 mModel;
  input::key_observer mUpKey, mDownKey, mLeftKey, mRightKey;
  input::mouse_observer mMouse;
};

class Renderer final {
public:
  Renderer(gl::context ctx, GameState* state)
    : mCtx(ctx)
    , mState(state)
    , mCommandBuffer(mCtx) {
    mPipeline = std::unique_ptr<gl::pipeline>(new gl::pipeline(mCtx, "\
attribute vec4 a_position;\n\
attribute vec4 a_color;\n\
uniform mat4 u_mat;\n\
uniform vec4 u_color;\n\
varying vec4 v_color;\n\
void main() {\n\
  gl_Position = u_mat * a_position;\n\
  v_color = a_color * u_color;\n\
}\n","\
precision mediump float;\n\
varying vec4 v_color;\n\
void main() {\n\
  gl_FragColor = v_color;\n\
}\n", {"a_position", "a_color"}, {"u_mat", "u_color"}));
    //auto mesh = geometry::generate_terrain(20, 20, vec4{1,1,1,1}, true);
    auto mesh = geometry::generate_sphere(1.0f, 16, vec4{1,1,1,1}, true);
    //auto mesh = geometry::generate_box(1.0f, 2.0f, 3.0f, vec4{1,1,1,1}, true);
    mMesh = std::make_unique<gl::mesh>(mCtx, gl::PRIMITIVE_LINES, mesh.vd, mesh.id, vertex_defs());
    mMeshBinding = std::make_unique<gl::mesh_binding>(mCtx, mMesh.get(), mPipeline.get());
    std::vector<gl::uniform_def> defs {
      { "u_mat", gl::UNIFORM_MF4, offsetof(uniform, mat) },
      { "u_color", gl::UNIFORM_VF4, offsetof(uniform, color) }
    };
    mUniformBinding = std::make_unique<gl::uniform_binding>(mCtx, mPipeline.get(), sizeof(uniform), defs);
  }
  void render(unsigned int width, unsigned int height, Entity* cam) {
    mCommandBuffer.set_viewport(0, 0, width, height);
    mCommandBuffer.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    mCommandBuffer.clear(gl::CLEAR_COLOR);
    mCommandBuffer.set_pipeline(mPipeline.get());

    assert(cam->has(mState->cameraComp));
    assert(cam->has(mState->transComp));
    auto const& camera = cam->get(mState->cameraComp);
    auto projection
      = mth::perspective_projection<float>(width, height, camera.fov,
                                           camera.znear, camera.zfar);
    auto view = mth::inverse(world(cam, mState->transComp));
    auto vp = projection * view;
    for (auto e : mState->scene.with(mState->shapeComp, mState->transComp)) {
      auto const& shape = e->get(mState->shapeComp);
      uniform u{
        mth::transpose(vp * world(e, mState->transComp)),
        shape.color
      };
      mCommandBuffer.set_mesh(mMeshBinding.get());
      mCommandBuffer.set_uniforms(mUniformBinding.get(), u);
      mCommandBuffer.draw();
    }
    mCommandBuffer.commit();
  }
private:
  gl::context mCtx;
  GameState* mState;
  std::unique_ptr<gl::pipeline> mPipeline;
  std::unique_ptr<gl::mesh> mMesh;
  std::unique_ptr<gl::mesh_binding> mMeshBinding;
  std::unique_ptr<gl::uniform_binding> mUniformBinding;
  gl::command_buffer mCommandBuffer;
};

class Game final {
public:
  Game(gl::context ctx)
    : mRenderer(ctx, &mState)
    , mCamera(&mState)
    , mSim(&mState) {
    printf("C Game\n");
    fflush(stdout);

    for (int i = 0; i < 5; ++i) {
      auto e = mState.scene.spawn();
      auto& trans = e->add(mState.transComp);
      trans.position = vec3{3.0f * i,0,0};
      auto& shape = e->add(mState.shapeComp);
      shape.color = vec4{0.2f + 0.2f * i, 1.0f, 1.0f, 1.0f};
      e->add(mState.physicalComp);
      mEntities.push_back(e);
    }

    {
      auto e = mState.scene.spawn();
      auto& trans = e->add(mState.transComp);
      trans.position = vec3{0,-2,-5};
      trans.parent = mCamera.entity();
      e->add(mState.shapeComp);
      e->add(mState.physicalComp);
      mEntities.push_back(e);
    }
  }
  ~Game() {
    printf("D Game\n");
    fflush(stdout);
  }
  int render(float dt, unsigned int width, unsigned int height) {
    mCamera.update(dt);
    mSim.update(dt);
    mRenderer.render(width, height, mCamera.entity());
    return 0;
  }
private:
  GameState mState;
  Renderer mRenderer;
  DebugCamera mCamera;
  Sim mSim;
  std::vector<Entity*> mEntities;
};

WASM_EXPORT("init")
void* init(gl::context ctx) {
  return new Game(ctx);
}
WASM_EXPORT("release")
void release(void* udata) {
  delete reinterpret_cast<Game*>(udata);
}
WASM_EXPORT("render")
int render(void* udata, float dt, unsigned int width, unsigned int height) {
  return reinterpret_cast<Game*>(udata)->render(dt, width, height);
}

WebSocket* gSocket = nullptr;

WASM_EXPORT("add")
double add(double a, double b) {
  gSocket = new MySocket("/whatever");
  return 2.0f * a + b;
}

WASM_EXPORT("allocate_buffer")
char* allocate_buffer(std::size_t size) {
  return (char*)malloc(size);
}
WASM_EXPORT("free_buffer")
void free_buffer(char* buffer) {
  free(buffer);
}
