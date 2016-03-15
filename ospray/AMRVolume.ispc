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
#include "AMRVolume.ih"
// embree
#include "embree2/rtcore.isph"


#define AMR_ISO_SURFACE 0


export void *uniform AMRVolume_create(void *uniform cppEquivalent) 
{
  AMRVolume *uniform self = uniform new uniform AMRVolume;
  Volume_Constructor(&self->super,cppEquivalent);

  return self;
};

// extern "C" uniform float AMR_sample_scalar(void *uniform self, uniform vec3f *uniform v);


struct TentAlgoStackEntry {
  uniform AMRCell *uniform cell;
  uniform float cellWidth;
  // lower-left-front coordinate (in grid space) of cell
  uniform vec3f basePos;
};

/*! struct for a cached octant - not used yet */
struct CachedOctant {
  box3f bounds;
  float corner[2][2][2];
};





inline uniform float tentApproximation_single(AMRVolume *uniform self,
                                              const uniform vec3f worldCoordinates)
{
  uniform TentAlgoStackEntry stackBase[100];
  uniform TentAlgoStackEntry *uniform stackPtr = stackBase;

  // // init numerator and denominator
  float num = 0.f;
  float den = 0.f;
  const uniform vec3f gridPos = worldCoordinates;

  // -------------------------------------------------------
  // do initial stack fill
  // ------------------------------------------------------- 
  // - compute region of cells in root grid
  uniform vec3i root_beg = make_vec3i(floor(gridPos - make_vec3f(.5f)));
  uniform vec3i root_end = make_vec3i(root_beg+make_vec3i(2));
  root_beg = max(root_beg,make_vec3i(0));
  root_end = min(root_end,self->rootDims);
  
  // now, throw the 2x2x2 possibly overlapping root cells on the stack.
  for (uniform int iz=root_beg.z; iz < root_end.z; iz++) {
    for (uniform int iy=root_beg.y; iy < root_end.y; iy++) {
      for (uniform int ix=root_beg.x; ix < root_end.x; ix++) {
        const uniform int rootCellID
          = ix + self->rootDims.x*(iy+self->rootDims.y*(iz));
        stackPtr->cell = self->rootCell+rootCellID;
        stackPtr->basePos = make_vec3f(ix,iy,iz);
        stackPtr->cellWidth = 1.f;
        stackPtr++;
        
        // -------------------------------------------------------
        // process stack nodes...
        // -------------------------------------------------------
        // note we do NOT know if the stack entries actually
        // overlap - that makes the pushing easier.
        while (stackPtr > stackBase) {
          -- stackPtr;
          uniform TentAlgoStackEntry cur = *stackPtr;
          
          const vec3f base = make_vec3f(cur.basePos);
          const float cellWidth  = cur.cellWidth;
          const vec3f cellCenter = base+0.5f*cellWidth;
          const vec3f delta = absf(cellCenter - gridPos);
          
          if (reduce_max(delta) >= cellWidth) 
            continue;
          
          if (cur.cell->childID < 0) {
            // leaf node ...
            const vec3f weight = make_vec3f(1.f) - delta * rcp(cellWidth);
            const float w = reduce_mul(weight);
            den += w;
            num += w * cur.cell->ccValue;
          } else {
            uniform AMROctCell *uniform oc = self->octCell+cur.cell->childID;
            uniform vec3f newBasePos;
            const uniform float newCellWidth = 0.5f * cur.cellWidth;
            for (uniform int iiz=0;iiz<2;iiz++) {
              newBasePos.z = cur.basePos.z + (iiz?newCellWidth:0.f);
              for (uniform int iiy=0;iiy<2;iiy++) {
                newBasePos.y = cur.basePos.y + (iiy?newCellWidth:0.f);
                for (uniform int iix=0;iix<2;iix++) {
                  newBasePos.x = cur.basePos.x + (iix?newCellWidth:0.f);
                  stackPtr->basePos = newBasePos;
                  stackPtr->cellWidth = newCellWidth;
                  stackPtr->cell = &oc->child[iiz][iiy][iix];
                  ++stackPtr;
                }
              }
            }
          }
        }
      }
    }
  }
  
  return reduce_add(num) / reduce_add(den+1e-8f);
}
inline float tentApproximation_srs(AMRVolume *uniform self,
                                   const varying vec3f &worldCoordinates)
{
  uniform float wx[programCount];
  uniform float wy[programCount];
  uniform float wz[programCount];
  unmasked {
    wx[programIndex] = worldCoordinates.x; 
    wy[programIndex] = worldCoordinates.y; 
    wz[programIndex] = worldCoordinates.z; 
  }

  uniform float ret[programCount];
  foreach_active(lane)
    ret[lane] = tentApproximation_single(self,make_vec3f(wx[lane],wy[lane],wz[lane]));
  return ret[programIndex];
}


inline float tentApproximation_packet(AMRVolume *uniform self,
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
  root_end = min(root_end,self->rootDims);

  // now, throw the 2x2x2 possibly overlapping root cells on the stack.
  for (int _iz=root_beg.z; _iz < root_end.z; _iz++) {
    foreach_unique (iz in _iz) {
      for (int _iy=root_beg.y; _iy < root_end.y; _iy++) {
        foreach_unique (iy in _iy) {
          for (int _ix=root_beg.x; _ix < root_end.x; _ix++) {
            foreach_unique (ix in _ix) {
              const uniform int rootCellID
                = ix + self->rootDims.x*(iy+self->rootDims.y*(iz));
              stackPtr->cell = self->rootCell+rootCellID;
              stackPtr->basePos = make_vec3f(ix,iy,iz);
              stackPtr->cellWidth = 1.f;
              stackPtr++;
              
              // -------------------------------------------------------
              // process stack nodes...
              // -------------------------------------------------------
              // note we do NOT know if the stack entries actually
              // overlap - that makes the pushing easier.
              while (stackPtr > stackBase) {
                -- stackPtr;
                uniform TentAlgoStackEntry cur = *stackPtr;
                
                const vec3f base = make_vec3f(cur.basePos);
                const float cellWidth  = cur.cellWidth;
                const vec3f cellCenter = base+0.5f*cellWidth;
                const vec3f delta = absf(cellCenter - gridPos);
                
                if (reduce_max(delta) >= cellWidth) 
                  continue;
                
                if (cur.cell->childID < 0) {
                  // leaf node ...
                  const vec3f weight = make_vec3f(1.f) - delta * rcp(cellWidth);
                  const float w = reduce_mul(weight);
                  den += w;
                  num += w * cur.cell->ccValue;
                } else {
                  uniform AMROctCell *uniform oc = self->octCell+cur.cell->childID;
                  uniform vec3f newBasePos;
                  const uniform float newCellWidth = 0.5f * cur.cellWidth;
                  for (uniform int iiz=0;iiz<2;iiz++) {
                    newBasePos.z = cur.basePos.z + (iiz?newCellWidth:0.f);
                    for (uniform int iiy=0;iiy<2;iiy++) {
                      newBasePos.y = cur.basePos.y + (iiy?newCellWidth:0.f);
                      for (uniform int iix=0;iix<2;iix++) {
                        newBasePos.x = cur.basePos.x + (iix?newCellWidth:0.f);
                        stackPtr->basePos = newBasePos;
                        stackPtr->cellWidth = newCellWidth;
                        stackPtr->cell = &oc->child[iiz][iiy][iix];
                        ++stackPtr;
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
static varying float AMRVolume_sample(void *uniform _self, 
                                      const varying vec3f &worldCoordinates)
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;
  assert(self);

#if 1
  return tentApproximation_packet(self,worldCoordinates);
  //  return tentApproximation_srs(self,worldCoordinates);
#else
  /*! iw: for now we're just passing everything back to c, so we don't
    have to deal with new algorithms AND ispc at the same time
    ... */
  uniform float x[programCount];
  uniform float y[programCount];
  uniform float z[programCount];
  
  x[programIndex] = worldCoordinates.x;
  y[programIndex] = worldCoordinates.y;
  z[programIndex] = worldCoordinates.z;
  uniform float res[programCount];
  
  // print("sample\n%\n%\n%\n",worldCoordinates.x,worldCoordinates.y,worldCoordinates.z);
  
  foreach_active(i) {
    uniform vec3f where = make_vec3f(x[i],y[i],z[i]);
    res[i] = AMR_sample_scalar(self->super.cppEquivalent,&where);
  }
  return res[programIndex];
#endif
}

//! The gradient at the given sample location in world coordinates.
static varying vec3f AMRVolume_gradient(void *uniform _self, 
                                        const varying vec3f &worldCoordinates)
{
  return make_vec3f(1,0,0);
}

//! Find the next hit point in the volume for ray casting based renderers.
static void AMRVolume_intersect(void *uniform _self, 
                                varying Ray &ray)
{
  AMRVolume *uniform self = (AMRVolume *uniform) _self;

  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const uniform float step
    = self->super.samplingStep / self->super.samplingRate;

  ray.t0 += step;
}

#if AMR_ISO_SURFACE
void computeTreeRanges(AMRVolume *uniform self)
{
  for (uniform int iz=0;iz<self->rootDims.z;iz++)
    for (uniform int iy=0;iy<self->rootDims.y;iy++)
      for (uniform int ix=0;ix<self->rootDims.x;ix++) {
        // TODO
        //        computeNodeRange(
      }
}
#endif

export void AMRVolume_set(void *uniform _self,
                          void *uniform _xf,
                          const uniform vec3i &dimensions,
                          void *uniform rootCell,
                          void *uniform octCell)
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;
  assert(self);

  self->super.boundingBox.lower = make_vec3f(0.f);
  self->super.boundingBox.upper = make_vec3f(dimensions);

  self->super.volumeClippingBox.lower = make_vec3f(0.f);
  self->super.volumeClippingBox.upper = make_vec3f(dimensions);

  self->super.intersect = &AMRVolume_intersect;
  self->super.computeSample = &AMRVolume_sample;
  self->super.computeGradient = &AMRVolume_gradient;

  self->super.samplingRate = 1.f;
  self->super.samplingStep = 1.f;

  self->super.gradientShadingEnabled = 0;

  self->super.transferFunction = (TransferFunction *uniform)_xf;

  self->rootDims = dimensions;
  self->rootCell = (AMRCell*uniform)rootCell;
  self->octCell  = (AMROctCell*uniform)octCell;

#if AMR_ISO_SURFACE
  computeTreeRanges(self);
#endif
}



