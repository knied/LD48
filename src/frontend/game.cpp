#include "renderer.hpp"
#include "camera.hpp"
#include "sim.hpp"

#include "wasm.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <cstddef>

#include <iostream>
std::ios_base::Init* _ios_init_workaround = nullptr;

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
      auto& physical = e->add(mState.physicalComp);
      physical.type = Comp::Physical::Collider;
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
  _ios_init_workaround = new std::ios_base::Init;
  return new Game(ctx);
}
WASM_EXPORT("release")
void release(void* udata) {
  delete reinterpret_cast<Game*>(udata);
  delete _ios_init_workaround;
}
WASM_EXPORT("render")
int render(void* udata, float dt, unsigned int width, unsigned int height) {
  return reinterpret_cast<Game*>(udata)->render(dt, width, height);
}

/*WebSocket* gSocket = nullptr;

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
  }*/
