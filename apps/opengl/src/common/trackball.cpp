// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#include "trackball.h"
using namespace ospcommon;
/** constractors */
void viewer::Trackball::SetRadius(const float r) { radius = r; }
void viewer::Trackball::SetCoordinate(const vec3f& u, 
                                      const vec3f& d,
                                      const vec3f& p)
{
  vec3f Z = normalize(d);
  vec3f U = normalize(cross(Z, -u));
  vec3f V = cross(Z, U);
  const linear3f l(U, V, Z);
  matrix_new = affine3f(l, vec3f(0.f));
  matrix_old = affine3f(l, vec3f(0.f));
  UpdateMatrix();
}

/**
 * @brief BeginDrag/Zoom: initialize drag/zoom
 * @param x: previous x position
 * @param y: previous y position
 */
void viewer::Trackball::BeginDrag(const float x, const float y) 
{
  position_old = proj2surf(x, y);
  matrix_old = matrix_new;
}
void viewer::Trackball::BeginZoom(const float x, const float y) 
{
  zoom_old = y;
}

/**
 * @brief Drag/Zoom: execute Drag/Zoom
 * @param x: current x position
 * @param y: current y position
 */
void viewer::Trackball::Drag(const float x, const float y)
{
  // new mouse position
  position_new = proj2surf(x, y);
  const auto p0 = normalize(position_old);
  const auto p1 = normalize(position_new);
  // get direction and angle
  vec3f dir = -normalize(cross(p0, p1));
  float angle = ospcommon::acos(dot(p0, p1));
  // update matrix
  if (angle < 0.001f) {
    // to prevent position_prev == position, this will cause invalid value
    return;
  } else { // compute rotation
    matrix_new = matrix_old * affine3f::rotate(dir, angle);
    UpdateMatrix();
  }
}
void viewer::Trackball::Zoom(const float x, const float y) 
{
  zoom_new += (y - zoom_old);
  zoom_old = y;
  UpdateMatrix();
}

/**
 * @brief matrix: trackball matrix accessor
 * @return current trackball matrix
 */
const affine3f& viewer::Trackball::Matrix() const 
{ 
  return matrix_final;
}
void viewer::Trackball::Reset() 
{
  matrix_new = affine3f(OneTy());
  zoom_new = 1.0f;
}
void viewer::Trackball::Reset(const affine3f &m) { matrix_new = m; }
void viewer::Trackball::UpdateMatrix()
{
  matrix_final = matrix_new * affine3f::scale(vec3f(zoom_new));
}

/**
 * @brief proj2surf: project (x,y) mouse pos on surface
 * @param x: X position
 * @param y: Y position
 * @return projected position
 */
vec3f viewer::Trackball::proj2surf(const float x, const float y) const 
{
  float r = x * x + y * y;
  float R = radius * radius;
  float z = r > R / 2 ? R / 2 / 
    ospcommon::sqrt(r) : ospcommon::sqrt(R - r);
  return vec3f(x, y, z);
}
