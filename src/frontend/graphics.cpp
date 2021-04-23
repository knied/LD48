#include "graphics.hpp"
#include "wasm.h"
#include <vector>

WASM_IMPORT("gl", "create_pipeline")
int gl_create_pipeline(unsigned int ctx,
                       char const* vs, char const* fs,
                       char const** attv, unsigned int attc,
                       char const** univ, unsigned int unic);
WASM_IMPORT("gl", "destroy_pipeline")
void gl_destroy_pipeline(int p);

WASM_IMPORT("gl", "create_mesh")
int gl_create_mesh(unsigned int ctx, gl::primitive_type pt,
                   gl::attribute_def const* defv, unsigned int defc);
WASM_IMPORT("gl", "destroy_mesh")
void gl_destroy_mesh(int m);
WASM_IMPORT("gl", "set_mesh")
void gl_set_mesh(int m, void const* vd, unsigned int vds,
                 void const* id, unsigned int ids);

WASM_IMPORT("gl", "create_mesh_binding")
int gl_create_mesh_binding(unsigned int ctx, int m, int p);
WASM_IMPORT("gl", "destroy_mesh_binding")
void gl_destroy_mesh_binding(int b);

WASM_IMPORT("gl", "create_uniform_binding")
int gl_create_uniform_binding(unsigned int ctx, int p, unsigned int size,
                              gl::uniform_def const* defv, unsigned int defc);
WASM_IMPORT("gl", "destroy_uniform_binding")
void gl_destroy_uniform_binding(int b);

WASM_IMPORT("gl", "execute_command_buffer")
void gl_execute_command_buffer(gl::context ctx, void const* buffer, unsigned int size);

namespace gl {

pipeline::pipeline(context ctx,
                   std::string const& vsGlsl, std::string const& fsGlsl,
                   std::vector<std::string> const& attributes,
                   std::vector<std::string> const& uniforms) {
  char const** attv = new char const*[attributes.size()];
  for (std::size_t i = 0; i < attributes.size(); ++i) {
    attv[i] = attributes[i].c_str();
  }
  char const** univ = new char const*[uniforms.size()];
  for (std::size_t i = 0; i < uniforms.size(); ++i) {
    univ[i] = uniforms[i].c_str();
  }
  m_handle = ::gl_create_pipeline(ctx, vsGlsl.c_str(), fsGlsl.c_str(),
                                  attv, attributes.size(),
                                  univ, uniforms.size());
}

pipeline::~pipeline() {
  ::gl_destroy_pipeline(m_handle);
}

uniform_binding::uniform_binding(context ctx, pipeline const* p,
                                 unsigned int size, std::vector<uniform_def> defs) {
  m_handle = ::gl_create_uniform_binding(ctx, p->handle(), size,
                                         defs.data(), defs.size());
}

uniform_binding::~uniform_binding() {
  ::gl_destroy_uniform_binding(m_handle);
}

mesh::mesh(context ctx, primitive_type pt,
           std::vector<attribute_def> const& defs) {
  m_handle = ::gl_create_mesh(ctx, pt, defs.data(), defs.size());
}

mesh::~mesh() {
  ::gl_destroy_mesh(m_handle);
}

void mesh::set(void const* vd, unsigned int vds,
               std::vector<unsigned int> const& id) {
  ::gl_set_mesh(m_handle, vd, vds, id.data(), id.size());
}

mesh_binding::mesh_binding(context ctx, mesh const* m, pipeline const* p) {
  m_handle = ::gl_create_mesh_binding(ctx, m->handle(), p->handle());
}

mesh_binding::~mesh_binding() {
  ::gl_destroy_mesh_binding(m_handle);
}

command_buffer::command_buffer(context ctx, std::size_t initial)
  : mCtx(ctx), mBuffer(initial) {
}

void command_buffer::commit() {
  write(COMMAND_END);
  ::gl_execute_command_buffer(mCtx, mBuffer.data(), mSize);
  mSize = 0;
}

} // namespace gl
