#ifndef FRONTEND_RENDERER_HPP
#define FRONTEND_RENDERER_HPP

#include "state.hpp"
#include "graphics.hpp"
#include "geometry.hpp"
#include "debug.hpp"
#include <iostream>

inline geometry::mesh
markerMesh() {
  geometry::mesh out;
  out.lines = false; // triangles

  float h = 0.11f;
  float a = 0.15f;
  float b = 0.3f;
  float c = 0.5f;
  vec3 norm{0,0,0};
  vec4 color{1,1,1,1};
  
  out.vd.push_back({ vec3{0, h, b}, norm, color });
  out.vd.push_back({ vec3{0, h, c}, norm, color });
  out.vd.push_back({ vec3{a, h, a}, norm, color });

  out.vd.push_back({ vec3{0, h, b}, norm, color });
  out.vd.push_back({ vec3{-a, h, a}, norm, color });
  out.vd.push_back({ vec3{0, h, c}, norm, color });

  for (std::size_t i = 0; i < out.vd.size(); ++i) {
    out.id.push_back(i);
  }
  out.calculateNormals();
  return out;
}

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
  mat4 nmat;
  vec4 color;
  vec3 ldir;
  float flash;
};

static std::vector<gl::attribute_def>
vertex_defs() {
  geometry::vertex tmp;
  return {
    { "a_position", sizeof(geometry::vertex), offsetof(geometry::vertex, pos), entry_count(tmp.pos), entry_gl_type(tmp.pos), false },
    { "a_normal", sizeof(geometry::vertex), offsetof(geometry::vertex, norm), entry_count(tmp.norm), entry_gl_type(tmp.norm), false },
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

    mLightPipeline = std::unique_ptr<gl::pipeline>(new gl::pipeline(mCtx, "\
attribute vec4 a_position;\n\
attribute vec3 a_normal;\n\
attribute vec4 a_color;\n\
uniform mat4 u_mat;\n\
uniform mat4 u_nmat;\n\
uniform vec4 u_color;\n\
varying vec4 v_color;\n\
varying vec3 v_normal;\n\
void main() {\n\
  gl_Position = u_mat * a_position;\n\
  v_normal = (u_nmat * vec4(a_normal,0.0)).xyz;\n\
  v_color = a_color * u_color;\n\
}\n","\
precision mediump float;\n\
uniform vec3 u_ldir;\n\
uniform float u_flash;\n\
varying vec4 v_color;\n\
varying vec3 v_normal;\n\
void main() {\n\
  vec3 ambientLight = vec3(0.4,0.5,0.6);\n\
  vec3 sunLight = vec3(3.0,3.0,2.0);\n\
  float gamma = 2.2;\n\
  vec3 hdr = vec3(0.0);\n\
  // Sunlight\n\
  float intensity = clamp(dot(normalize(v_normal), u_ldir), 0.0, 1.0);\n\
  hdr = hdr + v_color.xyz * sunLight * intensity;\n\
  // Ambient\n\
  hdr = hdr + v_color.xyz * ambientLight;\n\
  vec3 ldr = hdr / (hdr + vec3(1.0,1.0,1.0));\n\
  vec3 gc = pow(ldr, vec3(1.0 / gamma));\n\
  gl_FragColor = u_flash * vec4(1,1,1,1) + (1.0 - u_flash) * vec4(gc, 1.0);\n\
}\n", {"a_position", "a_normal", "a_color"}, {"u_mat", "u_nmat", "u_color", "u_ldir", "u_flash"}));
    
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
      { "u_nmat", gl::UNIFORM_MF4, offsetof(uniform, nmat) },
      { "u_color", gl::UNIFORM_VF4, offsetof(uniform, color) },
      { "u_ldir", gl::UNIFORM_VF3, offsetof(uniform, ldir) },
      { "u_flash", gl::UNIFORM_VF1, offsetof(uniform, flash) }
    };
    mUniformBinding = std::make_unique<gl::uniform_binding>(mCtx, mPipeline.get(), sizeof(uniform), defs);
    mLightUniformBinding = std::make_unique<gl::uniform_binding>(mCtx, mLightPipeline.get(), sizeof(uniform), defs);

    mMarkerDrawable = std::make_unique<Drawable>(mCtx, mPipeline.get(), markerMesh());
    //mMarkerDrawable = std::make_unique<Drawable>(mCtx, mPipeline.get(),
    //  geometry::generate_sphere(0.1f, 8, vec4{1,1,1,1}));
  }
  void render(unsigned int width, unsigned int height) {
    auto& state = GameState::instance();
    auto cam = state.camera;
    mCommandBuffer.set_viewport(0, 0, width, height);
    mCommandBuffer.set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
    mCommandBuffer.clear(gl::CLEAR_COLOR);
    mCommandBuffer.set_pipeline(mPipeline.get());

    assert(cam->has(state.cameraComp));
    assert(cam->has(state.transComp));
    auto const& camera = cam->get(state.cameraComp);
    auto projection = mth::perspective_projection<float>(
      width, height, camera.fov, camera.znear, camera.zfar);
    auto view = mth::inverse(world(cam, state.transComp));
    auto ldir = mth::normal(mth::transform_vector(view, vec3{1.0f,1.0f,0.3f}));
    auto vp = projection * view;

    { // Gizmos
      auto& gizmos = dbg::gizmos::instance();
      auto& mesh = gizmos.mesh();
      mGizmosMesh->set(mesh.vd, mesh.id);
      gizmos.clear();
      auto nmat = mth::identity<float,4,4>();
      uniform u{ mth::transpose(vp), nmat, vec4{1,1,1,1}, ldir, 0.0f };
      mCommandBuffer.set_mesh(mGizmosMeshBinding.get());
      mCommandBuffer.set_uniforms(mUniformBinding.get(), u);
      mCommandBuffer.draw();
    }

    // Markers
    for (auto& pair : mMarkers) {
      (void)pair;
      auto& p0 = pair.first;
      auto& p1 = pair.second;
      auto m = mth::translation(p0) * mth::look_at_matrix(p0, p1, vec3{0,1,0});
      //auto m = mth::look_at_matrix(p0, p1, vec3{0,1,0}) * mth::translation(p0);
      auto nmat = mth::identity<float,4,4>();
      uniform u{ mth::transpose(vp * m), nmat, vec4{1,1,0,1}, ldir, 0.0f };
      mCommandBuffer.set_mesh(mMarkerDrawable->binding());
      mCommandBuffer.set_uniforms(mUniformBinding.get(), u);
      mCommandBuffer.draw();
    }
    mMarkers.clear();

    mCommandBuffer.set_pipeline(mLightPipeline.get());
    // Entities
    for (auto e : state.scene.with(state.shapeComp, state.transComp)) {
      auto const& shape = e->get(state.shapeComp);
      auto model = world(e, state.transComp) * shape.trans;
      auto nmat = mth::inverse(view * model);
      uniform u{
        mth::transpose(vp * model), nmat, shape.color, ldir, shape.flash
      };
      if (shape.drawable != nullptr && shape.visible == true) {
        mCommandBuffer.set_mesh(shape.drawable->binding());
        mCommandBuffer.set_uniforms(mLightUniformBinding.get(), u);
        mCommandBuffer.draw();
      }
    }
    mCommandBuffer.commit();
  }

  void drawMarker(vec3 const& p0, vec3 const& p1) {
    mMarkers.push_back(std::make_pair(p0, p1));
  }

  std::unique_ptr<Drawable> createDrawable(geometry::mesh const& mesh) {
    return std::make_unique<Drawable>(mCtx, mLightPipeline.get(), mesh);
  }
private:
  gl::context mCtx;
  std::unique_ptr<gl::pipeline> mPipeline;
  std::unique_ptr<gl::pipeline> mLightPipeline;
  std::unique_ptr<gl::mesh> mGizmosMesh;
  std::unique_ptr<gl::mesh_binding> mGizmosMeshBinding;
  std::unique_ptr<gl::uniform_binding> mUniformBinding;
  std::unique_ptr<gl::uniform_binding> mLightUniformBinding;
  std::unique_ptr<Drawable> mMarkerDrawable;
  std::vector<std::pair<vec3, vec3>> mMarkers;
  gl::command_buffer mCommandBuffer;
};

#endif // FRONTEND_RENDERER_HPP
