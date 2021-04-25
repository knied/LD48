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
#include <set>

#include <iostream>
std::ios_base::Init* _ios_init_workaround = nullptr;

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
      e->add(state.instrComp);
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

  void spawnAlien(vec2 const& pos, bool targetPlayer = false) {
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
    actor.targetPlayer = targetPlayer;
    mEntities.push_back(e);
  }

  void spawnConsole(vec2 const& pos, std::string const& msg, bool complete = false) {
    auto& state = GameState::instance();
    auto e = state.scene.spawn();
    e->add(state.transComp);
    auto& shape = e->add(state.shapeComp);
    shape.drawable = mConsoleDrawable.get();
    shape.color = vec4{0.2f, 0.2f, 0.2f, 1.0f};
    auto& task = e->add(state.taskComp);
    task.complete = complete;
    task.pos = pos;
    auto& instr = e->add(state.instrComp);
    instr.msg = msg;
    auto& behavior = e->add(state.behaviorComp);
    behavior.onUpdateHandler = &mTaskBehavior;
    mEntities.push_back(e);
  }

  void spawnPlant(vec2 const& pos, std::string const& msg, bool complete = false) {
    auto& state = GameState::instance();
    auto e = state.scene.spawn();
    auto& trans = e->add(state.transComp);
    trans.position = vec3{10,0,0};
    auto& shape = e->add(state.shapeComp);
    shape.drawable = mPlantDrawable.get();
    shape.color = vec4{0.2f, 0.2f, 0.2f, 1.0f};
    auto& task = e->add(state.taskComp);
    task.complete = complete;
    task.pos = pos;
    task.plant = true;
    auto& instr = e->add(state.instrComp);
    instr.msg = msg;
    auto& behavior = e->add(state.behaviorComp);
    behavior.onUpdateHandler = &mTaskBehavior;
    mEntities.push_back(e);
  }

  void spawnCrew(vec2 const& pos, std::string const& name, int health, bool complete = false) {
    auto& state = GameState::instance();
    auto e = state.scene.spawn();
    e->add(state.transComp);
    auto& shape = e->add(state.shapeComp);
    shape.drawable = mCrewDrawable.get();
    shape.color = vec4{0.2f, 0.8f, 0.2f, 1.0f};
    shape.trans = mth::rotation(mth::from_axis(vec3{1,0,0}, -0.4f));
    auto& task = e->add(state.taskComp);
    task.complete = complete;
    task.pos = pos;
    auto& instr = e->add(state.instrComp);
    instr.msg = "Check Vitals: " + name;
    auto& behavior = e->add(state.behaviorComp);
    behavior.onUpdateHandler = &mTaskBehavior;
    auto& crew = e->add(state.crewComp);
    crew.name = name;
    crew.health = health;
    mEntities.push_back(e);
  }

  void story(int year, std::string body, bool gameOver = false) {
    commonInit(vec2{-10,0});
    mStory = "<h1>It's the Year " + std::to_string(year) + "</h1>";
    mStory += "<p>Crew: ";
    for (auto& crew : mCrew) {
      auto& name = crew.first;
      auto health = crew.second.second;
      if (health > 0) {
        mStory += name + " ";
      } else {
        mStory += "<strike>" + name + "</strike> ";
      }
    }
    mStory += "</p>";
    mStory += body;
    if (!gameOver) {
      mStory += "<p>Click to Continue</p>";
    } else {
      mStory += "<p>Click to Restart</p>";
    }
    mStoryBoard = true;
  }

  void initLevel(int level) {
    printf("initLevel %d\n", level);
    fflush(stdout);

    auto& state = GameState::instance();
    if (level == 0) {
      mCrew.clear();
      mCrew.insert(std::make_pair("Peter", std::make_pair('A', 100)));
      mCrew.insert(std::make_pair("Lucy", std::make_pair('B', 100)));
      mCrew.insert(std::make_pair("Frank", std::make_pair('C', 100)));
      mCrew.insert(std::make_pair("Mary", std::make_pair('D', 100)));
    } else {
      // persisten crew health
      for (auto t : state.scene.with(state.crewComp)) {
        auto& crew = t->get(state.crewComp);
        mCrew[crew.name].second = crew.health;
      }
    }

    mGameOver = false;
    mStoryBoard = false;
    mShowingInstructions = false;
    game_set_text(0, "");
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
      trigger.pos = mMap.pointOfInterest('H');
      auto& behavior = e->add(state.behaviorComp);
      behavior.onUpdateHandler = &mTriggerBehavior;
    }

    std::set<int> tasks;
    switch (level) {
    case 0: {
      commonInit(vec2{-10,0});
      mStory = "<h1>Deep Space Janitor</h1><p>You are the maintenance robot on a deep space mission.<br>A game for Ludum Dare 48.</p><p>Use W,A,S,D and your mouse to move around. Left mouse button to shoot.</p><p>Click to Start</p>";
      mStoryBoard = true;
    } break;
    case 1: {
      story(2048, "<p>All systems Go! Report at your charging station so we can start our journey into deep space.<br>Take good care of the sleeping crew!</p>");
    } break;
    case 2: {
      // Nothing to do.
      // Player navigates to charging station
      commonInit(mMap.pointOfInterest('0'));

      // testing
      //spawnAlien(mMap.pointOfInterest('1'));
      //spawnAlien(mMap.pointOfInterest('5'));
      //spawnAlien(mMap.pointOfInterest('6'));
    } break;
    case 3: {
      story(2122, "<p>Time for some chores!</p>");
    } break;
    case 4: {
      // Player does tasks
      // Player navigates to charging station
      commonInit(mMap.pointOfInterest('H'));
      tasks.insert(0);
      tasks.insert(1);
      tasks.insert(2);
    } break;
    case 5: {
      game_play_sound("danger");
      story(2136, "<h2>Intruders!</h2><p>Quickly find and defeat the alien robots!</p>");
    } break;
    case 6: {
      // Invaders! Kill alien.
      // Player navigates to charging station
      commonInit(mMap.pointOfInterest('H'));
      spawnAlien(mMap.pointOfInterest('1'));
      spawnAlien(mMap.pointOfInterest('5'));
      spawnAlien(mMap.pointOfInterest('6'));
    } break;
    case 7: {
      //game_play_sound("danger");
      story(3205, "<p>Time to water the plants!</p>");
    } break;
    case 8: {
      // Player does tasks
      // Player navigates to charging station
      commonInit(mMap.pointOfInterest('H'));
      tasks.insert(3);
      tasks.insert(4);
      tasks.insert(5);
    } break;
    case 9: {
      game_play_sound("danger");
      story(4054, "<h2>Intruders!</h2><p>Quickly find and defeat the alien robots!</p>");
    } break;
    case 10: {
      // Invaders! Kill alien.
      // Player navigates to charging station
      commonInit(mMap.pointOfInterest('H'));
      spawnAlien(mMap.pointOfInterest('1'));
      spawnAlien(mMap.pointOfInterest('5'));
      spawnAlien(mMap.pointOfInterest('6'));
      spawnAlien(mMap.pointOfInterest('0'), true);
      spawnAlien(mMap.pointOfInterest('3'), true);
      spawnAlien(mMap.pointOfInterest('8'), true);
    } break;
    default: {
      // Nothing to do.
      // Player navigates to charging station
      story(5267, "<p>You made it into deep space. Your journey has come to an end.<br>You find: Nothing<br>(Seriously, what did you expect?)</p><p>Thanks for playing :-)</p>", true);
      mGameOver = true;
      game_play_sound("win");
    }
    }
    // tasks
    spawnConsole(mMap.pointOfInterest('a'), "Calibrate Flux Capacitor", !tasks.contains(0));
    spawnConsole(mMap.pointOfInterest('b'), "Flush Iridium Coil", !tasks.contains(1));
    spawnConsole(mMap.pointOfInterest('c'), "Rewire Power Mesh", !tasks.contains(2));
    spawnPlant(mMap.pointOfInterest('d'), "Water Plant", !tasks.contains(3));
    spawnPlant(mMap.pointOfInterest('e'), "Water Plant", !tasks.contains(4));
    spawnPlant(mMap.pointOfInterest('f'), "Water Plant", !tasks.contains(5));

    int i = 0;
    for (auto& crew : mCrew) {
      spawnCrew(mMap.pointOfInterest(crew.second.first), crew.first, crew.second.second, !tasks.contains(100 + (i++)));
    }
    
    if (mStoryBoard || mGameOver) {
      game_set_text(0, mStory.c_str());
    }
    //transitionFade = 1.0f;
  }

  void checkObjectives() {
    auto& state = GameState::instance();
    auto& playerActor = state.player->get(state.actorComp);
    if (playerActor.health <= 0) {
      if (!mGameOver) {
        mStory = "<h1>You did not make it!</h1><p>The ship is lost.</p><p>Click to Restart</p>";
        game_set_text(0, mStory.c_str());
      }
      mGameOver = true;
      state.objectivesDone = false;
      mRestartCooldown = 2.0f;
      return;
    }

    bool crewDead = true;
    for (auto t : state.scene.with(state.crewComp)) {
      auto& crew = t->get(state.crewComp);
      if (crew.health > 0) {
        crewDead = false;
        break;
      }
    }
    if (crewDead) {
      if (!mGameOver) {
        mStory = "<h1>The crew is dead!</h1><p>Your mission failed.</p><p>Click to Restart</p>";
        game_set_text(0, mStory.c_str());
      }
      mRestartCooldown = 2.0f;
      mGameOver = true;
      state.objectivesDone = false;
      return;
    }
    
    std::string todoList = "To Do:<ul>";
    bool tasksDone = true;
    // Are all aliens dead?
    for (auto e : state.scene.with(state.actorComp)) {
      auto& actor = e->get(state.actorComp);
      if (actor.faction != 0 && actor.health > 0) {
        tasksDone = false;
        mRenderer.drawMarker(vec3{playerActor.pos(0), 0, playerActor.pos(1)},
                             vec3{actor.pos(0), 0, actor.pos(1)});
      }
    }
    if (!tasksDone) {
      todoList += "<li>Defeat Intruders!</li>";
    }

    if (tasksDone) { // only check if no aliens alive
      // Any maintenance tasks?
      for (auto e : state.scene.with(state.taskComp)) {
        auto& task = e->get(state.taskComp);
        if (!task.complete) {
          auto& instr = e->get(state.instrComp);
          tasksDone = false;
          mRenderer.drawMarker(vec3{playerActor.pos(0), 0, playerActor.pos(1)},
                               vec3{task.pos(0), 0, task.pos(1)});
          todoList += "<li>" + instr.msg + "</li>";
        }
      }
    }

    if (tasksDone) {
      auto chargerPos = mMap.pointOfInterest('H');
      mRenderer.drawMarker(vec3{playerActor.pos(0), 0, playerActor.pos(1)},
                           vec3{chargerPos(0), 0, chargerPos(1)});
      todoList += "<li>Go to Charging Station</li>";
    }

    todoList += "</ul>";
    game_set_text(1, todoList.c_str());
    
    state.objectivesDone = tasksDone;
  }
  
  Game(gl::context ctx)
    : mMap(24, 17, "\
########   a    ########\
########  0    d########\
###   e#    b   #    ###\
###B   # ###### #   C###\
###A 9   #  c #  7  D###\
###    # # 8  # #    ###\
######## #    # ########\
######## #### # ########\
#                      #\
## ### ########## ### ##\
#   ## f########  ##6  #\
#H  ##  ########  ##   #\
###### ########## ######\
#             4        #\
#      2          5    #\
#  1       3           #\
##   ###   ##   ###   ##\
")
    , mPlayerBehavior(&mMap)
    , mAlienBehavior(&mMap)
    , mProjectileBehavior(&mMap)
    , mTaskBehavior(&mMap)
    , mRenderer(ctx)
    , mActorDrawable(mRenderer.createDrawable(actorMesh()))
    , mProjectileDrawable(mRenderer.createDrawable(geometry::generate_sphere(0.05f, 5, vec4{1,1,1,1})))
    , mConsoleDrawable(mRenderer.createDrawable((consoleMesh())))
    , mCrewDrawable(mRenderer.createDrawable(geometry::generate_cylinder(0.3f, 0.8f, 8, vec4{1,1,1,1})))
    , mPlantDrawable(mRenderer.createDrawable(plantMesh())) {
    printf("C Game\n");
    fflush(stdout);
    initLevel(mLevel);
  }
  ~Game() {
    printf("D Game\n");
    fflush(stdout);
  }
  void onLostFocus() {
    mPaused = true;
    game_set_text(0, "<h1>Paused</h1><h2>Click to Continue</h2>");
  }
  void onGainedFocus() {
    mPaused = false;
    if (mStoryBoard || mGameOver) {
      game_set_text(0, mStory.c_str());
    } else {
      game_set_text(0, "");
    }
  }
  int render(float dt, unsigned int width, unsigned int height, bool focus) {
    bool wasPaused = mPaused;
    auto& state = GameState::instance();
    if (focus && !mFocus) {
      onGainedFocus();
    }
    if (!focus && mFocus) {
      onLostFocus();
    }
    mFocus = focus;

    if (!wasPaused) {
      if (mGameOver && mMouse.mousedownMain() > 0 && mRestartCooldown < 0.0f) {
        game_play_sound("continue");
        mLevel = 0;
        initLevel(mLevel);
      }
      if (state.levelComplete || (mStoryBoard && mMouse.mousedownMain() > 0)) {
        game_play_sound("continue");
        initLevel(++mLevel);
      }
    }
    
    if (mFocus && !mGameOver && !mStoryBoard) {
      for (auto e : state.scene.with(state.behaviorComp)) {
        auto& behavior = e->get(state.behaviorComp);
        if (behavior.onUpdateHandler != nullptr) {
          behavior.onUpdateHandler->onUpdate(e, dt);
        }
      }
      
      {
        std::string msg;
        bool show = false;
        for (auto e : state.scene.with(state.instrComp)) {
          auto& instr = e->get(state.instrComp);
          if (instr.show) {
            show = true;
            if (instr.pressE) {
              msg = "Press E to " + instr.msg;
            } else {
              msg = instr.msg;
            }
          }
        }
        if (show && !mShowingInstructions) {
          mShowingInstructions = true;
          game_set_text(0, msg.c_str());
        }
        if (!show && mShowingInstructions) {
          mShowingInstructions = false;
          game_set_text(0, "");
        }
      }
      checkObjectives();
    } else {
      game_set_text(1, "");
    }
    mRenderer.render(width, height);

    if (mRestartCooldown > 0.0f) {
      mRestartCooldown -= dt;
    } else {
      mRestartCooldown = -1.0f;
    }
    
    return 0;
  }
private:
  Map mMap;
  PlayerBehavior mPlayerBehavior;
  AlienBehavior mAlienBehavior;
  ProjectileBehavior mProjectileBehavior;
  TriggerBehavior mTriggerBehavior;
  TaskBehavior mTaskBehavior;
  PlayerCameraBehavior mPlayerCameraBehavior;
  Renderer mRenderer;
  std::unique_ptr<Drawable> mActorDrawable;
  std::unique_ptr<Drawable> mProjectileDrawable;
  std::unique_ptr<Drawable> mMapDrawable;
  std::unique_ptr<Drawable> mChargerDrawable;
  std::unique_ptr<Drawable> mConsoleDrawable;
  std::unique_ptr<Drawable> mCrewDrawable;
  std::unique_ptr<Drawable> mPlantDrawable;
  std::vector<Entity*> mEntities;
  int mLevel = 0;
  bool mFocus = false;
  bool mGameOver = false;
  bool mStoryBoard = false;
  input::mouse_observer mMouse;
  bool mShowingInstructions = false;
  bool mPaused = false;
  std::map<std::string, std::pair<char, int>> mCrew;
  std::string mStory;
  float mRestartCooldown = -1.0f;
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
