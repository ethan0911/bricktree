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
#include "CellDesc.ih"
#include "CellQuery.ih"
// embree
#include "embree2/rtcore.isph"



struct TentAlgoStackEntry {
  int32 blockID;
  vec3f blockPos;
  float blockWidth;
};

inline float tentApproximation_packet(MSAMRVolume *uniform self,
                                      const varying vec3f &worldSpacePos)
{
  GridPos gp = makeGridPos(self,toGridSpace(self,worldSpacePos));
  
  // "packet" variant
  uniform TentAlgoStackEntry stackBase[100];
  uniform TentAlgoStackEntry *uniform stackPtr = stackBase;
  
  // init numerator and denominator
  float num = 0.f;
  float den = 0.f;
  
  // -------------------------------------------------------
  // do initial stack fill
  // ------------------------------------------------------- 
  // - compute region of cells in root grid
  vec3i root_beg = make_vec3i(floor(gp.pos - make_vec3f(.5f)));
  vec3i root_end = make_vec3i(root_beg+make_vec3i(2));
  root_beg = max(root_beg,make_vec3i(0));
  root_end = min(root_end,self->rootGridDims);
  
  // now, throw the 2x2x2 possibly overlapping root cells on the stack.
  for (int _iz=root_beg.z; _iz < root_end.z; _iz++) {
    foreach_unique (tree_iz in _iz) {
      for (int _iy=root_beg.y; _iy < root_end.y; _iy++) {
        foreach_unique (tree_iy in _iy) {
          for (int _ix=root_beg.x; _ix < root_end.x; _ix++) {
            foreach_unique (tree_ix in _ix) {
              const uniform int rootCellID
                = tree_ix + self->rootGridDims.x*(tree_iy+self->rootGridDims.y*(tree_iz));

              uniform SumDataBlock *uniform dataBlockArray
                = self->dataBlock+self->firstDataBlockOfTree[rootCellID];
              uniform SumIndexBlock *uniform indexBlockArray
                = self->indexBlock+self->firstIndexBlockOfTree[rootCellID];
              uniform int32 *uniform indexBlockOfArray 
                = (uniform int32*)self->blockInfo + self->firstDataBlockOfTree[rootCellID];
              stackPtr->blockPos = make_vec3f(tree_ix,tree_iy,tree_iz);
              stackPtr->blockWidth = 1.f;
              stackPtr->blockID = 0; 
              stackPtr++;
              
              // -------------------------------------------------------
              // process stack nodes...
              // -------------------------------------------------------
              // note we do NOT know if the stack entries actually
              // overlap - that makes the pushing easier.
              while (stackPtr > stackBase) {
                -- stackPtr;
                uniform TentAlgoStackEntry cur = *stackPtr;
                
                const vec3f base = make_vec3f(cur.blockPos);
                const float blockWidth  = cur.blockWidth;
                uniform int blockID = cur.blockID;
                SumDataBlock *uniform dataBlock = dataBlockArray + blockID;

                SumIndexBlock *uniform indexBlock = NULL;
                const uniform int indexBlockID = indexBlockOfArray[blockID];
                if (indexBlockID >= 0)
                  indexBlock = indexBlockArray+indexBlockID;

                const uniform float childCellWidth = 0.25f * cur.blockWidth;
                uniform vec3f childCellPos;
                vec3f delta;
                for (uniform int iiz=0;iiz<4;iiz++) {
                  childCellPos.z = cur.blockPos.z + iiz * childCellWidth;
                  delta.z = abs(childCellPos.z + 0.5f * childCellWidth - gp.pos.z);
                  if (delta.z < childCellWidth)
                    for (uniform int iiy=0;iiy<4;iiy++) {
                      childCellPos.y = cur.blockPos.y + iiy * childCellWidth;
                      delta.y = abs(childCellPos.y + 0.5f * childCellWidth - gp.pos.y);
                      if (delta.y < childCellWidth)
                        for (uniform int iix=0;iix<4;iix++) {
                          childCellPos.x = cur.blockPos.x + iix * childCellWidth;
                          delta.x = abs(childCellPos.x + 0.5f * childCellWidth - gp.pos.x);
                          
                          if (delta.x < childCellWidth) {
                            // -------------------------------------------------------
                            // OK, this is a cell that at least one ray overlaps!
                            // -------------------------------------------------------
                            if (indexBlock && (indexBlock->child[iiz][iiy][iix] >= 0)) {
                              stackPtr->blockPos = childCellPos;
                              stackPtr->blockWidth = childCellWidth;
                              stackPtr->blockID = indexBlock->child[iiz][iiy][iix];
                              ++stackPtr;
                            } else {
                              const vec3f weight = max(make_vec3f(0.f),
                                                       make_vec3f(1.f) - delta * rcp(childCellWidth));
                              const float w = reduce_mul(weight);
                              den += w;
                              num += w * dataBlock->value[iiz][iiy][iix];
                            }
                          }
                        }
                    }
                }
              }
            }
          }
        }
      }
    }
  }

  const float result = num / (den + 1e-20f);
  return result;
}
                               

//! The value at the given sample location in world coordinates.
static varying float MSAMR_tentApprox(void *uniform _self, 
                                            const varying vec3f &worldSpacePos)
{
  MSAMRVolume *uniform self = (MSAMRVolume *uniform)_self;
  float f =  tentApproximation_packet(self,worldSpacePos);
  return f;
}

/*! plainest, dumbest way of reconstructing: find the given leaf cell,
    and return the value */
extern varying float MSAMR_nearestNeighbor(void *uniform _self, 
                                           const varying vec3f &worldSpacePos);

/*! perform tri-linear interpolation on the finest cell grid level -
  ie, coarse regions will be entirely flat inside the coarse region, 
  and have a linar fine-cell blend where they abut */
extern varying float MSAMR_interpolateOnFinestLevel(void *uniform _self, 
                                                    const varying vec3f &worldSpacePos);

/*! perform tri-linear interpolation on the level of the leaf cell
  that the sample is in.  note that THIS IS NOT CORRECT across level
  boundaries, as a sample may be in a corse cell, but its neighbor
  might be subdivided, which this one ignores */
extern varying float 
MSAMR_interpolateOnCurrentCellLevel(void *uniform _self, 
                                    const varying vec3f &worldSpacePos);

//! The gradient at the given sample location in world coordinates.
static varying vec3f MSAMRVolume_gradient(void *uniform _self, 
                                        const varying vec3f &worldCoordinates)
{
  return make_vec3f(1,0,0);
}

//! Find the next hit point in the volume for ray casting based renderers.
static void MSAMRVolume_intersect(void *uniform _self, 
                                varying Ray &ray)
{
  MSAMRVolume *uniform self = (MSAMRVolume *uniform) _self;
  
  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const uniform float step
    = self->super.samplingStep / self->super.samplingRate;
  ray.t0 += step;
}

export void *uniform MSAMRVolume_create(void *uniform cppEquivalent) 
{
  MSAMRVolume *uniform self = uniform new uniform MSAMRVolume;
  Volume_Constructor(&self->super,cppEquivalent);

  return self;
};

export void MSAMRVolume_set(void *uniform _self,
                            void *uniform _xf,
                            const uniform vec3i &rootGridDims,
                            const uniform vec3f &validFractionOfRootGrid,
                            void *uniform dataBlock,
                            void *uniform indexBlock,
                            void *uniform blockInfo,
                            void *uniform firstDataBlockOfTreeData,
                            void *uniform firstIndexBlockOfTreeData,
                            uniform float finestCellWidth)
{
  MSAMRVolume *uniform self = (MSAMRVolume *uniform)_self;
  assert(self);
  
  self->super.boundingBox.lower = make_vec3f(0.f);
  self->super.boundingBox.upper = make_vec3f(rootGridDims)*validFractionOfRootGrid; 
  
  self->super.volumeClippingBox.lower = make_vec3f(0.f);
  self->super.volumeClippingBox.upper = make_vec3f(rootGridDims)*validFractionOfRootGrid; 
  
  self->super.computeGradient = &MSAMRVolume_gradient;

  self->super.samplingRate = 1.f;//samplingRate;
  self->super.samplingStep = .1f;

  self->super.gradientShadingEnabled = 0;

  self->super.transferFunction = (TransferFunction *uniform)_xf;

  self->dataBlock  = (SumDataBlock  *)dataBlock;
  self->indexBlock = (SumIndexBlock *)indexBlock;
  self->blockInfo  = (SumBlockInfo  *)blockInfo;
  self->firstIndexBlockOfTree = (int32*)firstIndexBlockOfTreeData;
  self->firstDataBlockOfTree  = (int32*)firstDataBlockOfTreeData;
  // root grid dimensions
  self->rootGridDims = rootGridDims;
  self->validFractionOfRootGrid = validFractionOfRootGrid;
  self->finestCellWidth = finestCellWidth;

  self->super.intersect = &MSAMRVolume_intersect;
  // self->super.computeSample = &MSAMR_nearestNeighbor;
  // self->super.computeSample = &MSAMR_tentApprox;
  self->super.computeSample = &MSAMR_interpolateOnCurrentCellLevel;
}



