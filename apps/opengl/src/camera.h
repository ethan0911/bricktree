// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#pragma once
#ifndef OSPRAY_CAMERA_H
#define OSPRAY_CAMERA_H
// local
#include "common/constants.h"
#include "common/trackball.h"
#include "scene/properties.h"
// ospray
#include "ospray/ospray.h"
#include "ospcommon/vec.h"
namespace viewer {
  class Camera {
  private:
    ospcommon::vec2f mouse2screen(const int& x, const int& y, 
                                  const float& width, 
                                  const float& height);
  private:
    size_t width, height;
    float aspect;
    float fovy;
    ospcommon::vec3f eye;   // this trackball requires camera to be
    ospcommon::vec3f focus; // initialized on negtive z axis with
    ospcommon::vec3f up;    // y axis as the initial up vector !!!!
    viewer::Trackball ball;
    viewer::CameraProp& prop;
  public:
    Camera(viewer::CameraProp& p);
    void SetSize(const size_t& w, const size_t& h);
    void SetViewPort(const ospcommon::vec3f& vp,
                     const ospcommon::vec3f& vu, 
                     const ospcommon::vec3f& vi, 
                     const float& angle);
    size_t CameraWidth();
    size_t CameraHeight();
    float  CameraFocalLength();
    ospcommon::vec3f CameraFocus();
    ospcommon::vec3f CameraPos(const ospcommon::vec3f& dir);
    ospcommon::vec3f CameraPos();
    ospcommon::vec3f CameraUp();
    ospcommon::vec3f CameraDir();
    void   CameraBeginZoom(const float& x, const float& y);
    void   CameraZoom(const float& x, const float& y);
    void   CameraBeginDrag(const float& x, const float& y);
    void   CameraDrag(const float& x, const float& y);
    void   CameraMoveNZ(const float& v);
    void   CameraMovePX(const float& v);
    void   CameraMovePY(const float& v);
    //--------------------------------------------------------
    // OSPRay related
    //--------------------------------------------------------
    void Init(OSPCamera camera, const std::string type);
    void CameraUpdateView();
    void CameraUpdateProj(const size_t& width, const size_t& height);
  };
};
#endif //OSPRAY_CAMERA_H
