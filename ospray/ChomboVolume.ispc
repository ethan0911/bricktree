// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ours
#include "CellRef.ih"
#include "Octant.ih"
// ospray
#include "ospray/math/AffineSpace.ih"
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"
// embree
#include "embree2/rtcore.isph"



/*! plainest, dumbest way of reconstructing: find the given leaf cell,
  and return the value */
extern varying float Chombo_debug(void *uniform _self, 
                                  const varying vec3f &worldSpacePos)
{
  ChomboVolume *uniform self = (ChomboVolume *)_self;
  return 0.f;
}

/*! plainest, dumbest way of reconstructing: find the given leaf cell,
  and return the value */
extern varying float Chombo_nearestNeighbor(void *uniform _self, 
                                            const varying vec3f &worldSpacePos)
{
  ChomboVolume *uniform self = (ChomboVolume *)_self;
  Chombo *uniform chom = &self->chombo;
  
  // for now, let's assume the world-space position already IS in grid space
  const vec3f gridPos = worldSpacePos;
  CellRef leaf = findLeafCell(chom,gridPos);
  return leaf.value;
}

/*! interpolate on the finest level */
extern varying float Chombo_lerpOnFinestLevel(void *uniform _self, 
                                              const varying vec3f &worldSpacePos);

/*! performs trilinear interpolation on the level that the query point
is in.  this means that this method _will_ adapt to the level we're
in, and _will_ perorm proper trilinear interpolation in areas that do
_not_ contain a boundary.... BUT it definitely _will_ lead to
inconsistencies across level boundaries, since neighboring cells will
interpolate from different levels */
extern float octMethodSimple(const uniform Chombo *uniform self,
                             const vec3f &pos);

/*! the simplest variant of an octant method - determines the octant
    of the leaf cell the poitn is in, then finds the 8 neighboring
    dual octants of same or coarser level. If NOT at a level boundary
    that is the same as interpolating at the given level; across a
    cell boundary it is NOT smooth, but close. */
extern varying float Chombo_octantMethodSimple(void *uniform _self, 
                                               const varying vec3f &worldSpacePos)
{
  ChomboVolume *uniform self = (ChomboVolume *)_self;
  Chombo *uniform chom = &self->chombo;
  return octMethodSimple(chom,worldSpacePos);
  
  // // cell width on finest level - we can always offset by this value
  // // to make sure we're in a neighbor cell...
  // const float fcw = chom->finestLevelCellWidth;
  // const float hfcw = 0.5f * fcw;

  // // for now, let's assume the world-space position already IS in grid space
  // const vec3f pos = worldSpacePos;

  // CellRef leaf = findLeafCell(chom,pos);
  // const vec3f leafCenter = centerOf(leaf);

  // // neighbor cell _locator_ - ie, a position somewhere _in_ that
  // // cell, but neither the boundary nor the cell center
  // const vec3f loc
  //   = make_vec3f(pos.x < leafCenter.x
  //                ? leaf.pos.x - hfcw
  //                : leaf.pos.x + leaf.width + hfcw,
  //                pos.y < leafCenter.y
  //                ? leaf.pos.y - hfcw
  //                : leaf.pos.y + leaf.width + hfcw,
  //                pos.z < leafCenter.z
  //                ? leaf.pos.z - hfcw
  //                : leaf.pos.z + leaf.width + hfcw);

  // CellRef cell[2][2][2];
  // cell[0][0][0] = leaf; //findLeafCell(chom,make_vec3f(pos.x,pos.y,pos.z);
  // cell[0][0][1] = findLeafCell(chom,make_vec3f(loc.x,pos.y,pos.z));
  // cell[0][1][0] = findLeafCell(chom,make_vec3f(pos.x,loc.y,pos.z));
  // cell[0][1][1] = findLeafCell(chom,make_vec3f(loc.x,loc.y,pos.z));
  // cell[1][0][0] = findLeafCell(chom,make_vec3f(pos.x,pos.y,loc.z));
  // cell[1][0][1] = findLeafCell(chom,make_vec3f(loc.x,pos.y,loc.z));
  // cell[1][1][0] = findLeafCell(chom,make_vec3f(pos.x,loc.y,loc.z));
  // cell[1][1][1] = findLeafCell(chom,make_vec3f(loc.x,loc.y,loc.z));

  // const float dx = abs(pos.x-leafCenter.x)/leaf.width;
  // const float dy = abs(pos.y-leafCenter.y)/leaf.width;
  // const float dz = abs(pos.z-leafCenter.z)/leaf.width;

  // const float f000 = cell[0][0][0].value;
  // const float f001 = cell[0][0][1].value;
  // const float f010 = cell[0][1][0].value;
  // const float f011 = cell[0][1][1].value;
  // const float f100 = cell[1][0][0].value;
  // const float f101 = cell[1][0][1].value;
  // const float f110 = cell[1][1][0].value;
  // const float f111 = cell[1][1][1].value;
  
  // const float f00 = (1.f-dx)*f000 + dx*f001;
  // const float f01 = (1.f-dx)*f010 + dx*f011;
  // const float f10 = (1.f-dx)*f100 + dx*f101;
  // const float f11 = (1.f-dx)*f110 + dx*f111;

  // const float f0 = (1.f-dy)*f00+dy*f01;
  // const float f1 = (1.f-dy)*f10+dy*f11;

  // const float f = (1.f-dz)*f0+dz*f1;
  // return f;
}


inline float octMethodOnLevel(const uniform Chombo *uniform chom,
                              const float desiredLevel, const vec3f &pos)
{
  const CellRef mine = findCell(chom,pos,desiredLevel);
  const vec3f mineCenter = centerOf(mine);
  // Delta to octant corners
  const float DX = (pos.x >= mineCenter.x ? +.5f : -.5f)*mine.width;
  const float DY = (pos.y >= mineCenter.y ? +.5f : -.5f)*mine.width;
  const float DZ = (pos.z >= mineCenter.z ? +.5f : -.5f)*mine.width;

  // neighbor cell _locator_ - ie, a position somewhere _in_ that
  // cell, but neither the boundary nor the cell center
  const vec3f loc = mineCenter + 2.0f*make_vec3f(DX,DY,DZ);

  CellRef cell[2][2][2];
  cell[0][0][0] = mine; //findLeafCell(chom,make_vec3f(pos.x,pos.y,pos.z);
  cell[0][0][1] = findCell(chom,make_vec3f(loc.x,pos.y,pos.z),mine.width);
  cell[0][1][0] = findCell(chom,make_vec3f(pos.x,loc.y,pos.z),mine.width);
  cell[0][1][1] = findCell(chom,make_vec3f(loc.x,loc.y,pos.z),mine.width);
  cell[1][0][0] = findCell(chom,make_vec3f(pos.x,pos.y,loc.z),mine.width);
  cell[1][0][1] = findCell(chom,make_vec3f(loc.x,pos.y,loc.z),mine.width);
  cell[1][1][0] = findCell(chom,make_vec3f(pos.x,loc.y,loc.z),mine.width);
  cell[1][1][1] = findCell(chom,make_vec3f(loc.x,loc.y,loc.z),mine.width);

  // interpolation levels (actually, cell width) in given directions:
  // it's always the maximum cell width of any neighbor cell that
  // abuts on this/these side(s)
  const float l_dx  = cell[0][0][1].width;
  const float l_dy  = cell[0][1][0].width;
  const float l_dz  = cell[1][0][0].width;
  const float l_dxy = max(max(l_dx,l_dy),cell[0][1][1].width);
  const float l_dyz = max(max(l_dy,l_dz),cell[1][1][0].width);
  const float l_dxz = max(max(l_dx,l_dz),cell[1][0][1].width);
  const float l_dxyz = max(max(max(l_dxy,l_dxz),l_dyz),cell[1][1][1].width);
  // "my" width
  const float my_l = cell[0][0][0].width;

  // -------------------------------------------------------
  // compute octant corners
  // -------------------------------------------------------
  // us:
  const float oc000 = cell[0][0][0].value;
  // face neighbors
  const float oc001 = (l_dx == my_l) 
    ? 0.5f*(cell[0][0][1].value+cell[0][0][0].value)
    : octMethodOnLevel(chom,l_dx,mineCenter+make_vec3f(0,0,DX));
  const float oc010 = (l_dy == my_l)
    ? 0.5f*(cell[0][1][0].value+cell[0][0][0].value)
    : octMethodOnLevel(chom,l_dy,mineCenter+make_vec3f(0,DY,0));
  const float oc100 = (l_dz == my_l)
    ? 0.5f*(cell[1][0][0].value+cell[0][0][0].value)
    : octMethodOnLevel(chom,l_dz,mineCenter+make_vec3f(DZ,0,0));

  const float oc011 = (l_dxy == my_l)
    ? 0.25f*(cell[0][0][0].value+cell[0][0][1].value+
             cell[0][1][0].value+cell[0][1][1].value)
    : octMethodOnLevel(chom,l_dxy,mineCenter+make_vec3f(0,DY,DX));
  const float oc101 = (l_dxz == my_l)
    ? 0.25f*(cell[0][0][0].value+cell[0][0][1].value+
             cell[1][0][0].value+cell[0][0][1].value)
    : octMethodOnLevel(chom,l_dxz,mineCenter+make_vec3f(DZ,0,DX));
  const float oc110 = (l_dyz == my_l)
    ? 0.25f*(cell[0][0][0].value+cell[1][0][0].value+
             cell[0][1][0].value+cell[1][1][0].value)
    : octMethodOnLevel(chom,l_dyz,mineCenter+make_vec3f(DZ,DY,0));
  
  const float oc111 = (l_dxyz == my_l)
    ? 0.125f*(cell[0][0][1].value+cell[0][0][0].value+
              cell[0][0][1].value+cell[0][0][0].value+
              cell[0][1][1].value+cell[0][1][0].value+
              cell[0][1][1].value+cell[0][1][0].value)
    : octMethodOnLevel(chom,l_dxyz,mineCenter+make_vec3f(DZ,DY,DZ));
  
  // -------------------------------------------------------
  // interpolation
  // -------------------------------------------------------
  // interpolation weights inside octant (mind the "*2" for the octant)
  const float dx = 2.f*abs(pos.x-mineCenter.x)/mine.width;
  const float dy = 2.f*abs(pos.y-mineCenter.y)/mine.width;
  const float dz = 2.f*abs(pos.z-mineCenter.z)/mine.width;
  
  // interpolant inputs
  const float f000 = oc000;
  const float f001 = oc001;
  const float f010 = oc010;
  const float f011 = oc011;
  const float f100 = oc100;
  const float f101 = oc101;
  const float f110 = oc110;
  const float f111 = oc111;
  
  // interpolate in x
  const float f00 = (1.f-dx)*f000 + dx*f001;
  const float f01 = (1.f-dx)*f010 + dx*f011;
  const float f10 = (1.f-dx)*f100 + dx*f101;
  const float f11 = (1.f-dx)*f110 + dx*f111;
  
  // interpolate result in y
  const float f0 = (1.f-dy)*f00+dy*f01;
  const float f1 = (1.f-dy)*f10+dy*f11;
  
  // interpolate result in z
  const float f = (1.f-dz)*f0+dz*f1;
  return f;
}

#include "optOctMethod.ispc"
#include "compOctMethod.ispc"

/*! the simplest variant of an octant method - determines the octant
    of the leaf cell the poitn is in, then finds the 8 neighboring
    dual octants of same or coarser level. If NOT at a level boundary
    that is the same as interpolating at the given level; across a
    cell boundary it is NOT smooth, but close. */
extern varying float Chombo_octantMethodProper(void *uniform _self, 
                                               const varying vec3f &worldSpacePos)
{
  ChomboVolume *uniform self = (ChomboVolume *)_self;
  Chombo *uniform chom = &self->chombo;
  
  // for now, let's assume the world-space position already IS in grid space
  const vec3f pos = worldSpacePos;

#if 1
  // return opt_octMethodOnLevel(chom,0.f,pos);
  return compOctMethodOnLevel(chom,0.f,pos);
#else
  return octMethodOnLevel(chom,0.f,pos);
#endif
}
 


//! The gradient at the given sample location in world coordinates.
static varying vec3f Chombo_gradient(void *uniform _self, 
                                     const varying vec3f &worldCoordinates)
{
  return make_vec3f(1,0,0);
}

//! Find the next hit point in the volume for ray casting based renderers.
static void Chombo_intersect(void *uniform _self, 
                             varying Ray &ray)
{
  ChomboVolume *uniform self = (ChomboVolume *uniform) _self;

  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const uniform float step
    = self->super.samplingStep / self->super.samplingRate;

  ray.t0 += step;
}


export void *uniform ChomboVolume_create(void *uniform cppE)
{
  ChomboVolume *uniform self = uniform new uniform ChomboVolume;
  // self->super.cppE = cppE;
  return self;
} 

export void ChomboVolume_set(void *uniform _self,
                             void *uniform _xf,
                             uniform vec3i &rootGridDims,
                             void *uniform _brick_array,
                             void *uniform _tree_node,
                             uniform int32 numTreeNodes,
                             void *uniform _tree_item,
                             uniform float finestLevelCellWidth)
{
  ChomboVolume *uniform self = (ChomboVolume *uniform)_self;
  assert(self);
  
  self->super.boundingBox.lower = make_vec3f(0.f);
  self->super.boundingBox.upper = make_vec3f(rootGridDims); 
  
  self->super.volumeClippingBox.lower = make_vec3f(0.f);
  self->super.volumeClippingBox.upper = make_vec3f(rootGridDims); 
  
  self->super.samplingRate = 1.f;//samplingRate;
  self->super.samplingStep = 1.f; //.1f;

  self->super.gradientShadingEnabled = 0;

  self->super.transferFunction = (TransferFunction *uniform)_xf;

  self->super.intersect        = &Chombo_intersect;
  self->super.computeGradient  = &Chombo_gradient;
#if 0
  self->super.computeSample    = &Chombo_debug;
#elif 0
  // works
  self->super.computeSample    = &Chombo_nearestNeighbor;
#elif 0
  // works
  self->super.computeSample    = &Chombo_lerpOnFinestLevel;
#elif 1
  self->super.computeSample    = &Chombo_octantMethodSimple;
#elif 1
  self->super.computeSample    = &Chombo_octantMethodProper;
#else
# error no sample() method selected...
#endif

  // self->chombo.brick           = (ChomboBrick  *)_brick_array;
  self->chombo.node            = (KDTreeNode   *)_tree_node;
  self->chombo.numNodes        = numTreeNodes;
  self->chombo.item            = (ChomboBrick **)_tree_item;
  self->chombo.rootGridDims    = rootGridDims;

  self->chombo.bounds.lower    = make_vec3f(0);
  self->chombo.bounds.upper    = make_vec3f(rootGridDims);
  self->chombo.finestLevelCellWidth = finestLevelCellWidth;
  print("#osp:amr: chombo volume set\n");
  
  //  buildNodeRanges(self);
}




