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

// Functions that defined in CPP files
extern "C" unmasked uniform float BrickTree_scalar_sample(void *uniform cppObject, uniform vec3f &samplePos);
// TODO gradient shading is not working
extern "C" unmasked uniform vec3f BrickTree_scalar_computeGradient(void *uniform cppObject, uniform vec3f &samplePos);

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

inline varying float findBrickValue(BrickTreeVolume *uniform self,
                                    int   blockID,
                                    int   brickID,
                                    vec3i cellPos,
                                    int   parentBrickID,
                                    vec3i parentCellPos)
{
  
  // return 0.2;

  BrickTree  *bt = &(self->forest.data[blockID]);
  ValueBrick *vb = NULL;

#if STREAM_DATA

  // print("%\n", bt->valueBricksStatus[brickID].isLoaded);

  if (!bt->valueBricksStatus[brickID].isLoaded) {
    // request this brick if it is not requested
    if (!bt->valueBricksStatus[brickID].isRequested) {
      bt->valueBricksStatus[brickID].isRequested = true;
    }
    // return average value if this brick is requested but not loaded
    if (brickID == 0)  // root node, return average value of the tree
      return bt->avgValue;
    else {
      // inner node, return the average value of this node which is stored
      // in the parent node
      if (!bt->valueBricksStatus[parentBrickID].isLoaded)
        return bt->avgValue;
      else {
        vb = (ValueBrick *)(bt->valueBrick + parentBrickID);
        return vb->value[parentCellPos.z][parentCellPos.y][parentCellPos.x];
      }
    }
  } else {
    vb = (ValueBrick *)(bt->valueBrick + brickID);
    return vb->value[cellPos.z][cellPos.y][cellPos.x];
  }
  return bt->avgValue;

#else

  vb = (ValueBrick *)(bt->valueBrick + brickID);
  return vb->value[cellPos.z][cellPos.y][cellPos.x];

#endif
}

inline varying float getVoxel(void *uniform _self,
                              const varying vec3i &coord)
{
  // return 0.2f;

  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform) _self;

  // findValue
  int blockID       = getBlockID(self, coord);
  int brickSize     = self->blockWidth;
  int brickID       = 0;
  int parentBrickID = -1;  // parent brick ID
  vec3i cpos          = make_vec3i(0,0,0);
  vec3i parentCellPos = make_vec3i(0,0,0);

  while (brickSize > self->brickSize) {

    int cellSize = brickSize / self->brickSize;

    cpos.x = (coord.x % brickSize) / cellSize;
    cpos.y = (coord.y % brickSize) / cellSize;
    cpos.z = (coord.z % brickSize) / cellSize;

    parentCellPos.x = (coord.x % (brickSize * self->brickSize)) / brickSize;
    parentCellPos.y = (coord.y % (brickSize * self->brickSize)) / brickSize;
    parentCellPos.z = (coord.z % (brickSize * self->brickSize)) / brickSize;

    int ibID = self->forest.data[blockID].brickInfo[brickID].indexBrickID;

    if (ibID == -1) {
      return findBrickValue(self, blockID, brickID, cpos, parentBrickID, parentCellPos);
    } else {
      int childBrickID = self->forest.data[blockID].indexBrick[ibID].childID[cpos.z][cpos.y][cpos.x];
      if (childBrickID == -1) {
        return findBrickValue(self, blockID, brickID, cpos, parentBrickID, parentCellPos);
      } else {
        parentBrickID = brickID;
        brickID       = childBrickID;
      }
    }

    brickSize = cellSize;

  }
  
  cpos.x = coord.x % brickSize;
  cpos.y = coord.y % brickSize;
  cpos.z = coord.z % brickSize;

  return findBrickValue(self, blockID, brickID,cpos,parentBrickID,parentCellPos);
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

static float BrickTreeVolume_sample(void *uniform _self, const vec3f &samplePos)
{
#if 1

  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
  // Lower and upper corners of the box straddling the voxels to be
  // interpolated.
  const vec3i voxelIndex_0 = to_int(samplePos);
  const vec3i voxelIndex_1 = voxelIndex_0 + 1;
  // Fractional coordinates within the lower corner voxel used during interpolation.
  const vec3f fractionalLocalCoordinates = samplePos - to_float(voxelIndex_0);
  // Look up the voxel values to be interpolated.
  float voxelValue_000 = getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z));
  float voxelValue_001 = getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z));
  float voxelValue_010 = getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z));
  float voxelValue_011 = getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z));
  float voxelValue_100 = getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z));
  float voxelValue_101 = getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z));
  float voxelValue_110 = getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z));
  float voxelValue_111 = getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z));

  // Interpolate the voxel values.
  const float voxelValue_00 = voxelValue_000 + fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);
  const float voxelValue_01 = voxelValue_010 + fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);
  const float voxelValue_10 = voxelValue_100 + fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);
  const float voxelValue_11 = voxelValue_110 + fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);
  const float voxelValue_0  = voxelValue_00  + fractionalLocalCoordinates.y * (voxelValue_01  - voxelValue_00 );
  const float voxelValue_1  = voxelValue_10  + fractionalLocalCoordinates.y * (voxelValue_11  - voxelValue_10 );
  const float volumeSample  = voxelValue_0   + fractionalLocalCoordinates.z * (voxelValue_1   - voxelValue_0  );

  return volumeSample;

#else

  BrickTreeVolume *uniform self = (BrickTreeVolume * uniform) _self;
  uniform vec3f uSamplePos[programCount];
  uniform float uReturnValue[programCount];
  uSamplePos[programIndex] = samplePos;
  foreach_active(lane)
  {
    uReturnValue[lane] =
        BrickTree_scalar_sample(self->cppSampler, uSamplePos[lane]);
  }
  return uReturnValue[programIndex];

#endif
}

static vec3f BrickTreeVolume_computeGradient(void *uniform _self, const vec3f &samplePos)
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
    // print("#osp:bt: bounds (%\t%\t%\t)\n"
    //       "                (%\t%\t%\t)\n",
	  // self->super.boundingBox.lower.x,
	  // self->super.boundingBox.lower.y,
	  // self->super.boundingBox.lower.z,
	  // self->super.boundingBox.upper.x,
	  // self->super.boundingBox.upper.y,
	  // self->super.boundingBox.upper.z);

    self->validSize = validSize;
    self->gridSize = gridSize;
    self->brickSize = brickSize;
    self->blockWidth = blockWidth;
}

export void BrickTreeVolume_set_BricktreeForest(void *uniform _self,
                                                void *uniform _forest,
                                                uniform unsigned int size)
{
    BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
    assert(self); 
    BrickTree *uniform forest = (BrickTree *uniform)_forest;
    self->forest.data = forest;
    self->forest.size = size;

    print("[ispc] sizeof(BrickTree) %\n", sizeof(BrickTree));
    // print("[ispc] sizeof(uniform unsigned int64) %\n", sizeof(uniform unsigned int64));
    // print("[ispc] sizeof(uniform float) %\n", sizeof(uniform float));
    // print("[ispc] sizeof(vec2f) %\n", sizeof(vec2f));
    // print("[ispc] sizeof(vec3i) %\n", sizeof(vec3i));
    // print("[ispc] sizeof(uniform ValueBrick* uniform) %\n", sizeof(uniform ValueBrick* uniform));
    // print("[ispc] sizeof(uniform IndexBrick*) %\n", sizeof(uniform IndexBrick*));
    // for (int i = 0; i < size; ++i) {
    //   print("ispc %\n", forest[i].brickInfo);
    //   print("ispc %\n", forest[i].brickInfo[0].indexBrickID);
    // }
    // for (int x = 0; x < 4; ++x) 
    // for (int y = 0; y < 4; ++y) 
    // for (int z = 0; z < 4; ++z) 
    // print("ispc %\n", forest[2].valueBrick[1000].value[x][y][z]);

}