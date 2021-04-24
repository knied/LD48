#ifndef FRONTEND_MAP_HPP
#define FRONTEND_MAP_HPP

#include "geometry.hpp"
#include <common/gjk.hpp>
#include "debug.hpp"

class Map final {
public:
  struct Tile {
    enum Type { Walkable, Wall };
    Type type;
  };
  
  Map(int width, int height)
    : mWidth(width), mHeight(height) {
    mTiles.resize(mWidth * mHeight);
    mInvalid.type = Tile::Wall;
    for (int x = 0; x < mWidth; ++x) {
      for (int y = 0; y < mHeight; ++y) {
        Tile& tile = at(x, y);
        if (x  > 3 && y > 3) {
          tile.type = Tile::Wall; // just a test
        } else {
          tile.type = Tile::Walkable;
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

  void tryMove(vec2 const& curr, vec2& move, float r) const {
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
    for (int x = cx - 1; x <= cx + 1; ++x) {
      for (int y = cy - 1; y <= cy + 1; ++y) {
        if (x == cx && y == cy) continue;
        if (at(x, y).type != Tile::Wall) continue;
        auto wallPos = vec3{ (float)x, 0.5f, (float)y };
        auto mWall = mth::translation(wallPos);
        gjk::transformed_convex wall(mWall, &box);
        gjk::simplex simplex;
        if (gjk::gjk(actor, wall, simplex)) {
          // ignore
        } else {
          vec3 p0, p1;
          gjk::closest(simplex, p0, p1);
          if (mth::length(p1 - p0) > r) {
            dbg::gizmos::instance().drawLine(p0, p1, vec4{0, 1, 0, 1});
          } else {
            auto normal = mth::normal(p1 - p0);
            auto normal2 = vec2{normal(0), normal(2)};
            auto dot = mth::dot(normal2, move);
            if (dot > 0) {
              move = move - dot * normal2;
            }
            dbg::gizmos::instance().drawLine(p0, p1, vec4{1, 0, 0, 1});
          }
        }
      }
    }
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
          out.vd.push_back({ vec3{ stride * x, 0.0f, stride * y }, floorColor });
          out.vd.push_back({ vec3{ stride * x, 0.0f, stride * (y+1) }, floorColor });
          out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * y }, floorColor });
          out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * (y+1) }, floorColor });
          out.id.push_back(idx);
          out.id.push_back(idx+1);
          out.id.push_back(idx+2);
          out.id.push_back(idx+1);
          out.id.push_back(idx+3);
          out.id.push_back(idx+2);

          if (at(x+1, y).type == Tile::Wall) {
            // wall +x
            unsigned int idx = (unsigned int)out.vd.size();
            out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * y }, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * (y+1) }, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * y }, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * (y+1) }, wallColor });
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
            out.vd.push_back({ vec3{ stride * x, 0.0f, stride * y }, wallColor });
            out.vd.push_back({ vec3{ stride * x, 0.0f, stride * (y+1) }, wallColor });
            out.vd.push_back({ vec3{ stride * x, wallHeight, stride * y }, wallColor });
            out.vd.push_back({ vec3{ stride * x, wallHeight, stride * (y+1) }, wallColor });
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
            out.vd.push_back({ vec3{ stride * x, 0.0f, stride * (y+1) }, wallColor });
            out.vd.push_back({ vec3{ stride * x, wallHeight, stride * (y+1) }, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * (y+1) }, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * (y+1) }, wallColor });
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
            out.vd.push_back({ vec3{ stride * x, 0.0f, stride * y }, wallColor });
            out.vd.push_back({ vec3{ stride * x, wallHeight, stride * y }, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), wallHeight, stride * y }, wallColor });
            out.vd.push_back({ vec3{ stride * (x+1), 0.0f, stride * y }, wallColor });
            out.id.push_back(idx);
            out.id.push_back(idx+2);
            out.id.push_back(idx+1);
            out.id.push_back(idx);
            out.id.push_back(idx+3);
            out.id.push_back(idx+2);
          }
        }
      }
    }
    return out;
  }
private:
  int mWidth;
  int mHeight;
  Tile mInvalid;
  std::vector<Tile> mTiles;
};

#endif // FRONTEND_MAP_HPP
