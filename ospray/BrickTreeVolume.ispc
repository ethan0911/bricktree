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
#include "common/safe_gather.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"

// this module
#include "BrickTreeVolume.ih"
// embree
#include "rtcore.isph"

#define STREAM_DATA 1
#define VECTORIZE 1
#define SAMPLE_EACH_POINT 0

__define_gather_stride(ValueBrick);
__define_gather(ValueBrick);


struct range1f {
  float lo, hi;
};

/*! enum to symbolically iterate the 8 corners of an voxel */
enum { C000=0, C001,C010,C011,C100,C101,C110,C111 };

inline const int getChildBrickID(const uniform BrickTree *varying bt,
                                 int indexBrickID,
                                 varying vec3i cpos)
{
  if (indexBrickID == -1)
    return -1;

  const int cbID = bt->indexBrick[indexBrickID].childID[cpos.z][cpos.y][cpos.x];
  return cbID;
}

struct FindStack
{
  varying bool active;
  varying int32 cBrickID;
  varying int32 pBrickID;
  uniform int32 worldSpaceBrickW;
};

inline uniform FindStack *uniform pushStack(uniform FindStack *uniform stackPtr,
                                            varying int cBrickID,
                                            varying int pBrickID,
                                            uniform int wsBrickW)
{
  unmasked
  {
    stackPtr->active = false;
  }

  stackPtr->cBrickID         = cBrickID;
  stackPtr->pBrickID         = pBrickID;
  stackPtr->active           = true;
  stackPtr->worldSpaceBrickW = wsBrickW;
  return stackPtr + 1;
}

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
  //! current value brick cell width
  varying float size;
};

inline void BrickTreeVolume_getVoxel(void *uniform _self,
                            
         const varying vec3i &coord,
                                     varying float &value);

// Functions that defined in CPP files
extern "C" unmasked uniform float BrickTree_scalar_sample(void *uniform cppObject, uniform vec3f &samplePos);
// TODO gradient shading is not working
extern "C" unmasked uniform vec3f BrickTree_scalar_computeGradient(void *uniform cppObject, uniform vec3f &samplePos);


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


inline varying int getBlockID(BrickTreeVolume *uniform self,
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

// inline Address BrickTreeVolume_getValueBrickAddress(void *uniform _self,
//                                                const varying int &blockID,
//                                                const varying vec3i &coord)
// {
//   /* Cast to the actual volume subtype. */
//   BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
//   const uniform BrickTree * varying bt = (BrickTree *)(self->forest.data + blockID);
//
//   const uniform int       N    = self->brickSize;
//   uniform int wsBrickW = self->blockWidth;
//
//   uniform FindStack stack[16];
//   uniform FindStack *uniform stackPtr = pushStack(&stack[0],0,-1,wsBrickW);
//
//   while (stackPtr > stack) {
//     --stackPtr;
//     if (stackPtr->active) {
//       const int brickID = stackPtr->cBrickID;
//       const int ibID   = bt->brickInfo[brickID].indexBrickID;
//       const uniform int brickW  = stackPtr->worldSpaceBrickW;
//       const uniform int childBrickW   = brickW / N;
//
//       vec3i cpos = make_vec3i((coord.x % brickW) / childBrickW,
//                               (coord.y % brickW) / childBrickW,
//                               (coord.z % brickW) / childBrickW);
//       vec3i ppos = make_vec3i((coord.x % (brickW * N)) / brickW,
//                               (coord.y % (brickW * N)) / brickW,
//                               (coord.z % (brickW * N)) / brickW);
//
//       if (!bt->valueBricksStatus[brickID].isRequested) {
//         bt->valueBricksStatus[brickID].isRequested = 1;
//         bt->valueBricksStatus[brickID].loadWeight = (float)brickW / (float)wsBrickW;
//       }
//
//       const int childBrickID = getChildBrickID(bt, ibID, cpos);
//       cif (childBrickID == -1) {
//         uniform Address address = {brickID, cpos, stackPtr->pBrickID, ppos, brickW};
//         return address;
//       } else {
//         stackPtr = pushStack(stackPtr, childBrickID, brickID, childBrickW);
//       }
//     }
//   }
// }

inline uniform ValueBrick* getValueBrick(BrickTreeVolume *uniform btv,
                                const uniform BrickTree *uniform bt,
                                const int brickID)
{
  const uniform int N   = btv->brickSize;
  const uniform unsigned int capSize  = 1 << 31;
  const uniform unsigned int64 vbSize = N * N * N * bt->numValueBricks * 4;
  const uniform bool huge = (vbSize < capSize) ? false : true; 

  uniform ValueBrick* vb;
  if (huge) {
    const uniform uint8 *uniform const base = (const uniform uint8 *uniform const)bt->valueBrick;
    const uniform int stride                = sizeof(uniform ValueBrick);
    const uniform int segmentLength = 1 << 21;
    varying int segmentID     = brickID / segmentLength;
    varying int segmentOffset = brickID % segmentLength;

    foreach_unique(uniformSegID in segmentID)
    {
      const uniform uint64 scaledStartIndex = (uint64)(uniformSegID * segmentLength) * stride;
      /* properly shifted base address (shifted by 64-bits) */
      const uniform uint8 *uniform base_start = base + scaledStartIndex;
      const uniform ValueBrick *varying data =
        (const uniform ValueBrick *varying)(base_start + segmentOffset * stride);
      vb = (uniform ValueBrick*)data;
    }
  }else{
    vb = (uniform ValueBrick*)(bt->valueBrick + brickID);
  }

  return vb;
}

inline void BrickTreeVolume_getVoxels(void *uniform _self,
                                      const uniform int  blockID,
                                      const varying vec3i  coord,
                                      const uniform int cornerIdx,
                                      varying float *vCorners,
                                      varying unsigned int8  &cvFilled)
{
  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
  const uniform BrickTree *uniform bt = (BrickTree *)(self->forest.data + blockID);

  const uniform int N  = self->brickSize;
  uniform int wsBrickW = self->blockWidth;

  uniform FindStack stack[16];
  uniform FindStack *uniform stackPtr = pushStack(&stack[0],0,-1,wsBrickW);

  while (stackPtr > stack) {
    --stackPtr;
    if (stackPtr->active) {
      const int cBrickID = stackPtr->cBrickID;
      const int pBrickID = stackPtr->pBrickID;
      const int ibID   = bt->brickInfo[cBrickID].indexBrickID;
      const uniform int brickW  = stackPtr->worldSpaceBrickW;
      const uniform int childBrickW   = brickW / N; 

      vec3i cpos = make_vec3i((coord.x % brickW) / childBrickW,
                              (coord.y % brickW) / childBrickW,
                              (coord.z % brickW) / childBrickW);

      vec3i ppos = make_vec3i((coord.x % (brickW * N)) / brickW,
                              (coord.y % (brickW * N)) / brickW,
                              (coord.z % (brickW * N)) / brickW);

      box3i cBrickBBox;
      bool isRoot      = (brickW == self->blockWidth);
      cBrickBBox.lower = (coord / brickW) * brickW;
      cBrickBBox.upper =
          cBrickBBox.lower + make_vec3i(isRoot ? bt->validSize[0] : brickW,
                                        isRoot ? bt->validSize[1] : brickW,
                                        isRoot ? bt->validSize[2] : brickW);

      vec3f center = 0.5 * (to_float(cBrickBBox.lower) + to_float(cBrickBBox.upper));
      float radius           = 0.5 * brickW;
      float area = projectedSphereArea(_self, center, radius);
      area *= (768 * 768 * 0.25);

      // if current brick is not requested, request it
      if (!bt->valueBricksStatus[cBrickID].isRequested) {
        bt->valueBricksStatus[cBrickID].isRequested = 1;
        bt->valueBricksStatus[cBrickID].loadWeight = (float)brickW / (float)wsBrickW;
      }

      // if current brick is not loaded, return parent node value
      if (bt->valueBricksStatus[cBrickID].isLoaded) {
        const int childBrickID = getChildBrickID(bt, ibID, cpos);
        if(childBrickID == -1 || area <= 2) {
          // here make sure each brick (in each gang) is loaded we know that
          // some of the values are unset, we do query for all the gangs

          // --> query myself
          uniform ValueBrick* vb      = getValueBrick(self, bt, cBrickID);
          vCorners[cornerIdx] = vb->value[cpos.z][cpos.y][cpos.x];
          cvFilled |= (1 << cornerIdx);

          // // --> query neighbors
          for (uniform int ii = cornerIdx + 1; ii < 8; ++ii) {
            if ((cvFilled >> ii) & 1)
              continue;

            int step = N / brickW;
            // buggy? never go to else branch
            if (step <= 1) {
              vCorners[ii] = vCorners[cornerIdx];
              cvFilled |= (1 << ii);
            } else {
              const vec3i pos  = make_vec3i((ii & 1) ? cpos.x : cpos.x + 1,
                                           (ii & 2) ? cpos.y : cpos.y + 1,
                                           (ii & 4) ? cpos.z : cpos.z + 1);
              const bool valid = (pos.x < N & pos.y < N & pos.z < N);
              vCorners[ii]     = valid ? vb->value[pos.z][pos.y][pos.x] : 0.0;
              cvFilled |= ((valid ? 1 : 0) << ii);
            }
          }
        } else
        {
          stackPtr = pushStack(stackPtr, childBrickID, cBrickID, childBrickW);
        }
      } else {
        float value;
        if (cBrickID == 0) {
          value = bt->avgValue;
        } else {
          uniform ValueBrick* vb = getValueBrick(self, bt, pBrickID);
          value          = vb->value[ppos.z][ppos.y][ppos.x];
        }
        for (uniform int ii = cornerIdx; ii < 8; ++ii) {
          vCorners[ii] = value;
          cvFilled |= (1 << ii);
          
        }
      }
    }
  }
}


//inline void getBrickValues(BrickTreeVolume *uniform self,
//                           const varying int blockID,
//                           const varying Address &address,
//                           const uniform int cornerIdx,
//                           varying float* vCorners,
//                           varying int8*  cvFilled)
//{
//  /* Cast to the actual volume subtype. */
//  BrickTree  *bt = (BrickTree *)(self->forest.data + blockID);
//  ValueBrick *vb = NULL;
//
//  const uniform int bs = self->brickSize;
//
//  if (bt->valueBricksStatus[address.cBrickID].isLoaded) {
//    // here make sure each brick (in each gang) is loaded
//    // we know that some of the values are unset, we do query for all the gangs
//    // --> query myself
//    vb = (ValueBrick *)(bt->valueBrick + address.cBrickID);
//    const vec3i& cpos = address.cpos;
//    vCorners[cornerIdx] = vb->value[cpos.z][cpos.y][cpos.x];
//    cvFilled[cornerIdx] = 1;
//
//    // --> query neighbors
//    for (uniform int ii = cornerIdx + 1; ii < 8; ++ii) {
//      if (cvFilled[ii] == 1)
//        continue;
//
//      // For neighbors, the epsilon offset in world space is 1,
//      // which equals to the resolution of leaf node(value brick) in the world space.
//      // for any node that is not leaf, their resolution in worls space is more than 1.
//      // here, calculate the epsilon offset in current node's logical coorinate.
//      int step = bs / address.size;
//
//      // in non-leaf value brick, fill the neighbor with myself
//      // in leaf value brick, if neighbor is in the same value brick, fill the new value
//      // Any other case, DO NOT fill the neighbor and fill the value by traversing the tree.
//      if(step == 0){
//        vCorners[ii]     = vCorners[cornerIdx];
//        cvFilled[ii]     = 1;
//      }else{
//        const vec3i pos =make_vec3i((ii & 1) ? cpos.x : cpos.x + bs / address.size,
//                                    (ii & 2) ? cpos.y : cpos.y + bs / address.size,
//                                    (ii & 4) ? cpos.z : cpos.z + bs / address.size);
//        const bool valid = (pos.x < (bs + 0.5) & pos.y < bs & pos.z < bs);
//        vCorners[ii]     = valid ? vb->value[pos.z][pos.y][pos.x] : 0.0;
//        cvFilled[ii]     = valid ? 1 : 0;
//      }
//    }
//  } else /*if (bt->valueBricksStatus[address.cBrickID].isRequested)*/ {
//    // here make sure each brick (in each gang) has been requested but not yet loaded.
//    // we return average value if this brick is requested but not loaded
//    float value;
//    if (bt->valueBricksStatus[address.pBrickID].isLoaded) {
//      vb = (ValueBrick *)(bt->valueBrick + address.pBrickID);
//      value = vb->value[address.ppos.z][address.ppos.y][address.ppos.x];
//    } else {
//      value = bt->avgValue;
//    }
//    for (uniform int ii = cornerIdx; ii < 8; ++ii) {
//      vCorners[ii] = value;
//      cvFilled[ii] = 1;
//    }
//  }
//  /*else {
//    // request this brick if it is not requested
//    bt->valueBricksStatus[address.cBrickID].isRequested = 1;
//  }
//  */
//}

static float BrickTreeVolume_sample(void *uniform _self, const vec3f &samplePos)
{

  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
  float result;


#if VECTORIZE

  // Lower and upper corners of the box straddling the voxels to be interpolated.
  const vec3i voxelIndex_0 = to_int(samplePos);
  const vec3i voxelIndex_1 = voxelIndex_0 + 1;

  // Fractional coordinates within the lower corner voxel used during interpolation.
  const vec3f fractionalLocalCoordinates = samplePos - to_float(voxelIndex_0);

  // here I am using bit fields to indicate if the variable has been loaded
  // -- to mark x-th bit as one, you use  cvFilled |= (1 << x);
  // -- to check if x-th bit is true, you do (cvFilled >> x) & 1
  unsigned int8 cvFilled = 0; // use unsigned to avoid unexpected sign bit
  float vCorners[8] = {0,0,0,0,0,0,0,0};

  int numOfQueries = 0;

  for (uniform int i = 0; i < 8; ++i) {
    if (!((cvFilled >> i) & 1)) {
      const vec3i vtxPos =
          max(min(make_vec3i((i & 1) ? voxelIndex_0.x : voxelIndex_1.x,
                             (i & 2) ? voxelIndex_0.y : voxelIndex_1.y,
                             (i & 4) ? voxelIndex_0.z : voxelIndex_1.z),
                  self->validSize - 1),
              make_vec3i(0));
      
      const int blockID = getBlockID(self, vtxPos);

      foreach_unique(bID in blockID)
      {
        BrickTreeVolume_getVoxels(self, bID, vtxPos, i, vCorners, cvFilled);
      }

      numOfQueries++;

      //BrickTreeVolume_getVoxels(self, blockID, vtxPos, ii, vCorners, cvFilled);
      // const Address address =
      //     BrickTreeVolume_getValueBrickAddress(self, blockID, vtxPos);

      // getBrickValues(self, blockID, address, ii, vCorners, cvFilled);
    }
  }
  //if (numOfQueries > 1)
  //  print("%\n", numOfQueries);



  // Interpolate the voxel values.
  const float v_00 = vCorners[C000] + fractionalLocalCoordinates.x * (vCorners[C001] - vCorners[C000]);
  const float v_01 = vCorners[C010] + fractionalLocalCoordinates.x * (vCorners[C011] - vCorners[C010]);
  const float v_10 = vCorners[C100] + fractionalLocalCoordinates.x * (vCorners[C101] - vCorners[C100]);
  const float v_11 = vCorners[C110] + fractionalLocalCoordinates.x * (vCorners[C111] - vCorners[C110]);
  const float v_0  = v_00  + fractionalLocalCoordinates.y * (v_01  - v_00 );
  const float v_1  = v_10  + fractionalLocalCoordinates.y * (v_11  - v_10 );
  const float volumeSample = v_0 + fractionalLocalCoordinates.z * (v_1 - v_0);

  result = volumeSample;

#elif SAMPLE_EACH_POINT
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
    //self->super.adaptiveSampling = 1;
    self->validSize = validSize;
    self->gridSize = gridSize;
    self->brickSize = brickSize;
    self->blockWidth = blockWidth;

    //print("dir:(%,%,%)\n", self->camera->super.dir);
   
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

export void BrickTreeVolume_set_CameraInfo(void *uniform _self,
                                           void *uniform camera,
                                           void *uniform pCameraDir,
                                           void *uniform pCameraPos,
                                           void *uniform pFovy)                                         
{
  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
  self->cameraInfo.camera     = (PerspectiveCamera *uniform)camera;
  self->cameraInfo.pCameraDir = (uniform vec3f *uniform)pCameraDir;
  self->cameraInfo.pCameraPos = (uniform vec3f *uniform)pCameraPos;
  self->cameraInfo.pFovy = (uniform float *uniform)pFovy;
}

#if SAMPLE_EACH_POINT
inline Address BrickTreeVolume_getVoxelAddress(void *uniform _self,
                                               const uniform int &blockID,
                                               const varying vec3i &coord)
{
  // Cast to the actual volume subtype. 
  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
  BrickTree * bt = (BrickTree *)(self->forest.data + blockID);
  const int       N    = self->brickSize;

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
      Address address = {cBrickID, cpos, pBrickID, ppos};
      return address;
    } else {
      // has children
      const int cbID = bt->indexBrick[ibID].childID[cpos.z][cpos.y][cpos.x];
      if (cbID == -1) {  // this children is not ready
        Address address = {cBrickID, cpos, pBrickID, ppos};
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
  Address address = {cBrickID, cpos, pBrickID, ppos};
  return address;
}



static varying float getBrickValue(BrickTreeVolume *uniform self,
                                   const varying int blockID,
                                   const varying Address &address)
{
  // Cast to the actual volume subtype.
  BrickTree *bt  = (BrickTree *)(self->forest.data + blockID);
  ValueBrick *vb = NULL;

  //?? inefficient data access? gather ?

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
  // Cast to the actual volume subtype.
  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;

  const vec3i samplePos = max(make_vec3i(0),min(self->validSize -1,coord));
  const int blockID    = getBlockID(self, samplePos);

  Address address = BrickTreeVolume_getValueBrickAddress(self, blockID, samplePos);
  value           = getBrickValue(self, blockID, address);

  // foreach_unique(bID in blockID)
  // {
  //   //Address address = BrickTreeVolume_getVoxelAddress(self, bID, coord);
  //   Address address = BrickTreeVolume_getValueBrickAddress(self, bID, samplePos);
  //   value           = getBrickValue(self, bID, address);
  // }
}

#endif


