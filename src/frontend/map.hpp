#ifndef FRONTEND_MAP_HPP
#define FRONTEND_MAP_HPP

#include "geometry.hpp"

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
        if (x == y) {
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
