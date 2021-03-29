#include "wasm.h"
#include "websocket.hpp"
#include "graphics.hpp"
#include <stdio.h>
#include <string>
#include <vector>
#include <cstddef>

#include "../common/mth.hpp"

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

using vec3 = mth::vector<float,3>;
using mat4 = mth::matrix<float,4,4>;

class Game final {
public:
  struct vertex { vec3 pos; };
  struct uniform { vec3 color; mat4 mat; };
  
  Game(gl::context ctx)
    : mCtx(ctx), mCommandBuffer(mCtx) {
    printf("C Game\n");
    fflush(stdout);
    mPipeline = std::unique_ptr<gl::pipeline>(new gl::pipeline(mCtx, "\
attribute vec4 a_position;\n\
uniform mat4 u_mat;\n\
void main() {\n\
  gl_Position = u_mat * a_position;\n\
}\n","\
precision mediump float;\n\
uniform vec3 u_color;\n\
void main() {\n\
  gl_FragColor = vec4(u_color, 1.0);\n\
}\n", {"a_position"}, {"u_color", "u_mat"}));

    {
      std::vector<vertex> vd {
        { vec3{ -1.0f, -1.0f, 0.0f } },
        { vec3{ 1.0f, -1.0f, 0.0f } },
        { vec3{ 0.0f, 1.0f, 0.0f } }
      };
      std::vector<unsigned int> id { 0, 1, 2 };
      std::vector<gl::attribute_def> defs {
        { "a_position", sizeof(vertex), 0, 3, gl::ATTRIBUTE_FLOAT, false }
      };
      mMesh = std::make_unique<gl::mesh>(mCtx, gl::PRIMITIVE_TRIANGLES, vd, id, defs);
    }

    mMeshBinding = std::make_unique<gl::mesh_binding>(mCtx, mMesh.get(), mPipeline.get());

    {
      std::vector<gl::uniform_def> defs {
        { "u_color", gl::UNIFORM_VF3, offsetof(uniform, color) },
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
    fflush(stdout);
    mCommandBuffer.set_viewport(0, 0, width, height);
    mCommandBuffer.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    mCommandBuffer.clear(gl::CLEAR_COLOR);
    mCommandBuffer.set_pipeline(mPipeline.get());
    mCommandBuffer.set_mesh(mMeshBinding.get());

    auto znear = 0.1f;
    auto zfar = 1000.0f;
    auto fov = 90.0f;
    auto projection = mth::perspective_projection<float>(width, height,
                                                         fov, znear, zfar);
    auto view = mth::inverse(mth::translation(vec3{0.0f, 0.0f, 10.0f}));
    auto model = mth::identity<float,4,4>();
    static auto anim = 0.0f;
    anim += dt;
    while (anim > 1.0f) anim -= 1.0f;
    model = model * mth::rotation(mth::from_axis(vec3{0.0f,1.0f,0.0f}, 2.0f * anim * (float)mth::pi));
    auto mvp = projection * view * model;
    
    uniform u{
      vec3{ 0.8f, 0.2f, 0.2f },
      mth::transpose(mvp)
    };
    mCommandBuffer.set_uniforms(mUniformBinding.get(), u);
    mCommandBuffer.draw();
    mCommandBuffer.commit();
    return 0;
  }
private:
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
