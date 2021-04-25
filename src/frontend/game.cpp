#include "renderer.hpp"
#include "camera.hpp"
#include "sim.hpp"
#include "map.hpp"
#include "actor.hpp"
#include "player.hpp"
#include "alien.hpp"
#include "projectile.hpp"
#include "trigger.hpp"
#include "tasks.hpp"

#include "wasm.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <cstddef>

#include <iostream>
std::ios_base::Init* _ios_init_workaround = nullptr;

WASM_IMPORT("overlay", "set_text")
int overlay_set_text(char const* text);

class Game final {
public:
  void commonInit(vec2 const& playerPos) {
    auto& state = GameState::instance();
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
      auto e = state.scene.spawn();
      e->add(state.transComp);
      auto& shape = e->add(state.shapeComp);
      shape.drawable = mActorDrawable.get();
      shape.color = vec4{0.2f, 0.2f, 0.8f, 1.0f};
      auto& behavior = e->add(state.behaviorComp);
      behavior.onUpdateHandler = &mPlayerBehavior;
      behavior.onTriggerHandler = &mPlayerBehavior;
      auto& actor = e->add(state.actorComp);
      actor.pos = playerPos;
      mEntities.push_back(e);
      state.player = e;
    }

    for (int i = 0; i < 10; ++i) { // projectiles
      auto e = state.scene.spawn();
      e->add(state.transComp);
      auto& shape = e->add(state.shapeComp);
      shape.drawable = mProjectileDrawable.get();
      shape.color = vec4{0.9f, 1.0f, 0.7f, 1.0f};
      auto& behavior = e->add(state.behaviorComp);
      behavior.onUpdateHandler = &mProjectileBehavior;
      auto& projectile = e->add(state.projectileComp);
      projectile.damage = 0; // inactive
      state.projectiles.push_back(e);
      mEntities.push_back(e);
    }
  }

  void spawnAlien(vec2 const& pos) {
    auto& state = GameState::instance();
    auto e = state.scene.spawn();
    e->add(state.transComp);
    auto& shape = e->add(state.shapeComp);
    shape.drawable = mActorDrawable.get();
    shape.color = vec4{0.8f, 0.2f, 0.2f, 1.0f};
    auto& behavior = e->add(state.behaviorComp);
    behavior.onUpdateHandler = &mAlienBehavior;
    auto& actor = e->add(state.actorComp);
    actor.faction = 1;
    actor.pos = pos;
    mEntities.push_back(e);
  }

  void spawnConsole(vec2 const& pos) {
    auto& state = GameState::instance();
    auto e = state.scene.spawn();
    auto& trans = e->add(state.transComp);
    trans.position = vec3{pos(0), 0.0f, pos(1)};
    auto& shape = e->add(state.shapeComp);
    shape.drawable = mConsoleDrawable.get();
    shape.color = vec4{0.2f, 0.2f, 0.2f, 1.0f};
    mEntities.push_back(e);
  }

  void initLevel(int level) {
    mLevelComplete = false;
    mGameOver = false;
    overlay_set_text("");
    auto& state = GameState::instance();
    state.reset();
    { // map
      mMapDrawable = mRenderer.createDrawable(mMap.mesh());
      auto e = state.scene.spawn();
      e->add(state.transComp);
      auto& shape = e->add(state.shapeComp);
      shape.drawable = mMapDrawable.get();
      mEntities.push_back(e);
    }

    { // charger
      mChargerDrawable = mRenderer.createDrawable(geometry::generate_cylinder(0.4f, 0.1f, 12, vec4{0.8f, 0.8f, 1.0f, 1.0f}));
      auto e = state.scene.spawn();
      e->add(state.transComp);
      auto& shape = e->add(state.shapeComp);
      shape.drawable = mChargerDrawable.get();
      auto& trigger = e->add(state.triggerComp);
      trigger.pos = mMap.pointOfInterest('C');
      auto& behavior = e->add(state.behaviorComp);
      behavior.onUpdateHandler = &mTriggerBehavior;
    }

    switch (level) {
    case 0: {
      // Nothing to do.
      // Player navigates to charging station
      commonInit(mMap.pointOfInterest('0'));
      spawnConsole(mMap.pointOfInterest('a'));
      spawnConsole(mMap.pointOfInterest('b'));
    } break;
    case 1: {
      // Invaders! Kill alien.
      // Player navigates to charging station
      commonInit(mMap.pointOfInterest('C'));
      spawnAlien(mMap.pointOfInterest('1'));
      spawnAlien(mMap.pointOfInterest('5'));
      spawnAlien(mMap.pointOfInterest('6'));
    } break;
    default: {
      // Nothing to do.
      // Player navigates to charging station
      commonInit(vec2{1,1});
      mGameOver = true;
      overlay_set_text("<h1>You did it!</h1><h2>Click to Restart</h2>");
    }
    }
  }

  bool checkObjectives() {
    auto& state = GameState::instance();
    auto& playerActor = state.player->get(state.actorComp);
    if (playerActor.health <= 0) {
      if (!mGameOver) {
        overlay_set_text("<h1>The Ship is Lost!</h1><h2>Click to Restart</h2>");
      }
      mGameOver = true;
      return false;
    }
    
    bool tasksDone = true;
    // Are all aliens dead?
    for (auto e : state.scene.with(state.actorComp)) {
      auto& actor = e->get(state.actorComp);
      if (actor.faction != 0 && actor.health > 0) {
        tasksDone = false;
      }
    }
    
    return tasksDone && state.playerOnCharger;
  }
  
  Game(gl::context ctx)
    : mMap(24, 24, "\
########   ab   ########\
########  0     ########\
###    #        #    ###\
### 9  # ###### #    ###\
###      #    #    7 ###\
###    # # 8  # #    ###\
######## #    # ########\
######## #### # ########\
#                      #\
## ### ########## ### ##\
#   ##  ########  ##6  #\
#C  ##  ########  ##   #\
###### ########## ######\
######  ########  ######\
######  ########  ######\
###### ########## ######\
######  ########  ######\
######  ########  ######\
######  ########  ######\
###### ########## ######\
#             4        #\
#      2          5    #\
#  1       3           #\
##   ###   ##   ###   ##\
")
    , mPlayerBehavior(&mMap)
    , mAlienBehavior(&mMap)
    , mProjectileBehavior(&mMap)
    , mRenderer(ctx)
    , mActorDrawable(mRenderer.createDrawable(actorMesh()))
    , mProjectileDrawable(mRenderer.createDrawable(geometry::generate_sphere(0.05f, 5, vec4{1,1,1,1})))
    , mConsoleDrawable(mRenderer.createDrawable((consoleMesh()))) {
    printf("C Game\n");
    fflush(stdout);
    initLevel(mLevel);
    overlay_set_text("<h1>Click to Start</h1>");
  }
  ~Game() {
    printf("D Game\n");
    fflush(stdout);
  }
  void onLostFocus() {
    overlay_set_text("<h1>Paused</h1><h2>Click to Continue</h2>");
  }
  void onGainedFocus() {
    overlay_set_text("");
  }
  int render(float dt, unsigned int width, unsigned int height, bool focus) {
    if (focus && !mFocus) {
      onGainedFocus();
    }
    if (!focus && mFocus) {
      onLostFocus();
    }
    mFocus = focus;

    if (mGameOver && mMouse.mousedownMain() > 0) {
      mLevel = 0;
      initLevel(mLevel);
    }
    if (mLevelComplete && mMouse.mousedownMain() > 0) {
      initLevel(++mLevel);
    }
    
    if (mFocus && !mGameOver) {
      auto& state = GameState::instance();
      for (auto e : state.scene.with(state.behaviorComp)) {
        auto& behavior = e->get(state.behaviorComp);
        if (behavior.onUpdateHandler != nullptr) {
          behavior.onUpdateHandler->onUpdate(e, dt);
        }
      }
      if (checkObjectives()) {
        // Level complete
        if (!mLevelComplete) {
          overlay_set_text("<h1>Good Work!</h1><h2>Click to go to Standby</h2>");
        }
        mLevelComplete = true;
      } else {
        if (mLevelComplete) {
          overlay_set_text("");
        }
        mLevelComplete = false;
      }
    }
    mRenderer.render(width, height);
    return 0;
  }
private:
  Map mMap;
  PlayerBehavior mPlayerBehavior;
  AlienBehavior mAlienBehavior;
  ProjectileBehavior mProjectileBehavior;
  TriggerBehavior mTriggerBehavior;
  PlayerCameraBehavior mPlayerCameraBehavior;
  Renderer mRenderer;
  std::unique_ptr<Drawable> mActorDrawable;
  std::unique_ptr<Drawable> mProjectileDrawable;
  std::unique_ptr<Drawable> mMapDrawable;
  std::unique_ptr<Drawable> mChargerDrawable;
  std::unique_ptr<Drawable> mConsoleDrawable;
  std::vector<Entity*> mEntities;
  int mLevel = 0;
  bool mFocus = false;
  bool mGameOver = false;
  bool mLevelComplete = false;
  input::mouse_observer mMouse;
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
int render(void* udata, float dt, unsigned int width, unsigned int height, bool focus) {
  return reinterpret_cast<Game*>(udata)->render(dt, width, height, focus);
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
