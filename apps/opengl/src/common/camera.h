#pragma once
#ifndef OSPRAY_CAMERA_H
#define OSPRAY_CAMERA_H
#include "common/constants.h"
#include "common/properties.h"
#include "common/balls/trackball.h"
class Camera {
private:
  vec2f mouse2screen(const int& x, const int& y, 
                     const float& width, const float& height) {
    return vec2f(2.0f * (float) x / width  - 1.0f, 
                 2.0f * (float) y / height - 1.0f);
  }

private:
  size_t width, height;
  float aspect;
  float fovy;
  vec3f eye;   // this trackball requires camera to be
  vec3f focus; // initialized on negtive z axis with
  vec3f up;    // y axis as the initial up vector !!!!
  Trackball ball;
  viewer::CameraProp& prop;
public:
  Camera(viewer::CameraProp& p) : prop(p) {}
  void SetSize(const size_t& w, const size_t& h) 
  {
    width = w;
    height = h;
    aspect = (float) width / height;
  }
  void SetViewPort(const vec3f& vp, const vec3f& vu, 
                   const vec3f& vi, const float& angle)
  {
    eye = vp;
    up  = vu;
    focus = vi;
    fovy = angle;
  }
  size_t CameraWidth() { return this->width; }
  size_t CameraHeight() { return this->height; }
  float  CameraFocalLength() { return length(focus - eye); }
  vec3f  CameraFocus() { return this->focus; }
  vec3f  CameraPos() {
    auto dir = -xfmVector(this->ball.Matrix().l, this->eye - this->focus);
    auto up  =  xfmVector(this->ball.Matrix().l, this->up);
    return (-dir + this->focus);
  }
  vec3f  CameraUp() {
    return xfmVector(this->ball.Matrix().l, this->up);
  }
  void   CameraBeginZoom(const float& x, const float& y) {
    const vec2f p = mouse2screen(x, y, this->width, this->height);
    this->ball.BeginZoom(p.x, p.y);
  }
  void   CameraZoom(const float& x, const float& y) {
    const vec2f p = mouse2screen(x, y, this->width, this->height);
    this->ball.Zoom(p.x, p.y);
    CameraUpdateView();
  }
  void   CameraBeginDrag(const float& x, const float& y) {
    const vec2f p = mouse2screen(x, y, this->width, this->height);
    this->ball.BeginDrag(p.x, p.y);
  }
  void   CameraDrag(const float& x, const float& y) {
    const vec2f p = mouse2screen(x, y, this->width, this->height);
    //auto dir = -xfmVector(this->ball.Matrix().l, this->eye - this->focus);
    //auto up  =  xfmVector(this->ball.Matrix().l, this->up);
    //this->ball.Drag(p.x, p.y, up, dir);
    this->ball.Drag(p.x, p.y, this->up, this->focus - this->eye);
    CameraUpdateView();
  }
  void   CameraMoveNZ(const float& v) {
    const auto dir = -normalize(xfmVector(this->ball.Matrix().l, 
                                          this->eye - this->focus));
    focus += v * dir;
    eye   += v * dir;
    CameraUpdateView();
  }
  void   CameraMovePX(const float& v) {
    const auto Z = normalize(xfmVector(this->ball.Matrix().l, 
                                       this->eye - this->focus));
    const auto Y =  xfmVector(this->ball.Matrix().l, this->up);    
    const auto dir = cross(Y, Z);
    focus += v * dir;
    eye   += v * dir;
    CameraUpdateView();
  }
  void   CameraMovePY(const float& v) {
    const auto dir =  xfmVector(this->ball.Matrix().l, this->up);    
    focus += v * dir;
    eye   += v * dir;
    CameraUpdateView();
  }

  //--------------------------------------------------------
  // OSPRay related
  //--------------------------------------------------------
  void Init(OSPCamera camera, const std::string type) 
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
  void CameraUpdateView()
  {
    auto dir = -xfmVector(this->ball.Matrix().l, this->eye - this->focus);
    auto up  =  xfmVector(this->ball.Matrix().l, this->up);
    auto pos = (-dir + this->focus);
    prop.SetPos(pos);
    prop.SetDir(dir);
    prop.SetUp(up);
  }
  void CameraUpdateProj(const size_t& width, const size_t& height) 
  {
    SetSize(width, height);
    prop.SetAspect(aspect);
    prop.SetFovy(fovy);
  }
};

#endif //OSPRAY_CAMERA_H
