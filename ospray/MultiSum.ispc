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

inline float tentApproximation_packet(MultiSumAMRVolume *uniform self,
                                      const varying vec3f &worldCoordinates)
{
  // // "packet" variant
  // uniform TentAlgoStackEntry stackBase[100];
  // uniform TentAlgoStackEntry *uniform stackPtr = stackBase;

  // const vec3f gridPos = worldCoordinates;

  // // init numerator and denominator
  // float num = 0.f;
  // float den = 0.f;
  
  // // -------------------------------------------------------
  // // do initial stack fill
  // // ------------------------------------------------------- 
  // // - compute region of cells in root grid
  // vec3i root_beg = make_vec3i(floor(gridPos - make_vec3f(.5f)));
  // vec3i root_end = make_vec3i(root_beg+make_vec3i(2));
  // root_beg = max(root_beg,make_vec3i(0));
  // root_end = min(root_end,self->rootDims);

  // // now, throw the 2x2x2 possibly overlapping root cells on the stack.
  // for (int _iz=root_beg.z; _iz < root_end.z; _iz++) {
  //   foreach_unique (iz in _iz) {
  //     for (int _iy=root_beg.y; _iy < root_end.y; _iy++) {
  //       foreach_unique (iy in _iy) {
  //         for (int _ix=root_beg.x; _ix < root_end.x; _ix++) {
  //           foreach_unique (ix in _ix) {
  //             const uniform int rootCellID
  //               = ix + self->rootDims.x*(iy+self->rootDims.y*(iz));
  //             stackPtr->cell = self->rootCell+rootCellID;
  //             stackPtr->basePos = make_vec3f(ix,iy,iz);
  //             stackPtr->cellWidth = 1.f;
  //             stackPtr++;
              
  //             // -------------------------------------------------------
  //             // process stack nodes...
  //             // -------------------------------------------------------
  //             // note we do NOT know if the stack entries actually
  //             // overlap - that makes the pushing easier.
  //             while (stackPtr > stackBase) {
  //               -- stackPtr;
  //               uniform TentAlgoStackEntry cur = *stackPtr;
                
  //               const vec3f base = make_vec3f(cur.basePos);
  //               const float cellWidth  = cur.cellWidth;
  //               const vec3f cellCenter = base+0.5f*cellWidth;
  //               const vec3f delta = absf(cellCenter - gridPos);
                
  //               if (cur.cell->childID < 0) {
  //                 // leaf node ...
  //                 const vec3f weight = max(make_vec3f(0.f),
  //                                          make_vec3f(1.f) - delta * rcp(cellWidth));
  //                 const float w = reduce_mul(weight);
  //                 den += w;
  //                 num += w * cur.cell->ccValue;
  //               } else {
                
  //                 // if (reduce_max(delta) >= cellWidth) 
  //                 //   continue;
  //                 uniform AMROctCell *uniform oc = self->octCell+cur.cell->childID;
  //                 const uniform float newCellWidth = 0.5f * cur.cellWidth;
  //                 uniform vec3f newBasePos;
  //                 for (uniform int iiz=0;iiz<2;iiz++) {
  //                   newBasePos.z = cur.basePos.z + (iiz?newCellWidth:0.f);
  //                   if (abs(newBasePos.z + 0.5f *newCellWidth - gridPos.z) < newCellWidth)
  //                     for (uniform int iiy=0;iiy<2;iiy++) {
  //                       newBasePos.y = cur.basePos.y + (iiy?newCellWidth:0.f);
  //                       if (abs(newBasePos.y + 0.5f *newCellWidth - gridPos.y) < newCellWidth)
  //                         for (uniform int iix=0;iix<2;iix++) {
  //                           newBasePos.x = cur.basePos.x + (iix?newCellWidth:0.f);
  //                           if (abs(newBasePos.x + 0.5f *newCellWidth - gridPos.x) < newCellWidth)
  //                             {
  //                               stackPtr->basePos = newBasePos;
  //                               stackPtr->cellWidth = newCellWidth;
  //                               stackPtr->cell = &oc->child[iiz][iiy][iix];
  //                               ++stackPtr;
  //                             }
  //                         }
  //                     }
  //                 }
  //               }
  //             }
  //           }
  //         }
  //       }
  //     }
  //   }
  // }

  // return num / (den + 1e-8f);
  return 0.f;
}
                               

//! The value at the given sample location in world coordinates.
static varying float MultiSumAMRVolume_sample(void *uniform _self, 
                                              const varying vec3f &worldCoordinates)
{
  MultiSumAMRVolume *uniform self = (MultiSumAMRVolume *uniform)_self;
  assert(self);
  
  return tentApproximation_packet(self,worldCoordinates);
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



