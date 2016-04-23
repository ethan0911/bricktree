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




// ------------------------------------------------------------------
/*! plainest, dumbest way of reconstructing: find the given leaf cell,
  and return the value */
// ------------------------------------------------------------------
extern float Chombo_debug(void *uniform _self, const vec3f &pos)
{ return 0.f; }

// ------------------------------------------------------------------
/*! interpolate on the finest level */
// ------------------------------------------------------------------
extern float Chombo_lerpOnFinestLevel(void *uniform _self, const vec3f &pos);

// ------------------------------------------------------------------
/*! plainest, dumbest way of reconstructing: find the given leaf cell,
  and return the value */
// ------------------------------------------------------------------
extern float Chombo_nearestNeighbor(void *uniform _self, const vec3f &pos)
{
  ChomboVolume *uniform self = (ChomboVolume *)_self;
  CellRef leaf = findLeafCell(&self->chombo,pos);
  return leaf.value;
}

// ------------------------------------------------------------------
/*! the simplest variant of an octant method - determines the octant
  of the leaf cell the poitn is in, then finds the 8 neighboring
  dual octants of same or coarser level. If NOT at a level boundary
  that is the same as interpolating at the given level; across a
  cell boundary it is NOT smooth, but close. */
// ------------------------------------------------------------------
extern float Chombo_octIgnoreBoundaries(void *uniform _self, const vec3f &pos);

// ------------------------------------------------------------------
/* the octant method in which the boundary is defined by the coarser
   side - this is 'correct' in that it is interpolating and continuous
   even across level boundaries. it can, however, lead to weird
   patterns at boundaries in that the coarse levels can 'bleed' across
   the boundary.

   To explain this pattern, imagine a boundary between a coarse region
   on the left and a fine one on the right. The left has a constant 0,
   the right is refined to (0,1,1,1). The fine region would also have
   a representation on the coarse level, which would be
   0.75=avg(0,1,1,1). Now the value we'd expect at the boundary is 0,
   because both the coarse cell on the left is 0, as well as the
   left-most of the fine values. However, at the boundary we'd let
   interpolation on the coarse level define the value, which would be
   the average of the coarse cell on the left (0) and that on the
   right (0.75), which is 0.385. Ie, gonig from the left to the right
   we'd see a function that is first 0 (in the center of the coarse
   cell), would then interpolate up to 0.385 (interpolated value on
   coarse level) at the boundary, would then interpolate back down to
   0 again (left-most value on fine level), and then interpolate back
   up to 1 (next value(s) on the fine level). This 'ghost' spike at
   0.38 gives a visible artifact.
 */
// ------------------------------------------------------------------
extern float Chombo_octFromCoarseLevel(void *uniform _self, const vec3f &pos);


// ------------------------------------------------------------------
/*! "blend from coarser levels". check if we're in a uniform region,
    and if so do lerp. if not, blend values from lower level. NOTE
    THAT QUALITATIVELY THIS IS THE SAME AS ONE OF THE OCT METHODS */
// ------------------------------------------------------------------
extern float Chombo_blendFromCoarser(void *uniform _self, const vec3f &pos);

// ------------------------------------------------------------------
/*! a 'octant from coarse' method that defines the boundary of two
    octants through the coarest of all adjacent levels. unlike the
    'level'-based oct-from-coarse method, this one looks only at the
    corners of the octant, and defines an intersection of octants as
    the average of all cells that abut there, even if those are on
    different levels. ie, it will NOT always use values from a coarser
    level */
// ------------------------------------------------------------------
extern float Chombo_octFromCoarseCellValue(void *uniform _self, const vec3f &pos);
extern float Chombo_octFromCoarseCorners(void *uniform _self, const vec3f &pos);









// ------------------------------------------------------------------
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
                             const void *uniform _brick_array,
                             const void *uniform _tree_node,
                             uniform int32 numTreeNodes,
                             const void *uniform _tree_leaf,
                             uniform int32 numLeaves,
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
#elif 1
  // works
  self->super.computeSample    = &Chombo_lerpOnFinestLevel;
#elif 0
  self->super.computeSample    = &Chombo_blendFromCoarser;
#elif 0
  self->super.computeSample    = &Chombo_octIgnoreBoundaries;
#elif 0
  self->super.computeSample    = &Chombo_octFromCoarseCorners;
#elif 1
  self->super.computeSample    = &Chombo_octFromCoarseCellValue;
#elif 1
  self->super.computeSample    = &Chombo_octFromCoarseLevel;
#elif 1
  self->super.computeSample    = &Chombo_octantMethodProper;
#else
# error no sample() method selected...
#endif

  // self->chombo.brick           = (ChomboBrick  *)_brick_array;
  self->chombo.node            = (KDTreeNode   *)_tree_node;
  self->chombo.numNodes        = numTreeNodes;
  self->chombo.leaf            = (ChomboLeaf *)_tree_leaf;
  self->chombo.numLeaves       = numLeaves;
  self->chombo.rootGridDims    = rootGridDims;

  self->chombo.bounds.lower    = make_vec3f(0);
  self->chombo.bounds.upper    = make_vec3f(rootGridDims);
  self->chombo.finestLevelCellWidth = finestLevelCellWidth;
  print("#osp:amr: chombo volume set\n");
  
  //  buildNodeRanges(self);
}




