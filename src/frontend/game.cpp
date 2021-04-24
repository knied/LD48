#include "renderer.hpp"
#include "camera.hpp"
#include "sim.hpp"
#include "map.hpp"
#include "actor.hpp"
#include "player.hpp"

#include "wasm.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <cstddef>

#include <iostream>
std::ios_base::Init* _ios_init_workaround = nullptr;

class MyBehavior : public Comp::Behavior::OnUpdate
                 , public Comp::Behavior::OnContact {
public:
  virtual void onUpdate(Entity* self, float /*dt*/) override {
    auto& state = GameState::instance();
    auto& physical = self->get(state.physicalComp);
    auto& shape = self->get(state.shapeComp);
    if (physical.contacts > 0) {
      shape.color = vec4{1,0,0,1};
    } else {
      shape.color = vec4{1,1,1,1};
    }
    physical.contacts = 0;
  }
  virtual void onContact(Entity* self, Entity* other) override {
    auto& state = GameState::instance();
    auto& physical = self->get(state.physicalComp);
    physical.contacts++;

    if (other->has(state.shapeComp)) {
      auto& shape = other->get(state.shapeComp);
      shape.color = vec4{1,0,1,1};
    }
  }
};

class Game final {
public:
  Game(gl::context ctx)
    : mMap(10, 10)
    , mPlayerBehavior(&mMap)
    , mRenderer(ctx)
      //, mCamera(vec3{5,3,5})
    , mSim() {
    printf("C Game\n");
    fflush(stdout);
    //auto sphere = mRenderer.createDrawable(
    //  geometry::generate_sphere(1.0f, 16, vec4{1,1,1,1}, true));
    
    auto& state = GameState::instance();
    /*for (int i = 0; i < 5; ++i) {
      auto e = state.scene.spawn();
      auto& trans = e->add(state.transComp);
      trans.position = vec3{3.0f * i,0,0};
      auto& shape = e->add(state.shapeComp);
      shape.color = vec4{0.2f + 0.2f * i, 1.0f, 1.0f, 1.0f};
      shape.drawable = sphere;
      e->add(state.physicalComp);
      //auto& behavior = e->add(state.behaviorComp);
      //behavior.onUpdateHandler = &mMyBehavior;
      //behavior.onContactHandler = &mMyBehavior;
      mEntities.push_back(e);
    }

    {
      auto e = state.scene.spawn();
      auto& trans = e->add(state.transComp);
      trans.position = vec3{0,-2,-5};
      trans.parent = mCamera.entity();
      auto& shape = e->add(state.shapeComp);
      shape.drawable = sphere;
      auto& physical = e->add(state.physicalComp);
      physical.type = Comp::Physical::Collider;
      auto& behavior = e->add(state.behaviorComp);
      behavior.onUpdateHandler = &mMyBehavior;
      behavior.onContactHandler = &mMyBehavior;
      mEntities.push_back(e);
      }*/


    { // follow camera
        auto e = state.scene.spawn();
        e->add(state.transComp);
        e->add(state.cameraComp);
        auto& behavior = e->add(state.behaviorComp);
        behavior.onUpdateHandler = &mPlayerCameraBehavior;
        mEntities.push_back(e);
        state.camera = e;
    }
    
    { // player
      auto drawable = mRenderer.createDrawable(actorMesh());
      auto e = state.scene.spawn();
      e->add(state.transComp);
      auto& shape = e->add(state.shapeComp);
      shape.drawable = drawable;
      shape.color = vec4{0.2f, 0.2f, 0.8f, 1.0f};
      auto& behavior = e->add(state.behaviorComp);
      behavior.onUpdateHandler = &mPlayerBehavior;
      //behavior.onContactHandler = &mPlayerBehavior;
      auto& actor = e->add(state.actorComp);
      (void)actor;
      mEntities.push_back(e);
      state.player = e;
    }

    { // map
      auto map = mRenderer.createDrawable(mMap.mesh());
      auto e = state.scene.spawn();
      e->add(state.transComp);
      auto& shape = e->add(state.shapeComp);
      shape.drawable = map;
      mEntities.push_back(e);
    }
  }
  ~Game() {
    printf("D Game\n");
    fflush(stdout);
  }
  int render(float dt, unsigned int width, unsigned int height) {
    auto& state = GameState::instance();
    //mCamera.update(dt);
    mSim.update(dt);
    for (auto e : state.scene.with(state.behaviorComp)) {
      auto& behavior = e->get(state.behaviorComp);
      if (behavior.onUpdateHandler != nullptr) {
        behavior.onUpdateHandler->onUpdate(e, dt);
      }
    }
    mRenderer.render(width, height);
    return 0;
  }
private:
  Map mMap;
  PlayerBehavior mPlayerBehavior;
  PlayerCameraBehavior mPlayerCameraBehavior;
  MyBehavior mMyBehavior;
  Renderer mRenderer;
  //DebugCamera mCamera;
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
