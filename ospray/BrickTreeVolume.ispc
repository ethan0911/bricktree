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
#include "ospray/common/Model.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"
// this module
#include "BrickTreeVolume.ih"
// embree
#include "rtcore.isph"

#define STREAM_DATA 1
#define VECTORIZE 1

struct Address
{
  //! The 1D offset of the voxel in the brick buffer
  varying int cBrickID;
  //! The offset of the voxel in the enclosing brick.
  varying vec3i cpos;
  //! The 1D offset of the voxel's parent brick in the enclosing block.
  varying int pBrickID;
  //! The offset of the voxel's parent brick in the enclosing brick.
  varying vec3i ppos;
};

// Functions that defined in CPP files
extern "C" unmasked uniform float BrickTree_scalar_sample(void *uniform cppObject, uniform vec3f &samplePos);
// TODO gradient shading is not working
extern "C" unmasked uniform vec3f BrickTree_scalar_computeGradient(void *uniform cppObject, uniform vec3f &samplePos);


static varying int getBlockID(BrickTreeVolume *uniform self,
                              const varying vec3i &coord)
{
  int x = coord.x / self->blockWidth;
  int y = coord.y / self->blockWidth;
  int z = coord.z / self->blockWidth;
  x = (x >= self->gridSize.x) ? (self->gridSize.x -1) : x;
  y = (y >= self->gridSize.y) ? (self->gridSize.y -1) : y;
  z = (z >= self->gridSize.z) ? (self->gridSize.z -1) : z;
  return x + y * self->gridSize.x + z * self->gridSize.y * self->gridSize.x;
}

// Find the next hit point in the volume for ray casting based renderers.
static void BrickTreeVolume_stepRay(void *uniform _self, 
                                    varying Ray &ray,
                                    const varying float samplingRate)
{
  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform) _self;
  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const float step = self->super.samplingStep / samplingRate; 
  ray.t0 += step;
}

inline Address BrickTreeVolume_getVoxelAddress(void *uniform _self,
                                               const uniform int &blockID,
                                               const varying vec3i &coord)
{
  /* Cast to the actual volume subtype. */
  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
  BrickTree * bt = (BrickTree *)(self->forest.data + blockID);
  const int       N    = self->brickSize;
  const float rcp_N    = 1.f / (float)N;

  Address address;
  // variables  
  int cBrickID = 0;
  int pBrickID = -1; // parent brick ID
  vec3i cpos = make_vec3i(0,0,0);
  vec3i ppos = make_vec3i(0,0,0);
  // compute sample values
  int brickW = self->blockWidth;
  while (brickW > N) {
    // compute target cell position (indices)
    const int cellW = brickW / N;  // <- N
    cpos.x          = (coord.x % brickW) / cellW;
    cpos.y          = (coord.y % brickW) / cellW;
    cpos.z          = (coord.z % brickW) / cellW;
    ppos.x          = (coord.x % (brickW * N)) / brickW;
    ppos.y          = (coord.y % (brickW * N)) / brickW;
    ppos.z          = (coord.z % (brickW * N)) / brickW;
    // query
    const int ibID = bt->brickInfo[cBrickID].indexBrickID;
    if (ibID == -1) {
      address.cBrickID = cBrickID;
      address.cpos     = cpos;
      address.pBrickID = pBrickID;
      address.ppos     = ppos;
      // has no children
      return address;
    } else {
      // has children
      const int cbID = bt->indexBrick[ibID].childID[cpos.z][cpos.y][cpos.x];
      if (cbID == -1) {  // this children is not ready
        address.cBrickID = cBrickID;
        address.cpos     = cpos;
        address.pBrickID = pBrickID;
        address.ppos     = ppos;
        // has no children
        return address;
      } else {  // this children has a children still
        pBrickID = cBrickID;
        cBrickID = cbID;
      }
    }
    brickW = cellW;
  }
  // now we reached a leaf child
  cpos.x           = coord.x % brickW;
  cpos.y           = coord.y % brickW;
  cpos.z           = coord.z % brickW;
  address.cBrickID = cBrickID;
  address.cpos     = cpos;
  address.pBrickID = pBrickID;
  address.ppos     = ppos;
  // has no children
  return address;
}

static varying float getBrickValue(BrickTreeVolume *uniform self,
                                   const uniform int blockID,
                                   const varying Address &address)
{
  /* Cast to the actual volume subtype. */
  BrickTree *bt  = (BrickTree *)(self->forest.data + blockID);
  ValueBrick *vb = NULL;

  if (bt->valueBricksStatus[address.cBrickID].isLoaded) {
    // here make sure each brick (in each gang) is loaded
    vb = (ValueBrick *)(bt->valueBrick + address.cBrickID);
    return vb->value[address.cpos.z][address.cpos.y][address.cpos.x];
  } else if (bt->valueBricksStatus[address.cBrickID].isRequested != 0) {
    // here make sure each brick (in each gang) has been requested but
    // not yet loaded. we return average value if this brick is requested
    // but not loaded
    if (bt->valueBricksStatus[address.pBrickID].isLoaded != 0) {
      vb = (ValueBrick *)(bt->valueBrick + address.pBrickID);
      return vb->value[address.ppos.z][address.ppos.y][address.ppos.x];
    } else {
      return bt->avgValue;
    }

  } else {
    // request this brick if it is not requested
    bt->valueBricksStatus[address.cBrickID].isRequested = 1;
  }

  return bt->avgValue;
}

inline void BrickTreeVolume_getVoxel(void *uniform _self,
    const varying vec3i &coord,
    varying float &value)
{
  /* Cast to the actual volume subtype. */
  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
  const int blockID    = getBlockID(self, coord);

  foreach_unique(bID in blockID)
  {
    Address address = BrickTreeVolume_getVoxelAddress(self, bID, coord);
    value           = getBrickValue(self, bID, address);
  }
}

static float BrickTreeVolume_sample(void *uniform _self, 
    const vec3f &samplePos)
{

  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
  float result;

#if VECTORIZE

  // Lower and upper corners of the box straddling the voxels to be interpolated.
  const vec3i voxelIndex_0 = to_int(samplePos);
  const vec3i voxelIndex_1 = voxelIndex_0 + 1;
  // Fractional coordinates within the lower corner voxel used during interpolation.
  const vec3f fractionalLocalCoordinates = samplePos - to_float(voxelIndex_0);
  // Look up the voxel values to be interpolated.
  float v_000;
  float v_001;
  float v_010;
  float v_011;
  float v_100;
  float v_101;
  float v_110;
  float v_111;
  BrickTreeVolume_getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z), v_000);
  BrickTreeVolume_getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z), v_001);
  BrickTreeVolume_getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z), v_010);
  BrickTreeVolume_getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z), v_011);
  BrickTreeVolume_getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z), v_100);
  BrickTreeVolume_getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z), v_101);
  BrickTreeVolume_getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z), v_110);
  BrickTreeVolume_getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z), v_111);

  // Interpolate the voxel values.
  const float v_00 = v_000 + fractionalLocalCoordinates.x * (v_001 - v_000);
  const float v_01 = v_010 + fractionalLocalCoordinates.x * (v_011 - v_010);
  const float v_10 = v_100 + fractionalLocalCoordinates.x * (v_101 - v_100);
  const float v_11 = v_110 + fractionalLocalCoordinates.x * (v_111 - v_110);
  const float v_0  = v_00  + fractionalLocalCoordinates.y * (v_01  - v_00 );
  const float v_1  = v_10  + fractionalLocalCoordinates.y * (v_11  - v_10 );
  const float volumeSample = v_0 + fractionalLocalCoordinates.z * (v_1 - v_0);

  result = volumeSample;

#else

  uniform vec3f uSamplePos[programCount];
  uniform float uReturnValue[programCount];
  uSamplePos[programIndex] = samplePos;
  foreach_active(lane)
  {
    uReturnValue[lane] =
        BrickTree_scalar_sample(self->cppSampler, uSamplePos[lane]);
  }
  result = uReturnValue[programIndex];

#endif

  return result;

}

static vec3f BrickTreeVolume_computeGradient(void *uniform _self, 
                                             const vec3f &samplePos)
{
  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
  uniform vec3f uSamplePos[programCount];
  uniform vec3f uReturnValue[programCount];
  uSamplePos[programIndex] = samplePos;
  foreach_active(lane) {
    uReturnValue[lane] = 
      BrickTree_scalar_computeGradient(self->cppSampler,uSamplePos[lane]);
  }
  return uReturnValue[programIndex];
}

export void *uniform BrickTreeVolume_create(void *uniform cppEquivalent) 
{
    BrickTreeVolume *uniform self = uniform new uniform BrickTreeVolume;
    Volume_Constructor(&self->super,cppEquivalent);
    return self;
};

export void BrickTreeVolume_set(void *uniform _self,
                                const uniform vec3i &validSize,
                                const uniform vec3i &gridSize,
                                const uniform int &brickSize,
                                const uniform int &blockWidth,
                                /*! pointer to the c++ side object */
                                void *uniform cppObject,
                                void *uniform cppSampler)
{
    BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
    assert(self);
    self->super.boundingBox.lower = make_vec3f(0.f);
    self->super.boundingBox.upper = make_vec3f(validSize);
    self->super.computeGradient  = BrickTreeVolume_computeGradient;
    self->super.stepRay          = BrickTreeVolume_stepRay;
    self->super.sample           = BrickTreeVolume_sample;
    self->cppObject              = cppObject;
    self->cppSampler             = cppSampler;
    self->validSize = validSize;
    self->gridSize = gridSize;
    self->brickSize = brickSize;
    self->blockWidth = blockWidth;
    // print("#osp:bt: bounds (%\t%\t%\t)\n"
    //       "                (%\t%\t%\t)\n",
    //       self->super.boundingBox.lower.x,
    //       self->super.boundingBox.lower.y,
    //       self->super.boundingBox.lower.z,
    //       self->super.boundingBox.upper.x,
    //       self->super.boundingBox.upper.y,
    //       self->super.boundingBox.upper.z);
}

export void BrickTreeVolume_set_BricktreeForest(void *uniform _self,
                                                void *uniform _forest,
                                                uniform unsigned int size)
{
    BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
    assert(self); 
    uniform BrickTree *uniform forest = (uniform BrickTree *uniform)_forest;
    self->forest.data = forest;
    self->forest.size = size;
}



// static varying float findBrickValue(BrickTreeVolume *uniform self,
//                                     int blockID,
//                                     int brickID,
//                                     vec3i cellPos,
//                                     int parentBrickID,
//                                     vec3i parentCellPos)
// {

//   BrickTree  *bt = (BrickTree *)(self->forest.data + blockID);
//   ValueBrick *vb = NULL;

// #if STREAM_DATA

//   if (bt->valueBricksStatus[brickID].isLoaded) {
//     // here make sure each brick (in each gang) is loaded 
//     vb = (ValueBrick *)(bt->valueBrick + brickID);
//     return vb->value[cellPos.z][cellPos.y][cellPos.x];
//   } else if (bt->valueBricksStatus[brickID].isRequested != 0) {
//     // here make sure each brick (in each gang) has been requested but
//     // not yet loaded. we return average value if this brick is requested 
//     // but not loaded
//     if (bt->valueBricksStatus[parentBrickID].isLoaded != 0) {
//       vb = (ValueBrick *)(bt->valueBrick + parentBrickID);
//       return vb->value[parentCellPos.z][parentCellPos.y][parentCellPos.x];
//     } else{
//       return bt->avgValue;
//     } 

//   } else {
//     // request this brick if it is not requested
//     bt->valueBricksStatus[brickID].isRequested = 1;
//   }

//   return bt->avgValue;

// #else

//   vb = (ValueBrick *)(bt->valueBrick + brickID);
//   return vb->value[cellPos.z][cellPos.y][cellPos.x];

// #endif
// }

// static varying float getVoxel(void *uniform _self,
//                               const varying vec3i &coord)
// {
//   BrickTreeVolume *uniform self = (BrickTreeVolume *uniform) _self;
//   // constants
//   const int       N    = self->brickSize;
//   const float rcp_N    = 1.f / (float)N;
//   const int blockID    = getBlockID(self, coord);
//   BrickTree * bt = (BrickTree *)(self->forest.data + blockID);
//   // variables  
//   int cBrickID = 0;
//   int pBrickID = -1; // parent brick ID
//   vec3i cpos = make_vec3i(0,0,0);
//   vec3i ppos = make_vec3i(0,0,0);
//   // compute sample values
//   int brickW = self->blockWidth;
//   while (brickW > N) {
//     // compute target cell position (indices)
//     const int cellW = brickW / N; // <- N    
//     cpos.x = (coord.x % brickW) / cellW;
//     cpos.y = (coord.y % brickW) / cellW;
//     cpos.z = (coord.z % brickW) / cellW;    
//     ppos.x = (coord.x % (brickW * N)) / brickW;
//     ppos.y = (coord.y % (brickW * N)) / brickW;
//     ppos.z = (coord.z % (brickW * N)) / brickW;
//     // query
//     const int ibID = bt->brickInfo[cBrickID].indexBrickID;
//     if (ibID == -1) {
//       // has no children
//       return findBrickValue(self, blockID, cBrickID, cpos, pBrickID, ppos);
//     } else {
//       // has children
//       const int cbID = 
//         bt->indexBrick[ibID].childID[cpos.z][cpos.y][cpos.x];
//       if (cbID == -1) { // this children is not ready
//         return findBrickValue(self, blockID, cBrickID, cpos, pBrickID, ppos);
//       } else { // this children has a children still
//         pBrickID = cBrickID;
//         cBrickID = cbID;
//       }
//     }
//     brickW = cellW;
//   }
//   // now we reached a leaf child
//   cpos.x = coord.x % brickW;
//   cpos.y = coord.y % brickW;
//   cpos.z = coord.z % brickW;
//   return findBrickValue(self, blockID, cBrickID, cpos, pBrickID, ppos);
// }


//  float v_000 = 
//    getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z));
//  float v_001 =
//    getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z));
//  float v_010 = 
//    getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z));
//  float v_011 =
//    getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z));
//  float v_100 = 
//    getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z));
//  float v_101 = 
//    getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z));
//  float v_110 = 
//    getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z));
//  float v_111 = 
//    getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z));