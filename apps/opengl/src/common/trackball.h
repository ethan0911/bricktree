// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
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
namespace viewer {
  class Trackball {
  private:
    float radius = 1.0f;
    ospcommon::affine3f matrix_new   {ospcommon::OneTy()};
    ospcommon::affine3f matrix_old   {ospcommon::OneTy()};
    ospcommon::affine3f matrix_final {ospcommon::OneTy()};
    ospcommon::vec3f position_new;
    ospcommon::vec3f position_old;
    float zoom_new = 1.f;
    float zoom_old = 1.f;
  public:
    /** constractors */
    void SetRadius(const float r);
    void SetCoordinate(const ospcommon::vec3f& u, 
                       const ospcommon::vec3f& d,
                       const ospcommon::vec3f& p);

    /**
     * @brief BeginDrag/Zoom: initialize drag/zoom
     * @param x: previous x position
     * @param y: previous y position
     */
    void BeginDrag(const float x, const float y);
    void BeginZoom(const float x, const float y);

    /**
     * @brief Drag/Zoom: execute Drag/Zoom
     * @param x: current x position
     * @param y: current y position
     */
    void Drag(const float x, const float y);
    void Zoom(const float x, const float y);

    /**
     * @brief matrix: trackball matrix accessor
     * @return current trackball matrix
     */
    const ospcommon::affine3f& Matrix() const;
    void Reset();
    void Reset(const ospcommon::affine3f &m);

  private:
    void UpdateMatrix();
    /**
     * @brief proj2surf: project (x,y) mouse pos on surface
     * @param x: X position
     * @param y: Y position
     * @return projected position
     */
    ospcommon::vec3f proj2surf(const float x, const float y) const;
  };
};
#endif //OSPRAY_TRACKBALL_H
