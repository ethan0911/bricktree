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





/*! interpolate on the finest level */
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
                 : leaf.pos.x + leaf.cellWidth + hfcw,
                 pos.y < leafCenter.y
                 ? leaf.pos.y - hfcw
                 : leaf.pos.y + leaf.cellWidth + hfcw,
                 pos.z < leafCenter.z
                 ? leaf.pos.z - hfcw
                 : leaf.pos.z + leaf.cellWidth + hfcw);

  CellRef cell[2][2][2];
  cell[0][0][0] = leaf; //findLeafCell(chom,make_vec3f(pos.x,pos.y,pos.z);
  cell[0][0][1] = findLeafCell(chom,make_vec3f(loc.x,pos.y,pos.z));
  cell[0][1][0] = findLeafCell(chom,make_vec3f(pos.x,loc.y,pos.z));
  cell[0][1][1] = findLeafCell(chom,make_vec3f(loc.x,loc.y,pos.z));
  cell[1][0][0] = findLeafCell(chom,make_vec3f(pos.x,pos.y,loc.z));
  cell[1][0][1] = findLeafCell(chom,make_vec3f(loc.x,pos.y,loc.z));
  cell[1][1][0] = findLeafCell(chom,make_vec3f(pos.x,loc.y,loc.z));
  cell[1][1][1] = findLeafCell(chom,make_vec3f(loc.x,loc.y,loc.z));

  const float dx = 2.f*abs(pos.x-leafCenter.x)/leaf.cellWidth;
  const float dy = 2.f*abs(pos.y-leafCenter.y)/leaf.cellWidth;
  const float dz = 2.f*abs(pos.z-leafCenter.z)/leaf.cellWidth;

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
  self->super.computeSample    = &Chombo_octantMethodSimple;
#elif 1
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




