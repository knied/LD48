#ifndef FRONTEND_GRAPHICS_HPP
#define FRONTEND_GRAPHICS_HPP

#include <string>
#include <vector>

namespace gl {

using context = unsigned int;

class pipeline {
public:
  pipeline(context ctx,
           std::string const& vsGlsl, std::string const& fsGlsl,
           std::vector<std::string> const& attributes,
           std::vector<std::string> const& uniforms);
  ~pipeline();
  int handle() const { return m_handle; }
private:
  int m_handle;
};

enum uniform_type {
  UNIFORM_VF1 = 1,
  UNIFORM_VF2,
  UNIFORM_VF3,
  UNIFORM_VF4,
  UNIFORM_VI1,
  UNIFORM_VI2,
  UNIFORM_VI3,
  UNIFORM_VI4,
  UNIFORM_MF2,
  UNIFORM_MF3,
  UNIFORM_MF4
};
struct uniform_def {
  char const* name;
  uniform_type type;
  unsigned int offset;
};
class uniform_binding {
public:
  uniform_binding(context ctx, pipeline const* p,
                  unsigned int size, std::vector<uniform_def> defs);
  ~uniform_binding();
  int handle() const { return m_handle; }
private:
  int m_handle;
};

enum attribute_type {
  ATTRIBUTE_FLOAT = 1,
  ATTRIBUTE_UNSIGNED_BYTE
};
struct attribute_def {
  char const* name;
  unsigned int stride;
  unsigned int offset;
  unsigned int size;
  attribute_type type;
  bool normalize;
};
enum primitive_type {
  PRIMITIVE_TRIANGLES = 1,
  PRIMITIVE_LINES
};
class mesh {
private:
  mesh(context ctx, primitive_type pt,
       void const* vd, unsigned int vds,
       std::vector<unsigned int> const& id,
       std::vector<attribute_def> const& defs);
public:
  template<typename vertex>
  mesh(context ctx, primitive_type pt,
       std::vector<vertex> const& vd,
       std::vector<unsigned int> const& id,
       std::vector<attribute_def> const& defs)
    : mesh(ctx, pt, vd.data(), sizeof(vertex) * vd.size(), id, defs) {}
  ~mesh();
  int handle() const { return m_handle; }
private:
  int m_handle;
};
class mesh_binding {
public:
  mesh_binding(context ctx, mesh const* m, pipeline const* p);
  ~mesh_binding();
  int handle() const { return m_handle; }
private:
  int m_handle;
};

enum clear_mask : unsigned int {
  CLEAR_COLOR = 1,
  CLEAR_DEPTH = 2,
  CLEAR_STENCIL = 4
};
enum command : unsigned int {
  COMMAND_END = 0,
  COMMAND_SET_VIEWPORT,
  COMMAND_SET_CLEAR_COLOR,
  COMMAND_CLEAR,
  COMMAND_SET_PIPELINE,
  COMMAND_SET_UNIFORMS,
  COMMAND_SET_MESH,
  COMMAND_DRAW
};
class command_buffer final {
public:
  command_buffer(context ctx, std::size_t initial = 64);
  ~command_buffer() = default;

  void set_viewport(int x, int y, unsigned int width, unsigned int height) {
    write(COMMAND_SET_VIEWPORT);
    write(x);
    write(y);
    write(width);
    write(height);
  }
  void set_clear_color(float r, float g, float b, float a) {
    write(COMMAND_SET_CLEAR_COLOR);
    write(r);
    write(g);
    write(b);
    write(a);
  }
  void set_pipeline(pipeline const* p) {
    write(COMMAND_SET_PIPELINE);
    write(p->handle());
  }
  void set_mesh(mesh_binding const* m) {
    write(COMMAND_SET_MESH);
    write(m->handle());
  }
  template<typename uniform>
  void set_uniforms(uniform_binding const* binding, uniform data) {
    write(COMMAND_SET_UNIFORMS);
    write(binding->handle());
    write(data);
  }
  void clear(clear_mask mask) {
    write(COMMAND_CLEAR);
    write(mask);
  }
  void draw() {
    write(COMMAND_DRAW);
  }
  void commit();
private:
  template<typename T>
  void write(T const& value) {
    auto size = sizeof(T);
    while (mSize + size > mBuffer.size()) {
      mBuffer.resize(2 * mBuffer.size());
    }
    *(T*)(mBuffer.data() + mSize) = value;
    mSize += size;
  }

  context mCtx;
  std::vector<char> mBuffer;
  std::size_t mSize = 0;
};

} // namespace gl

#endif // FRONTEND_GRAPHICS_HPP
