#ifndef FRONTEND_RENDERER_HPP
#define FRONTEND_RENDERER_HPP

#include "state.hpp"
#include "graphics.hpp"
#include "geometry.hpp"
#include "debug.hpp"

template<typename T, unsigned int R, unsigned int C>
static constexpr inline unsigned int
entry_count(mth::matrix<T, R, C> const&) noexcept {
  return R * C;
}
template<typename T, unsigned int R, unsigned int C>
static constexpr inline gl::attribute_type
entry_gl_type(mth::matrix<T, R, C> const&) noexcept {
  return gl::atype<T>();
}

struct uniform {
  mat4 mat;
  vec4 color;
};

static std::vector<gl::attribute_def>
vertex_defs() {
  geometry::vertex tmp;
  return {
    { "a_position", sizeof(geometry::vertex), offsetof(geometry::vertex, pos), entry_count(tmp.pos), entry_gl_type(tmp.pos), false },
    { "a_color", sizeof(geometry::vertex), offsetof(geometry::vertex, color), entry_count(tmp.color), entry_gl_type(tmp.color), false }
  };
}

class Drawable final {
public:
  Drawable(gl::context ctx, gl::pipeline const* pipeline,
           geometry::mesh const& mesh) {
    auto primType = mesh.lines ? gl::PRIMITIVE_LINES : gl::PRIMITIVE_TRIANGLES;
    mMesh = std::make_unique<gl::mesh>(
      ctx, primType, vertex_defs());
    mBinding = std::make_unique<gl::mesh_binding>(
      ctx, mMesh.get(), pipeline);
    mMesh->set(mesh.vd, mesh.id);
  }
  gl::mesh_binding const* binding() const {
    return mBinding.get();
  }
private:
  std::unique_ptr<gl::mesh> mMesh;
  std::unique_ptr<gl::mesh_binding> mBinding;
};

class Renderer final {
public:
  Renderer(gl::context ctx)
    : mCtx(ctx)
    , mCommandBuffer(mCtx) {
    mPipeline = std::unique_ptr<gl::pipeline>(new gl::pipeline(mCtx, "\
attribute vec4 a_position;\n\
attribute vec4 a_color;\n\
uniform mat4 u_mat;\n\
uniform vec4 u_color;\n\
varying vec4 v_color;\n\
void main() {\n\
  gl_Position = u_mat * a_position;\n\
  v_color = a_color * u_color;\n\
}\n","\
precision mediump float;\n\
varying vec4 v_color;\n\
void main() {\n\
  gl_FragColor = v_color;\n\
}\n", {"a_position", "a_color"}, {"u_mat", "u_color"}));
    mGizmosMesh = std::make_unique<gl::mesh>(
      mCtx, gl::PRIMITIVE_LINES, vertex_defs());
    mGizmosMeshBinding = std::make_unique<gl::mesh_binding>(
      mCtx, mGizmosMesh.get(), mPipeline.get());
    //auto mesh = geometry::generate_terrain(20, 20, vec4{1,1,1,1}, true);
    //auto mesh = geometry::generate_sphere(1.0f, 16, vec4{1,1,1,1}, true);
    //auto mesh = geometry::generate_box(1.0f, 2.0f, 3.0f, vec4{1,1,1,1}, true);
    //mMesh = std::make_unique<gl::mesh>(
    //  mCtx, gl::PRIMITIVE_LINES, vertex_defs());
    //mMeshBinding = std::make_unique<gl::mesh_binding>(
    //  mCtx, mMesh.get(), mPipeline.get());
    //mMesh->set(mesh.vd, mesh.id);
    std::vector<gl::uniform_def> defs {
      { "u_mat", gl::UNIFORM_MF4, offsetof(uniform, mat) },
      { "u_color", gl::UNIFORM_VF4, offsetof(uniform, color) }
    };
    mUniformBinding = std::make_unique<gl::uniform_binding>(mCtx, mPipeline.get(), sizeof(uniform), defs);
  }
  void render(unsigned int width, unsigned int height) {
    auto& state = GameState::instance();
    auto cam = state.camera;
    mCommandBuffer.set_viewport(0, 0, width, height);
    mCommandBuffer.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    mCommandBuffer.clear(gl::CLEAR_COLOR);
    mCommandBuffer.set_pipeline(mPipeline.get());

    assert(cam->has(state.cameraComp));
    assert(cam->has(state.transComp));
    auto const& camera = cam->get(state.cameraComp);
    auto projection = mth::perspective_projection<float>(
      width, height, camera.fov, camera.znear, camera.zfar);
    auto view = mth::inverse(world(cam, state.transComp));
    auto vp = projection * view;

    { // Gizmos
      auto& gizmos = dbg::gizmos::instance();
      auto& mesh = gizmos.mesh();
      mGizmosMesh->set(mesh.vd, mesh.id);
      gizmos.clear();
      uniform u{ mth::transpose(vp), vec4{1,1,1,1} };
      mCommandBuffer.set_mesh(mGizmosMeshBinding.get());
      mCommandBuffer.set_uniforms(mUniformBinding.get(), u);
      mCommandBuffer.draw();
    }

    // Entities
    for (auto e : state.scene.with(state.shapeComp, state.transComp)) {
      auto const& shape = e->get(state.shapeComp);
      uniform u{
        mth::transpose(vp * world(e, state.transComp)),
        shape.color
      };
      if (shape.drawable != nullptr && shape.visible == true) {
        mCommandBuffer.set_mesh(shape.drawable->binding());
        mCommandBuffer.set_uniforms(mUniformBinding.get(), u);
        mCommandBuffer.draw();
      }
    }
    mCommandBuffer.commit();
  }

  std::unique_ptr<Drawable> createDrawable(geometry::mesh const& mesh) {
    return std::make_unique<Drawable>(mCtx, mPipeline.get(), mesh);
  }
private:
  gl::context mCtx;
  std::unique_ptr<gl::pipeline> mPipeline;
  std::unique_ptr<gl::mesh> mGizmosMesh;
  std::unique_ptr<gl::mesh_binding> mGizmosMeshBinding;
  std::unique_ptr<gl::uniform_binding> mUniformBinding;
  gl::command_buffer mCommandBuffer;
};

#endif // FRONTEND_RENDERER_HPP
