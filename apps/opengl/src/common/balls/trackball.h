/**
 * This file defines the trackball class
 * which is independent to the project itself.
 */
#pragma once
#ifndef OSPRAY_TRACKBALL_H
#define OSPRAY_TRACKBALL_H

#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"

//
// following the implementation of
// http://image.diku.dk/research/trackballs/index.html
//
// NOTE:
// This trackball has some camera requirements:
// 1) Camera should be located on negtive z axis initially
// 2) Use y axis as the initial up vector
//
using namespace ospcommon;
class Trackball {
private:
  float radius = 1.0f;
  affine3f matrix_new = affine3f(OneTy());
  affine3f matrix_old = affine3f(OneTy());
  vec3f position_new;
  vec3f position_old;
  float zoom_new = 1.f;
  float zoom_old;
public:
  /** constractors */
  Trackball() {}
  void SetRadius(const float r) { radius = r; }

  /**
   * @brief BeginDrag/Zoom: initialize drag/zoom
   * @param x: previous x position
   * @param y: previous y position
   */
  void BeginDrag(float x, float y) {
    position_old = proj2surf(x, y);
    matrix_old = matrix_new;
  }

  void BeginZoom(float x, float y) {
    zoom_old = y;
  }

  /**
   * @brief Drag/Zoom: execute Drag/Zoom
   * @param x: current x position
   * @param y: current y position
   */
  void Drag(float x, float y, 
            ospcommon::vec3f u, 
            ospcommon::vec3f d) 
  {
    position_new = proj2surf(x, y);

    /* ospcommon::vec3f Z = normalize(d); */
    /* ospcommon::vec3f U = normalize(cross(Z, u)); */
    /* ospcommon::vec3f V = cross(U, Z); */
    /* linear3f l = linear3f(U, V, Z); */

    //affine3f m = affine3f(l.inverse(), ospcommon::vec3f(0.f, 0.f, 0.f));

    /* auto p0 = l.inverse() * position_prev; */
    /* auto p1 = l.inverse() * position; */

    auto p0 =  position_new;
    auto p1 =  position_old;

    // get direction
    vec3f dir = normalize(cross(p0, p1));

    // compute rotation angle
    float angle = ospcommon::acos(dot(normalize(p0), normalize(p1)));
    if (angle < 0.001f) {
      // to prevent position_prev == position, this will cause invalid value
      return;
    } else { // compute rotation
      matrix_new = matrix_old * affine3f::rotate(dir, angle);
    }
  }

  void Zoom(float x, float y) {
    zoom_new += (y - zoom_old);
    zoom_old = y;
  }

  /**
   * @brief matrix: trackball matrix accessor
   * @return current trackball matrix
   */
  affine3f Matrix() { return matrix_new * affine3f::scale(vec3f(zoom_new)); }

  void Reset() {
    matrix_new = affine3f(OneTy());
    zoom_new = 1.0f;
  }

  void Reset(const affine3f &m) { matrix_new = m; }

private:
  /**
   * @brief proj2surf: project (x,y) mouse pos on surface
   * @param x: X position
   * @param y: Y position
   * @return projected position
   */
  vec3f proj2surf(const float x, const float y) const {
    float r = x * x + y * y;
    float R = radius * radius;
    float z = r > R / 2 ? R / 2 / ospcommon::sqrt(r) : ospcommon::sqrt(R - r);
    return vec3f(x, y, z);
  }
};

#endif //OSPRAY_TRACKBALL_H
