// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#include "camera.h"
#include "ospcommon/AffineSpace.h"
using namespace ospcommon;
viewer::Camera::Camera(viewer::CameraProp& p) : prop(p) {}
vec2f viewer::Camera::mouse2screen(const int& x, const int& y, 
                                   const float& width,
                                   const float& height) 
{
  return vec2f(2.0f * (float) x / width  - 1.0f, 
               2.0f * (float) y / height - 1.0f);
}
size_t viewer::Camera::CameraWidth() { return this->width; }
size_t viewer::Camera::CameraHeight() { return this->height; }
float  viewer::Camera::CameraFocalLength() { return length(focus - eye); }
vec3f  viewer::Camera::CameraFocus() { return this->focus; }
vec3f  viewer::Camera::CameraPos() 
{
  auto dir = CameraDir();
  return (-dir + this->focus);
}
vec3f  viewer::Camera::CameraPos(const vec3f& dir) 
{
  return (-dir + this->focus);
}
vec3f  viewer::Camera::CameraUp() 
{
  return xfmVector(this->ball.Matrix().l, vec3f(0.f, 1.f, 0.f));
}
vec3f  viewer::Camera::CameraDir() 
{
  return -xfmVector(this->ball.Matrix().l, vec3f(0, 0, -CameraFocalLength()));
}
void   viewer::Camera::CameraBeginZoom(const float& x, const float& y) {
  const vec2f p = mouse2screen(x, y, this->width, this->height);
  this->ball.BeginZoom(p.x, p.y);
}
void   viewer::Camera::CameraZoom(const float& x, const float& y) {
  const vec2f p = mouse2screen(x, y, this->width, this->height);
  this->ball.Zoom(p.x, p.y);
  CameraUpdateView();
}
void   viewer::Camera::CameraBeginDrag(const float& x, const float& y) {
  const vec2f p = mouse2screen(x, y, this->width, this->height);
  this->ball.BeginDrag(p.x, p.y);
}
void   viewer::Camera::CameraDrag(const float& x, const float& y) {
  const vec2f p = mouse2screen(x, y, this->width, this->height);
  this->ball.Drag(p.x, p.y);
  CameraUpdateView();
}
void   viewer::Camera::CameraMoveNZ(const float& v) {
  const auto dir = CameraDir();
  focus += v * dir;
  eye   += v * dir;
  CameraUpdateView();
}
void   viewer::Camera::CameraMovePX(const float& v) {
  const auto Z = CameraDir();
  const auto Y = CameraUp();
  const auto dir = -cross(Y, Z);
  focus += v * dir;
  eye   += v * dir;
  CameraUpdateView();
}
void   viewer::Camera::CameraMovePY(const float& v) {
  const auto dir = CameraUp();
  focus += v * dir;
  eye   += v * dir;
  CameraUpdateView();
}

//--------------------------------------------------------
// OSPRay related
//--------------------------------------------------------
void viewer::Camera::SetSize(const size_t& w, const size_t& h) 
{
  width = w;
  height = h;
  aspect = (float) width / height;
  prop.SetAspect(aspect);
}
void viewer::Camera::SetViewPort(const vec3f& vp, const vec3f& vu, 
                                 const vec3f& vi, const float& angle)
{
  up    = vu;
  eye   = vp;
  focus = vi;
  fovy = angle;
  prop.SetUp(this->up);
  prop.SetDir(this->focus - this->eye);
  prop.SetPos(this->eye);
  prop.SetFovy(this->fovy);
  this->ball.SetCoordinate(this->up, 
                           this->focus - this->eye,
                           this->eye);
}
void viewer::Camera::Init(OSPCamera camera, const std::string type) 
{
  if (camera == nullptr) { 
    throw std::runtime_error("empty camera found"); 
  }
  if (type == "perspective") {
    prop.Init(camera, viewer::CameraProp::Perspective);
  } else if (type == "orthographic") {
    prop.Init(camera, viewer::CameraProp::Orthographic);
  } else {
    prop.Init(camera, viewer::CameraProp::Panoramic);
  }
  CameraUpdateView();
  CameraUpdateProj(this->width, this->height);
}
void viewer::Camera::CameraUpdateView()
{
  auto dir = CameraDir();
  auto up  = CameraUp();
  auto pos = CameraPos(dir);
  prop.SetPos(pos);
  prop.SetDir(dir);
  prop.SetUp(up);
}
void viewer::Camera::CameraUpdateProj(const size_t& width, 
                                      const size_t& height) 
{
  SetSize(width, height);
}
