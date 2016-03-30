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

// ospray
#include "ospray/math/vec.ih"
#include "ospray/math/AffineSpace.ih"
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"
// this module
#include "ChomboVolume.ih"
// embree
#include "embree2/rtcore.isph"

/*! plainest, dumbest way of reconstructing: find the given leaf cell,
    and return the value */
extern varying float Chombo_nearestNeighbor(void *uniform _self, 
                                            const varying vec3f &worldSpacePos)
{
  Chombo *uniform self = (Chombo *)_self;
  // for now, let's assume the world-space position already IS in grid space
  const vec3f gridPos = worldSpacePos;
  float pos[3];
  pos[0] = gridPos.x;
  pos[1] = gridPos.y;
  pos[2] = gridPos.z;
  
  if (getPos(self->node[0]) > pos[getDim(self->node[0])])
    return .3f;
  else
    return .7f;
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
  Chombo *uniform self = (Chombo *uniform) _self;

  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const uniform float step
    = self->super.samplingStep / self->super.samplingRate;

  ray.t0 += step;
}



export void *uniform ChomboVolume_create(void *uniform cppE)
{
  Chombo *uniform self = uniform new uniform Chombo;
  // self->super.cppE = cppE;
  return self;
} 

export void ChomboVolume_set(void *uniform _self,
                             void *uniform _xf,
                             uniform vec3i &rootGridDims,
                             void *uniform _brick_array,
                             void *uniform _tree_node,
                             void *uniform _tree_item)
{
  Chombo *uniform self = (Chombo *uniform)_self;
  assert(self);
  
  self->super.boundingBox.lower = make_vec3f(0.f);
  self->super.boundingBox.upper = make_vec3f(rootGridDims); 
  
  self->super.volumeClippingBox.lower = make_vec3f(0.f);
  self->super.volumeClippingBox.upper = make_vec3f(rootGridDims); 
  
  self->super.samplingRate = 1.f;//samplingRate;
  self->super.samplingStep = .1f;

  self->super.gradientShadingEnabled = 0;

  self->super.transferFunction = (TransferFunction *uniform)_xf;

  self->brick  = (ChomboBrick  *)_brick_array;
  self->node   = (KDTreeNode   *)_tree_node;
  self->item   = (ChomboBrick **)_tree_item;
  self->rootGridDims = rootGridDims;
  // self->finestCellWidth = finestCellWidth;

  self->super.intersect       = &Chombo_intersect;
  self->super.computeGradient = &Chombo_gradient;
  self->super.computeSample   = &Chombo_nearestNeighbor;
}




