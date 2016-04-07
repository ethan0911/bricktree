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
// ospray
#include "ospray/math/AffineSpace.ih"
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"
// embree
#include "embree2/rtcore.isph"


//#define CACHE_OCTANT


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
                                              const varying vec3f &worldSpacePos)
{
  ChomboVolume *uniform self = (ChomboVolume *)_self;
  Chombo *uniform chom = &self->chombo;
  
  // for now, let's assume the world-space position already IS in grid space
  const vec3f pos = worldSpacePos;

  const float cw = chom->finestLevelCellWidth;
  const float hcw = 0.5f * cw;

  // translate to grid space on finest grid level
  const float fx = (pos.x-hcw)/cw;
  const float fy = (pos.y-hcw)/cw;
  const float fz = (pos.z-hcw)/cw;

  const float x0 = (pos.x <= hcw) ? -1.f : (float)(int)(fx);
  const float y0 = (pos.y <= hcw) ? -1.f : (float)(int)(fy);
  const float z0 = (pos.z <= hcw) ? -1.f : (float)(int)(fz);

  const CellRef c000 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(0.5f,0.5f,0.5f))*cw);
  const CellRef c001 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(1.5f,0.5f,0.5f))*cw);
  const CellRef c010 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(0.5f,1.5f,0.5f))*cw);
  const CellRef c011 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(1.5f,1.5f,0.5f))*cw);
  const CellRef c100 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(0.5f,0.5f,1.5f))*cw);
  const CellRef c101 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(1.5f,0.5f,1.5f))*cw);
  const CellRef c110 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(0.5f,1.5f,1.5f))*cw);
  const CellRef c111 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(1.5f,1.5f,1.5f))*cw);

  const float dx = fx - x0;
  const float dy = fy - y0;
  const float dz = fz - z0;

  const float f000 = c000.value;
  const float f001 = c001.value;
  const float f010 = c010.value;
  const float f011 = c011.value;
  const float f100 = c100.value;
  const float f101 = c101.value;
  const float f110 = c110.value;
  const float f111 = c111.value;
  
  const float f00 = (1.f-dx)*f000 + dx*f001;
  const float f01 = (1.f-dx)*f010 + dx*f011;
  const float f10 = (1.f-dx)*f100 + dx*f101;
  const float f11 = (1.f-dx)*f110 + dx*f111;

  const float f0 = (1.f-dy)*f00+dy*f01;
  const float f1 = (1.f-dy)*f10+dy*f11;

  const float f = (1.f-dz)*f0+dz*f1;
  return f00;
}





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
  
  // cell width on finest level - we can always offset by this value
  // to make sure we're in a neighbor cell...
  const float fcw = chom->finestLevelCellWidth;
  const float hfcw = 0.5f * fcw;

  // for now, let's assume the world-space position already IS in grid space
  const vec3f pos = worldSpacePos;

  CellRef leaf = findLeafCell(chom,pos);
  const vec3f leafCenter = centerOf(leaf);

  // neighbor cell _locator_ - ie, a position somewhere _in_ that
  // cell, but neither the boundary nor the cell center
  const vec3f loc
    = make_vec3f(pos.x < leafCenter.x
                 ? leaf.pos.x - hfcw
                 : leaf.pos.x + leaf.width + hfcw,
                 pos.y < leafCenter.y
                 ? leaf.pos.y - hfcw
                 : leaf.pos.y + leaf.width + hfcw,
                 pos.z < leafCenter.z
                 ? leaf.pos.z - hfcw
                 : leaf.pos.z + leaf.width + hfcw);

  CellRef cell[2][2][2];
  cell[0][0][0] = leaf; //findLeafCell(chom,make_vec3f(pos.x,pos.y,pos.z);
  cell[0][0][1] = findLeafCell(chom,make_vec3f(loc.x,pos.y,pos.z));
  cell[0][1][0] = findLeafCell(chom,make_vec3f(pos.x,loc.y,pos.z));
  cell[0][1][1] = findLeafCell(chom,make_vec3f(loc.x,loc.y,pos.z));
  cell[1][0][0] = findLeafCell(chom,make_vec3f(pos.x,pos.y,loc.z));
  cell[1][0][1] = findLeafCell(chom,make_vec3f(loc.x,pos.y,loc.z));
  cell[1][1][0] = findLeafCell(chom,make_vec3f(pos.x,loc.y,loc.z));
  cell[1][1][1] = findLeafCell(chom,make_vec3f(loc.x,loc.y,loc.z));

  const float dx = abs(pos.x-leafCenter.x)/leaf.width;
  const float dy = abs(pos.y-leafCenter.y)/leaf.width;
  const float dz = abs(pos.z-leafCenter.z)/leaf.width;

  const float f000 = cell[0][0][0].value;
  const float f001 = cell[0][0][1].value;
  const float f010 = cell[0][1][0].value;
  const float f011 = cell[0][1][1].value;
  const float f100 = cell[1][0][0].value;
  const float f101 = cell[1][0][1].value;
  const float f110 = cell[1][1][0].value;
  const float f111 = cell[1][1][1].value;
  
  const float f00 = (1.f-dx)*f000 + dx*f001;
  const float f01 = (1.f-dx)*f010 + dx*f011;
  const float f10 = (1.f-dx)*f100 + dx*f101;
  const float f11 = (1.f-dx)*f110 + dx*f111;

  const float f0 = (1.f-dy)*f00+dy*f01;
  const float f1 = (1.f-dy)*f10+dy*f11;

  const float f = (1.f-dz)*f0+dz*f1;
  return f;
}


#ifdef CACHE_OCTANT
varying vec3f cached_lo, cached_hi, cached_center;
varying float cached_width = 0.f;
varying float cached_val[2][2][2];

#endif

inline float octMethodOnLevel(const uniform Chombo *uniform chom,
                              const float desiredLevel, const vec3f &pos)
{
#ifdef CACHE_OCTANT
  if (desiredLevel == 0.f) {
    if (pos.x >= cached_lo.x && pos.x <= cached_hi.x &&
        pos.y >= cached_lo.y && pos.y <= cached_hi.y &&
        pos.z >= cached_lo.z && pos.z <= cached_hi.z) {
      
  // -------------------------------------------------------
  // interpolation
  // -------------------------------------------------------
  // interpolation weights inside octant (mind the "*2" for the octant)
      const vec3f mineCenter = cached_center;
      const float dx = 2.f*abs(pos.x-mineCenter.x)/cached_width;
      const float dy = 2.f*abs(pos.y-mineCenter.y)/cached_width;
      const float dz = 2.f*abs(pos.z-mineCenter.z)/cached_width;
      
      // interpolant inputs
      const float f000 = cached_val[0][0][0];
      const float f001 = cached_val[0][0][1];
      const float f010 = cached_val[0][1][0];
      const float f011 = cached_val[0][1][1];
      const float f100 = cached_val[1][0][0];
      const float f101 = cached_val[1][0][1];
      const float f110 = cached_val[1][1][0];
      const float f111 = cached_val[1][1][1];
      
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
  }
#endif

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

#ifdef CACHE_OCTANT
  if (desiredLevel == 0.f) {
    cached_width = mine.width;
    cached_center = mineCenter;
    cached_lo = min(mineCenter,mineCenter+make_vec3f(DX,DY,DZ));
    cached_hi = max(mineCenter,mineCenter+make_vec3f(DX,DY,DZ));
    cached_val[0][0][0] = oc000;
    cached_val[0][0][1] = oc001;
    cached_val[0][1][0] = oc010;
    cached_val[0][1][1] = oc011;
    cached_val[1][0][0] = oc100;
    cached_val[1][0][1] = oc101;
    cached_val[1][1][0] = oc110;
    cached_val[1][1][1] = oc111;
  }
#endif
  
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

  return octMethodOnLevel(chom,0.f,pos);
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
#if 1
  self->super.computeSample    = &Chombo_octantMethodProper;
#elif 0
  self->super.computeSample    = &Chombo_octantMethodSimple;
#elif 0
  self->super.computeSample    = &Chombo_lerpOnFinestLevel;
#else
  self->super.computeSample    = &Chombo_nearestNeighbor;
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




