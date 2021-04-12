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

struct uniform { mat4 mat; };

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

class DebugCamera final {
public:
  DebugCamera()
    : mAngle{0,0}
    , mPos{0,0}
    , mModel(mth::identity<float,4,4>())
    , mUpKey("KeyW")
    , mDownKey("KeyS")
    , mLeftKey("KeyA")
    , mRightKey("KeyD") {}
  ~DebugCamera() {}
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
    mModel = mth::identity<float,4,4>();
    mModel = mModel * mth::rotation(mth::from_axis(vec3{0.0f,1.0f,0.0f}, mAngle(0)));
    mModel = mModel * mth::rotation(mth::from_axis(vec3{1.0f,0.0f,0.0f}, mAngle(1)));
    int frontBack = mUpKey.pressed() - mDownKey.pressed();
    int leftRight = mRightKey.pressed() - mLeftKey.pressed();
    if (frontBack != 0 || leftRight != 0) {
      auto dmove = 4.0f * dt * mth::normal(vec2{
          (float)leftRight,
          (float)frontBack
        });
      mPos += (mth::baseX(mModel) * dmove(0) - mth::baseZ(mModel) * dmove(1));
    }
    mModel = mth::translation(mPos) * mModel;
  }
  auto model() const {
    return mModel;
  }
private:
  vec2 mAngle;
  vec3 mPos;
  mat4 mModel;
  input::key_observer mUpKey, mDownKey, mLeftKey, mRightKey;
  input::mouse_observer mMouse;
};

class Game final {
public:
  Game(gl::context ctx)
    : mCtx(ctx)
    , mCommandBuffer(mCtx) {
    printf("C Game\n");
    fflush(stdout);
    mPipeline = std::unique_ptr<gl::pipeline>(new gl::pipeline(mCtx, "\
attribute vec4 a_position;\n\
attribute vec4 a_color;\n\
uniform mat4 u_mat;\n\
varying vec4 v_color;\n\
void main() {\n\
  gl_Position = u_mat * a_position;\n\
  v_color = a_color;\n\
}\n","\
precision mediump float;\n\
varying vec4 v_color;\n\
void main() {\n\
  gl_FragColor = v_color;\n\
}\n", {"a_position", "a_color"}, {"u_mat"}));

    {
      /*vec4 red{ 1.0f, 0.0f, 0.0f, 1.0f };
      vec4 green{ 0.0f, 1.0f, 0.0f, 1.0f };
      vec4 blue{ 0.0f, 0.0f, 1.0f, 1.0f };
      vec4 yellow{ 1.0f, 1.0f, 0.0f, 1.0f };
      vec4 cyan{ 0.0f, 1.0f, 1.0f, 1.0f };
      vec4 pink{ 1.0f, 0.0f, 1.0f, 1.0f };
      std::vector<vertex> vd {
        // front
        { vec3{ -1.0f, -1.0f, -1.0f }, red },
        { vec3{ 1.0f, -1.0f, -1.0f }, red },
        { vec3{ 1.0f, 1.0f, -1.0f }, red },
        { vec3{ -1.0f, -1.0f, -1.0f }, red },
        { vec3{ 1.0f, 1.0f, -1.0f }, red },
        { vec3{ -1.0f, 1.0f, -1.0f }, red },
        // back
        { vec3{ -1.0f, -1.0f, 1.0f }, green },
        { vec3{ 1.0f, 1.0f, 1.0f }, green },
        { vec3{ 1.0f, -1.0f, 1.0f }, green },
        { vec3{ -1.0f, -1.0f, 1.0f }, green },
        { vec3{ -1.0f, 1.0f, 1.0f }, green },
        { vec3{ 1.0f, 1.0f, 1.0f }, green },
        // top
        { vec3{ -1.0f, 1.0f, -1.0f }, blue },
        { vec3{ 1.0f, 1.0f, -1.0f }, blue },
        { vec3{ 1.0f, 1.0f, 1.0f }, blue },
        { vec3{ -1.0f, 1.0f, -1.0f }, blue },
        { vec3{ 1.0f, 1.0f, 1.0f }, blue },
        { vec3{ -1.0f, 1.0f, 1.0f }, blue },
        // bottom
        { vec3{ -1.0f, -1.0f, -1.0f }, yellow },
        { vec3{ 1.0f, -1.0f, 1.0f }, yellow },
        { vec3{ 1.0f, -1.0f, -1.0f }, yellow },
        { vec3{ -1.0f, -1.0f, -1.0f }, yellow },
        { vec3{ -1.0f, -1.0f, 1.0f }, yellow },
        { vec3{ 1.0f, -1.0f, 1.0f }, yellow },
        // left
        { vec3{ -1.0f, -1.0f, 1.0f }, cyan },
        { vec3{ -1.0f, -1.0f, -1.0f }, cyan },
        { vec3{ -1.0f, 1.0f, -1.0f }, cyan },
        { vec3{ -1.0f, -1.0f, 1.0f }, cyan },
        { vec3{ -1.0f, 1.0f, -1.0f }, cyan },
        { vec3{ -1.0f, 1.0f, 1.0f }, cyan },
        // right
        { vec3{ 1.0f, -1.0f, 1.0f }, pink },
        { vec3{ 1.0f, 1.0f, -1.0f }, pink },
        { vec3{ 1.0f, -1.0f, -1.0f }, pink },
        { vec3{ 1.0f, -1.0f, 1.0f }, pink },
        { vec3{ 1.0f, 1.0f, 1.0f }, pink },
        { vec3{ 1.0f, 1.0f, -1.0f }, pink },
      };
      std::vector<unsigned int> id;
      for (unsigned int i = 0; i < vd.size(); ++i) {
        id.push_back(i);
        }*/
      //auto mesh = geometry::generate_terrain(20, 20, vec4{1,1,1,1}, true);
      auto mesh = geometry::generate_sphere(1.0f, 16, vec4{1,1,1,1}, true);
      mMesh = std::make_unique<gl::mesh>(mCtx, gl::PRIMITIVE_LINES, mesh.vd, mesh.id, vertex_defs());
    }

    mMeshBinding = std::make_unique<gl::mesh_binding>(mCtx, mMesh.get(), mPipeline.get());

    {
      std::vector<gl::uniform_def> defs {
        { "u_mat", gl::UNIFORM_MF4, offsetof(uniform, mat) }
      };
      mUniformBinding = std::make_unique<gl::uniform_binding>(mCtx, mPipeline.get(), sizeof(uniform), defs);
    }
  }
  ~Game() {
    printf("D Game\n");
    fflush(stdout);
  }
  int render(float dt, unsigned int width, unsigned int height) {
    //printf("Game::render %f %u %u\n", dt, width, height);
    //fflush(stdout);
    mCommandBuffer.set_viewport(0, 0, width, height);
    mCommandBuffer.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    mCommandBuffer.clear(gl::CLEAR_COLOR);
    mCommandBuffer.set_pipeline(mPipeline.get());
    mCommandBuffer.set_mesh(mMeshBinding.get());

    auto znear = 0.1f;
    auto zfar = 1000.0f;
    auto fov = 80.0f;
    auto projection = mth::perspective_projection<float>(width, height,
                                                         fov, znear, zfar);
    mCamera.update(dt);
    auto view = mth::inverse(mCamera.model());
    //auto view = mth::inverse(mth::translation(vec3{0.0f, 3.0f, 5.0f}));
    auto model = mth::identity<float,4,4>();
    auto mvp = projection * view * model;
    
    uniform u{
      mth::transpose(mvp)
    };
    mCommandBuffer.set_uniforms(mUniformBinding.get(), u);
    mCommandBuffer.draw();
    mCommandBuffer.commit();
    return 0;
  }
private:
  DebugCamera mCamera;
  //input::mouse_observer mMouse;
  //input::key_observer mKey;
  gl::context mCtx;
  std::unique_ptr<gl::pipeline> mPipeline;
  std::unique_ptr<gl::mesh> mMesh;
  std::unique_ptr<gl::mesh_binding> mMeshBinding;
  std::unique_ptr<gl::uniform_binding> mUniformBinding;
  gl::command_buffer mCommandBuffer;
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
