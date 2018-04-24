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
  bool inverse_rotate = true; // inverse rotation
  float radius = 1.0f;
  affine3f matrix = affine3f(OneTy());
  affine3f matrix_prev = affine3f(OneTy());
  vec3f position;
  vec3f position_prev;
  float position_surf[2];
  float zoom = 1.f;
  float zoom_prev;
public:
  /** constractors */
  Trackball() {}

  void SetRadius(const float r) { radius = r; }

  void SetInverseRotateMode(bool r) { inverse_rotate = r; }

  /**
   * @brief BeginDrag/Zoom: initialize drag/zoom
   * @param x: previous x position
   * @param y: previous y position
   */
  void BeginDrag(float x, float y) {
    position_prev = proj2surf(x, y);
    position_surf[0] = x;
    position_surf[1] = y;
    matrix_prev = matrix;
  }

  void BeginZoom(float x, float y) {
    zoom_prev = y;
  }

  /**
   * @brief Drag/Zoom: execute Drag/Zoom
   * @param x: current x position
   * @param y: current y position
   */
  void Drag(float x, float y) {
    // get direction
    position = proj2surf(x, y);
    vec3f dir = normalize(cross(position_prev, position));
    if (inverse_rotate) dir *= -1.0f;
    // compute rotation angle
    float angle = ospcommon::acos(dot(normalize(position_prev), normalize(position)));
    if (angle < 0.001f) {
      // to prevent position_prev == position, this will cause invalid value
      return;
    } else { // compute rotation
      matrix = matrix_prev * affine3f::rotate(dir, angle);
    }
  }

  void Zoom(float x, float y) {
    zoom += (y - zoom_prev);
    zoom_prev = y;
  }

  /**
   * @brief matrix: trackball matrix accessor
   * @return current trackball matrix
   */
  affine3f Matrix() { return matrix * affine3f::scale(vec3f(zoom)); }

  void Reset() {
    matrix = affine3f(OneTy());
    zoom = 1.0f;
  }

  void Reset(const affine3f &m) { matrix = m; }

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
