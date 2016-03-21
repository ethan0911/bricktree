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
#include "MultiSum.ih"
// embree
#include "embree2/rtcore.isph"

struct TentAlgoStackEntry {
  int32 nodeID;
  vec3f blockPos;
  float blockWidth;
};

inline float tentApproximation_packet(MultiSumAMRVolume *uniform self,
                                      const varying vec3f &worldCoordinates)
{
  // "packet" variant
  uniform TentAlgoStackEntry stackBase[100];
  uniform TentAlgoStackEntry *uniform stackPtr = stackBase;
  
  const vec3f gridPos = worldCoordinates;
  
  // init numerator and denominator
  float num = 0.f;
  float den = 0.f;
  
  // -------------------------------------------------------
  // do initial stack fill
  // ------------------------------------------------------- 
  // - compute region of cells in root grid
  vec3i root_beg = make_vec3i(floor(gridPos - make_vec3f(.5f)));
  vec3i root_end = make_vec3i(root_beg+make_vec3i(2));
  root_beg = max(root_beg,make_vec3i(0));
  root_end = min(root_end,self->rootGridDims);
  
  // now, throw the 2x2x2 possibly overlapping root cells on the stack.
  for (int _iz=root_beg.z; _iz < root_end.z; _iz++) {
    foreach_unique (iz in _iz) {
      for (int _iy=root_beg.y; _iy < root_end.y; _iy++) {
        foreach_unique (iy in _iy) {
          for (int _ix=root_beg.x; _ix < root_end.x; _ix++) {
            foreach_unique (ix in _ix) {
              const uniform int rootCellID
                = ix + self->rootGridDims.x*(iy+self->rootGridDims.y*(iz));
              // stackPtr->cell = self->rootCell+rootCellID;
              stackPtr->blockPos = make_vec3f(ix,iy,iz);
              stackPtr->blockWidth = 1.f;
              stackPtr->nodeID = rootCellID;
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
                // const vec3f cellCenter = base+0.5f*blockWidth;
                // const vec3f delta = absf(cellCenter - gridPos);
                uniform int nodeID = cur.nodeID;
                SumDataBlock *uniform dataBlock = self->dataBlock + nodeID;
                SumIndexBlock *uniform indexBlock = NULL;
                const uniform int indexBlockID = self->blockInfo[nodeID].indexBlockID;
                if (indexBlockID >= 0) 
                  indexBlock = self->indexBlock + indexBlockID;
                // this is a entire block of child cells ...
                
                // if (cur.cell->childID < 0) {
                //   // leaf node ...
                //   const vec3f weight = max(make_vec3f(0.f),
                //                            make_vec3f(1.f) - delta * rcp(blockWidth));
                //   const float w = reduce_mul(weight);
                //   den += w;
                //   num += w * cur.cell->ccValue;
                // } else {
                  
                  // if (reduce_max(delta) >= blockWidth) 
                  //   continue;
                const uniform float childCellWidth = 0.25f * cur.blockWidth;
                uniform vec3f childCellPos;
                vec3f delta;
                for (uniform int iiz=0;iiz<4;iiz++) {
                  childCellPos.z = cur.blockPos.z + iiz * childCellWidth;
                  delta.z = abs(childCellPos.z + 0.5f * childCellWidth - gridPos.z);
                  if (delta.z < childCellWidth)
                    for (uniform int iiy=0;iiy<4;iiy++) {
                      childCellPos.y = cur.blockPos.y + iiy * childCellWidth;
                      delta.y = abs(childCellPos.y + 0.5f * childCellWidth - gridPos.y);
                      if (delta.y < childCellWidth)
                        for (uniform int iix=0;iix<4;iix++) {
                          childCellPos.x = cur.blockPos.x + iix * childCellWidth;
                          delta.x = abs(childCellPos.x + 0.5f * childCellWidth - gridPos.x);
                          if (delta.x < childCellWidth) {
                            // -------------------------------------------------------
                            // OK, this is a cell that at least one ray overlaps!
                            // -------------------------------------------------------
                            if (indexBlock && (indexBlock->child[iz][iy][ix] >= 0)) {
                              stackPtr->blockPos = childCellPos;
                              stackPtr->blockWidth = childCellWidth;
                              stackPtr->nodeID = indexBlock->child[iz][iy][ix];
                              ++stackPtr;
                            } else {
                              const vec3f weight = max(make_vec3f(0.f),
                                                       make_vec3f(1.f) - delta * rcp(childCellWidth));
                              const float w = reduce_mul(weight);
                              den += w;
                              num += w * dataBlock->value[iz][iy][ix];
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
  
  return num / (den + 1e-8f);
}
                               

//! The value at the given sample location in world coordinates.
static varying float MultiSumAMRVolume_sample(void *uniform _self, 
                                              const varying vec3f &worldCoordinates)
{
  MultiSumAMRVolume *uniform self = (MultiSumAMRVolume *uniform)_self;
  assert(self);
  
  float f =  tentApproximation_packet(self,worldCoordinates);
  if (reduce_max(worldCoordinates) > .9f)
    print("wc\n %\n %\n %\n-> %\n",worldCoordinates.x,worldCoordinates.y,worldCoordinates.z,f);
  return f;
}

//! The gradient at the given sample location in world coordinates.
static varying vec3f MultiSumAMRVolume_gradient(void *uniform _self, 
                                        const varying vec3f &worldCoordinates)
{
  return make_vec3f(1,0,0);
}

//! Find the next hit point in the volume for ray casting based renderers.
static void MultiSumAMRVolume_intersect(void *uniform _self, 
                                varying Ray &ray)
{
  MultiSumAMRVolume *uniform self = (MultiSumAMRVolume *uniform) _self;
  
  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const uniform float step
    = self->super.samplingStep / self->super.samplingRate;
  
  ray.t0 += step;
}

export void *uniform MultiSumAMRVolume_create(void *uniform cppEquivalent) 
{
  MultiSumAMRVolume *uniform self = uniform new uniform MultiSumAMRVolume;
  Volume_Constructor(&self->super,cppEquivalent);

  return self;
};

export void MultiSumAMRVolume_set(void *uniform _self,
                                  void *uniform _xf,
                                  const uniform vec3i &rootGridDims,
                                  const uniform vec3f &validFractionOfRootGrid,
                                  void *uniform indexBlock,
                                  void *uniform dataBlock,
                                  void *uniform blockInfo,
                                  void *uniform rootCell)
{
  MultiSumAMRVolume *uniform self = (MultiSumAMRVolume *uniform)_self;
  assert(self);
  
  self->super.boundingBox.lower = make_vec3f(0.f);
  self->super.boundingBox.upper = make_vec3f(rootGridDims)*validFractionOfRootGrid; 
  
  self->super.volumeClippingBox.lower = make_vec3f(0.f);
  self->super.volumeClippingBox.upper = make_vec3f(rootGridDims)*validFractionOfRootGrid; 
  
  self->super.intersect = &MultiSumAMRVolume_intersect;
  self->super.computeSample = &MultiSumAMRVolume_sample;
  self->super.computeGradient = &MultiSumAMRVolume_gradient;

  self->super.samplingRate = 1.f;
  self->super.samplingStep = 1.f;

  self->super.gradientShadingEnabled = 0;

  self->super.transferFunction = (TransferFunction *uniform)_xf;

  self->indexBlock = (SumIndexBlock *)indexBlock;
  self->dataBlock  = (SumDataBlock  *)dataBlock;
  self->blockInfo  = (SumBlockInfo  *)blockInfo;
  self->rootCell   = (uint32        *)rootCell;
  // root grid dimensions
  self->rootGridDims = rootGridDims;
  self->validFractionOfRootGrid = validFractionOfRootGrid;
}



