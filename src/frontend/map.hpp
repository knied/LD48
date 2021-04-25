#ifndef FRONTEND_MAP_HPP
#define FRONTEND_MAP_HPP

#include "geometry.hpp"
#include <common/gjk.hpp>
#include "debug.hpp"
#include <map>

class Map final {
public:
  struct Tile {
    enum Type { Walkable, Wall };
    Type type;
  };
  
  Map(int width, int height, std::string const& layout)
    : mWidth(width), mHeight(height)
    , mPathing(mWidth * mHeight) {
    assert(mWidth * mHeight == layout.size());
    mTiles.resize(mWidth * mHeight);
    mInvalid.type = Tile::Wall;
    for (int x = 0; x < mWidth; ++x) {
      for (int y = 0; y < mHeight; ++y) {
        char c = layout[y * mWidth + x];
        Tile& tile = at(x, y);
        if (c == '#') {
          tile.type = Tile::Wall;
        } else {
          tile.type = Tile::Walkable;
          if (c != ' ') {
            mPointOfInterest.insert(std::make_pair(c, centerOf(x,y)));
          }
        }
      }
    }
  }
  Tile& at(int x, int y) {
    if (x < 0 || x >= mWidth ||
        y < 0 || y >= mHeight) {
      return mInvalid;
    }
    return mTiles[y * mWidth + x];
  }
  Tile const& at(int x, int y) const {
    if (x < 0 || x >= mWidth ||
        y < 0 || y >= mHeight) {
      return mInvalid;
    }
    return mTiles[y * mWidth + x];
  }
  Tile const& at(vec2 const& pos) const {
    int x = pos(0) >= 0.0f ? (int)pos(0) : (int)pos(0) - 1;
    int y = pos(1) >= 0.0f ? (int)pos(1) : (int)pos(1) - 1;
    return at(x, y);
  }
  int& costAt(int x, int y) {
    return mPathing[y * mWidth + x];
  }

  vec2 centerOf(int x, int y) const {
    return vec2{(float)x + 0.5f, (float)y + 0.5f};
  }

  vec2 nextWall(vec2 const& pos) const {
    int x = pos(0) >= 0.0f ? (int)pos(0) : (int)pos(0) - 1;
    int y = pos(1) >= 0.0f ? (int)pos(1) : (int)pos(1) - 1;
    if (at(x+1,y).type == Tile::Wall) return centerOf(x+1,y);
    if (at(x-1,y).type == Tile::Wall) return centerOf(x-1,y);
    if (at(x,y+1).type == Tile::Wall) return centerOf(x,y+1);
    if (at(x,y-1).type == Tile::Wall) return centerOf(x,y-1);
    return centerOf(x+1,y); // just pick one
  }

  void calcCost(int fx, int fy, int tx, int ty, int cost = 0) {
    if (at(tx, ty).type == Tile::Wall) return;
    auto& tc = costAt(tx, ty);
    if (tc <= cost) return;
    tc = cost;
    if (fx == tx && fy == ty) return;
    calcCost(fx, fy, tx + 1, ty, cost+1);
    calcCost(fx, fy, tx - 1, ty, cost+1);
    calcCost(fx, fy, tx, ty + 1, cost+1);
    calcCost(fx, fy, tx, ty - 1, cost+1);
  }
  vec2 nextWaypoint(vec2 const& from, vec2 const& to) {
    int fx = from(0) >= 0.0f ? (int)from(0) : (int)from(0) - 1;
    int fy = from(1) >= 0.0f ? (int)from(1) : (int)from(1) - 1;
    int tx = to(0) >= 0.0f ? (int)to(0) : (int)to(0) - 1;
    int ty = to(1) >= 0.0f ? (int)to(1) : (int)to(1) - 1;
    if (fx == tx && fy == ty) return to;
    
    for (auto& i : mPathing) {
      i = 10000000;
    }
    calcCost(fx, fy, tx, ty);
    int dir = 0;
    int min = 10000000;
    if (costAt(fx+1, fy) < min) {
      dir = 0;
      min = costAt(fx+1, fy);
    }
    if (costAt(fx-1, fy) < min) {
      dir = 1;
      min = costAt(fx-1, fy);
    }
    if (costAt(fx, fy+1) < min) {
      dir = 2;
      min = costAt(fx, fy+1);
    }
    if (costAt(fx, fy-1) < min) {
      dir = 3;
      min = costAt(fx, fy-1);
    }
    if (dir == 0) return centerOf(fx+1, fy);
    if (dir == 1) return centerOf(fx-1, fy);
    if (dir == 2) return centerOf(fx, fy+1);
    return centerOf(fx, fy-1);
  }

  bool tryMove(vec2 const& curr, vec2& move, float r) const {
    /*if (at(curr).type == Tile::Wall) {
      // Actor is stuck in wall. Let them escape.
      return;
      }*/

    int cx = curr(0) >= 0.0f ? (int)curr(0) : (int)curr(0) - 1;
    int cy = curr(1) >= 0.0f ? (int)curr(1) : (int)curr(1) - 1;
    auto npos = curr + move;
    /*if (at(npos).type == Tile::Wall) {
      move = vec2{0,0};
      }*/
    auto actorPos = vec3{ (float)npos(0), 0.5f, (float)npos(1) };
    gjk::sphere sphere{ 0.0f };
    auto mActor = mth::translation(actorPos);
    gjk::transformed_convex actor(mActor, &sphere);
    gjk::box box{ vec3{1,1,1} };
    //gjk::sphere box{ 0.5f };
    bool collision = false;
    for (int x = cx - 1; x <= cx + 1; ++x) {
      for (int y = cy - 1; y <= cy + 1; ++y) {
        //if (x == cx && y == cy) continue;
        if (at(x, y).type != Tile::Wall) continue;
        auto wallPos = vec3{ (float)x, 0.5f, (float)y };
        auto mWall = mth::translation(wallPos);
        gjk::transformed_convex wall(mWall, &box);
        gjk::simplex simplex;
        if (gjk::gjk(actor, wall, simplex)) {
          collision = true;
        } else {
          vec3 p0, p1;
          gjk::closest(simplex, p0, p1);
          if (mth::length(p1 - p0) > r) {
            //dbg::gizmos::instance().drawLine(p0, p1, vec4{0, 1, 0, 1});
          } else {
            auto normal = mth::normal(p1 - p0);
            auto normal2 = vec2{normal(0), normal(2)};
            auto dot = mth::dot(normal2, move);
            if (dot > 0) {
              move = move - dot * normal2;
            }
            //dbg::gizmos::instance().drawLine(p0, p1, vec4{1, 0, 0, 1});
            collision = true;
          }
        }
      }
    }
    return collision;
  }

  geometry::mesh mesh() const {
    geometry::mesh out;
    out.lines = false; // triangles
    vec4 floorColor{1,1,1,1};
    vec4 wallColor{0.8f,0.8f,0.8f,1};
    float const stride = 1.0f;
    float const wallHeight = 1.0f;
    for (int x = 0; x < mWidth; ++x) {
      for (int y = 0; y < mHeight; ++y) {
        Tile const& tile = at(x, y);
        if (tile.type == Tile::Walkable) {
          // floor
          unsigned int idx = (unsigned int)out.vd.size();
          vec3 norm{0,1,0};
          out.vd.push_back({ vec3{ stride * x, 0.0f, stride * y }, norm, floorColor });
          out.vd.push_back({ vec3{ stride * x, 0.0f, stride * (y+1) }, norm, floorColor });
          out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * y }, norm, floorColor });
          out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * (y+1) }, norm, floorColor });
          out.id.push_back(idx);
          out.id.push_back(idx+1);
          out.id.push_back(idx+2);
          out.id.push_back(idx+1);
          out.id.push_back(idx+3);
          out.id.push_back(idx+2);

          if (at(x+1, y).type == Tile::Wall) {
            // wall +x
            unsigned int idx = (unsigned int)out.vd.size();
            vec3 norm{-1,0,0};
            out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * y }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * (y+1) }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * y }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * (y+1) }, norm, wallColor });
            out.id.push_back(idx);
            out.id.push_back(idx+1);
            out.id.push_back(idx+2);
            out.id.push_back(idx+1);
            out.id.push_back(idx+3);
            out.id.push_back(idx+2);
          }
          if (at(x-1, y).type == Tile::Wall) {
            // wall -x
            unsigned int idx = (unsigned int)out.vd.size();
            vec3 norm{1,0,0};
            out.vd.push_back({ vec3{ stride * x, 0.0f, stride * y }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * x, 0.0f, stride * (y+1) }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * x, wallHeight, stride * y }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * x, wallHeight, stride * (y+1) }, norm, wallColor });
            out.id.push_back(idx);
            out.id.push_back(idx+2);
            out.id.push_back(idx+1);
            out.id.push_back(idx+1);
            out.id.push_back(idx+2);
            out.id.push_back(idx+3);
          }
          if (at(x, y+1).type == Tile::Wall) {
            // wall +y
            unsigned int idx = (unsigned int)out.vd.size();
            vec3 norm{0,0,-1};
            out.vd.push_back({ vec3{ stride * x, 0.0f, stride * (y+1) }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * x, wallHeight, stride * (y+1) }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * (y+1) }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * (y+1) }, norm, wallColor });
            out.id.push_back(idx);
            out.id.push_back(idx+1);
            out.id.push_back(idx+2);
            out.id.push_back(idx);
            out.id.push_back(idx+2);
            out.id.push_back(idx+3);
          }
          if (at(x, y-1).type == Tile::Wall) {
            // wall -y
            unsigned int idx = (unsigned int)out.vd.size();
            vec3 norm{0,0,1};
            out.vd.push_back({ vec3{ stride * x, 0.0f, stride * y }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * x, wallHeight, stride * y }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * y }, norm, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * y }, norm, wallColor });
            out.id.push_back(idx);
            out.id.push_back(idx+2);
            out.id.push_back(idx+1);
            out.id.push_back(idx);
            out.id.push_back(idx+3);
            out.id.push_back(idx+2);
          }
        } else {
          // blocked
          vec4 black{0,0,0,1};
          unsigned int idx = (unsigned int)out.vd.size();
          vec3 norm{0,1,0};
          out.vd.push_back({ vec3{ stride * x, wallHeight, stride * y }, norm, black });
          out.vd.push_back({ vec3{ stride * x, wallHeight, stride * (y+1) }, norm, black });
          out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * y }, norm, black });
          out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * (y+1) }, norm, black });
          out.id.push_back(idx);
          out.id.push_back(idx+1);
          out.id.push_back(idx+2);
          out.id.push_back(idx+1);
          out.id.push_back(idx+3);
          out.id.push_back(idx+2);
        }
      }
    }
    return out;
  }
  vec2 pointOfInterest(char c) {
    auto it = mPointOfInterest.find(c);
    if (it != mPointOfInterest.end()) {
      return it->second;
    }
    return vec2{0,0};
  }
private:
  int mWidth;
  int mHeight;
  Tile mInvalid;
  std::vector<Tile> mTiles;
  std::map<char, vec2> mPointOfInterest;
  std::vector<int> mPathing;
};

#endif // FRONTEND_MAP_HPP
